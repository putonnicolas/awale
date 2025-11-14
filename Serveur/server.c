#include "server.h"

void init(void)
{
#ifdef WIN32
  WSADATA wsa;
  int err = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (err < 0)
  {
    puts("WSAStartup failed !");
    exit(EXIT_FAILURE);
  }
#endif
}

void end(void)
{
#ifdef WIN32
  WSACleanup();
#endif
}

void app(void)
{
  SOCKET sock = init_connection();
  char buffer[BUF_SIZE];
  /* the index for the array */
  int actual = 0;
  int max = sock;

  /* an array for all clients, initialize at NULL */
  Client **clients = malloc(sizeof(Client *) * MAX_CLIENTS);
  if (!clients)
  {
    perror("malloc clients");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < MAX_CLIENTS; i++)
    clients[i] = NULL;

  fd_set rdfs;

  while (1)
  {
    int i = 0;
    FD_ZERO(&rdfs);

    /* add STDIN_FILENO */
    FD_SET(STDIN_FILENO, &rdfs);

    /* add the connection socket */
    FD_SET(sock, &rdfs);

    /* add socket of each client */
    for (i = 0; i < actual; i++)
    {
      if (clients[i] != NULL)
      {
        FD_SET(clients[i]->sock, &rdfs);
      }
    }

    if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
    {
      perror("select()");
      exit(errno);
    }

    /* something from standard input : i.e keyboard */
    if (FD_ISSET(STDIN_FILENO, &rdfs))
    {
      printf("stoping...\n");
      /* stop process when type on keyboard */
      break;
    }
    else if (FD_ISSET(sock, &rdfs))
    {
      /* new client */
      if (actual >= MAX_CLIENTS)
      {
        perror("To many users. Can't accept new connections.\n");
      }

      printf("New client\n");
      SOCKADDR_IN csin = {0};
      socklen_t sinsize = sizeof csin;
      int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
      if (csock == SOCKET_ERROR)
      {
        perror("accept()");
        continue;
      }

      /* after connecting the client sends its name */
      if (read_client(csock, buffer) == -1)
      {
        /* disconnected */
        continue;
      }

      /* what is the new maximum fd ? */
      max = csock > max ? csock : max;

      FD_SET(csock, &rdfs);

      // create a new client object and initialize it
      Client *c = malloc(sizeof(Client));
      if (!c)
      {
        perror("Error malloc Client\n");
        continue;
      }
      c->sock = csock;
      strncpy(c->name, buffer, BUF_SIZE - 1);
      c->challenged = NULL;
      c->challenger = NULL;
      c->game = NULL;
      c->gameToWatch = NULL;

      // check if the player's username is already connected
      char alreadyConnected = 0;
      for (int j = 0; j < MAX_CLIENTS; j++)
      {
        if (clients[j] != NULL)
        {
          if (strcmp(clients[j]->name, c->name) == 0)
          {
            alreadyConnected = 1;
            break;
          }
        }
      }

      if (alreadyConnected)
      {
        send_message_to_specific_client(
            c, "Username already connected. Connection refused.\n", 1);
        closesocket(csock);
        continue;
      }

      // check if the user connects to the game for the first time or not
      char isKnown = check_existing_user(c);
      load_user_data(c);

      char message[4096];
      if (isKnown)
      {
        snprintf(message, sizeof(message),
                 "Welcome back %s\nTo get a list of all available commands "
                 "type [help].\n\nMy bio : %s\n\nPlayers online : %d.",
                 c->name, c->bio, actual);
      }
      else
      {
        snprintf(
            message, sizeof(message),
            "Welcome %s to Online Awale XTrem Experience!\nTo get a list of "
            "all available commands type [help].\n\nPlayers online : %d.",
            c->name, actual);
      }
      send_message_to_specific_client(c, message, 1);

      if (actual != 0)
        list(c, clients, actual, 0);

      for (int i = 0; i < MAX_CLIENTS; i++)
      {
        if (clients[i] == NULL)
        {
          clients[i] = c;
          actual++;
          break;
        }
      }
    }
    else
    {
      printf("Client talking\n");
      int i = 0;
      for (i = 0; i < MAX_CLIENTS; i++)
      {
        if (clients[i] != NULL)
        {
          /* a client is talking */
          if (FD_ISSET(clients[i]->sock, &rdfs))
          {
            Client *client = clients[i];
            int c = read_client(clients[i]->sock, buffer);

            /* client disconnected */
            if (c == 0)
            {
              strncpy(buffer, client->name, BUF_SIZE - 1);
              remove_client(clients, i, &actual);
              strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
              Client dummy_client = {.sock = -999};
              send_message_to_all_clients(clients, dummy_client, MAX_CLIENTS, buffer, 1);
            }
            else
            {
              // extract the command and the args
              ParsedMessage *props = (ParsedMessage *)malloc(sizeof(ParsedMessage));
              extract_props(buffer, props);
              // ----------------------------------------------------------------------------------//
              // args : player to challenge
              if ((strcmp(props->command, "challenge") == 0) &&
                  (props->argc == 1))
              {
                create_challenge(client, clients, props);
              }
              // ----------------------------------------------------------------------------------//
              // args : index of the hole to play
              else if ((strcmp(props->command, "play") == 0) &&
                       (props->argc == 1))
              {
                play_awale(client, props);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "ff") == 0) &&
                       (props->argc == 0))
              {
                forfeit(client);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "list") == 0) &&
                       (props->argc == 0))
              {
                list(client, clients, actual, 1);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "say") == 0) &&
                       (props->argc > 0))
              {
                chat(client, clients, props);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "deny") == 0) &&
                       (props->argc == 0))
              {
                deny(client);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "help") == 0) &&
                       (props->argc == 0))
              {
                help(client);
              }
              // ----------------------------------------------------------------------------------//
              // args : name of the player to watch
              else if ((strcmp(props->command, "watch") == 0) &&
                       (props->argc == 1))
              {
                watch(client, clients, props);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "stopwatch") == 0) &&
                       (props->argc == 0))
              {
                stopwatch(client);
              }
              // ----------------------------------------------------------------------------------//
              // args : none
              else if ((strcmp(props->command, "bio") == 0) &&
                       (props->argc > 0))
              {
                char message[2048] = {0};

                for (int i = 0; i < props->argc; i++)
                {
                  strcat(message, props->argv[i]);
                  strcat(message, " ");
                }
                update_user_bio(client, message);
              }
              else
              {
                char message[2048] = {0};
                snprintf(message, sizeof(message),
                         "Unknown command : %s, or missing arguments.", buffer);
                send_message_to_specific_client(client, message, 1);
              }
              // free the props
              free(props->command);
              for (int i = 0; i < props->argc; i++)
                free(props->argv[i]);
              free(props->argv);
              free(props);
            }

            break;
          }
        }
      }
    }
  }

  clear_clients(clients, actual);
  end_connection(sock);
}

// ---------------------------------------------------- //
void clear_clients(Client **clients, int actual)
{
  // Free and remove all clients that are not null
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i] != NULL)
    {
      closesocket(clients[i]->sock);
      free(clients[i]);
      clients[i] = NULL;
    }
  }
  free(clients);
}

// ---------------------------------------------------- //
void remove_client(Client **clients, int toRemove, int *actual)
{
  // Remove the client from the game he watch
  remove_specific_watcher(clients[toRemove]);

  // make him forfate the game if he's in a party
  if (clients[toRemove]->game)
  {
    forfeit(clients[toRemove]);
  }

  closesocket(clients[toRemove]->sock);

  free(clients[toRemove]);
  clients[toRemove] = NULL;

  (*actual)--;
}

// ---------------------------------------------------- //
void send_message_to_all_clients(Client **clients, Client sender,
                                 int actual, const char *buffer,
                                 char from_server)
{
  int i = 0;
  char message[BUF_SIZE];
  message[0] = '\0';
  for (i = 0; i < actual; i++)
  {
    if (clients[i] != NULL)
    {
      /* we don't send message to the sender */
      if (sender.sock != clients[i]->sock)
      {
        if (from_server == 0)
        {
          strncpy(message, sender.name, BUF_SIZE - 1);
          strncat(message, " : ", sizeof message - strlen(message) - 1);
        }
        strncat(message, buffer, sizeof message - strlen(message) - 1);
        write_client(clients[i]->sock, message);
      }
    }
  }
}

// ---------------------------------------------------- //
void send_message_to_specific_client(Client *client, const char *buffer,
                                     char from_server)
{
  if (!client)
    return;

  Client *client_arr_adapter[1] = {client};
  Client dummy_client = {.sock = -999};
  send_message_to_all_clients(client_arr_adapter, dummy_client, 1, buffer,
                              from_server);
}

// ---------------------------------------------------------------- //
int init_connection(void)
{
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = {0};

  if (sock == INVALID_SOCKET)
  {
    perror("socket()");
    exit(errno);
  }

  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
  {
    perror("bind()");
    exit(errno);
  }

  if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
  {
    perror("listen()");
    exit(errno);
  }

  return sock;
}

void end_connection(int sock) { closesocket(sock); }

// ---------------------------------------------------------------- //
int read_client(SOCKET sock, char *buffer)
{
  int n = 0;

  if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
  {
    perror("recv()");
    /* if recv error we disonnect the client */
    n = 0;
  }

  buffer[n] = 0;

  return n;
}

// ---------------------------------------------------------------- //
void write_client(SOCKET sock, const char *buffer)
{
  if (send(sock, buffer, strlen(buffer), 0) < 0)
  {
    perror("send()");
    exit(errno);
  }
}

// ---------------------------------------------------- //
void list(Client *client, Client **clients, int nbClients, char showBio)
{
  // create a message of size according to the number of clients connected 
  size_t messageSize = 2048 * nbClients;
  char *message = malloc(messageSize);
  char buffer[4096];

  if (!message)
    return;

  snprintf(message, messageSize,
           "\nListe des joueurs actuellement connectés :\n");

  int found = 0;
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i] != NULL)
    {

      if (clients[i]->sock != INVALID_SOCKET && strlen(clients[i]->name) > 0 &&
          strcmp(client->name, clients[i]->name) != 0)
      {
        found = 1;
        snprintf(buffer, sizeof(buffer), "  - %s - victoires : %d", clients[i]->name, clients[i]->wins);
        strncat(message, buffer, sizeof(message) - strlen(message) - 1);

        if (clients[i]->game != NULL)
        {
          strncat(message, "\t(in game)", messageSize - strlen(message) - 1);
        }

        if (showBio && clients[i]->bio && strlen(clients[i]->bio) > 0)
        {
          snprintf(buffer, sizeof(buffer), " — %s", clients[i]->bio);
          strncat(message, buffer, messageSize - strlen(message) - 1);
        }

        strncat(message, "\n", messageSize - strlen(message) - 1);
      }
    }
  }

  if (!found)
  {
    strncat(message, "  Aucun autre joueur connecté.\n",
            messageSize - strlen(message) - 1);
  }

  strncat(message, "\n", messageSize - strlen(message) - 1);

  send_message_to_specific_client(client, message, 1);
  free(message);
}

// ---------------------------------------------------------------- //
int main(int argc, char **argv)
{
  printf("Initializing server...\n");
  init();

  printf("Starting server...\n");
  app();

  printf("Terminating server...\n");
  end();

  return EXIT_SUCCESS;
}

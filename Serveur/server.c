#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

static void init(void)
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

static void end(void)
{
#ifdef WIN32
  WSACleanup();
#endif
}

static void app(void)
{
  SOCKET sock = init_connection();
  char buffer[BUF_SIZE];
  /* the index for the array */
  int actual = 0;
  int max = sock;
  /* an array for all clients */
  Client clients[MAX_CLIENTS];

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
      FD_SET(clients[i].sock, &rdfs);
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

      Client c = {csock};
      strncpy(c.name, buffer, BUF_SIZE - 1);
      printf("%s\n", c.name);
      clients[actual] = c;
      actual++;
    }
    else
    {
      printf("Client talking\n");
      int i = 0;
      for (i = 0; i < actual; i++)
      {
        /* a client is talking */
        if (FD_ISSET(clients[i].sock, &rdfs))
        {
          Client *client = &clients[i];
          int c = read_client(clients[i].sock, buffer);

          ParsedMessage *props = (ParsedMessage *)malloc(sizeof(ParsedMessage));
          extract_props(buffer, props);

          /* client disconnected */
          if (c == 0)
          {
            closesocket(clients[i].sock);
            remove_client(clients, i, &actual);
            strncpy(buffer, client->name, BUF_SIZE - 1);
            strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
            send_message_to_all_clients(clients, *client, actual, buffer, 1);
          }
          else
          {
            // ----------------------------------------------------------------------------------//
            // args : player to challenge
            if ((strcmp(props->command, "challenge") == 0) && (props->argc == 1))
            {
              create_challlenge(client, clients, props);
            }
            // ----------------------------------------------------------------------------------//
            // args : index of the hole to play
            else if ((strcmp(props->command, "play") == 0) && (props->argc == 1))
            {
              play_awale(client, props);
            }
            // ----------------------------------------------------------------------------------//
            // args : none
            else if ((strcmp(props->command, "ff") == 0) && (props->argc == 0))
            {
              forfeit(client);
            }
            // ----------------------------------------------------------------------------------//
            else
            {
              printf("Unknown command : %s, or missing arguments.", buffer);
            }
          }
          break;
        }
      }
    }
  }

  clear_clients(clients, actual);
  end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
  int i = 0;
  for (i = 0; i < actual; i++)
  {
    closesocket(clients[i].sock);
  }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
  /* we remove the client in the array */
  memmove(clients + to_remove, clients + to_remove + 1,
          (*actual - to_remove - 1) * sizeof(Client));
  /* number client - 1 */
  (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender,
                                        int actual, const char *buffer,
                                        char from_server)
{
  int i = 0;
  char message[BUF_SIZE];
  message[0] = 0;
  for (i = 0; i < actual; i++)
  {
    /* we don't send message to the sender */
    if (sender.sock != clients[i].sock)
    {
      if (from_server == 0)
      {
        strncpy(message, sender.name, BUF_SIZE - 1);
        strncat(message, " : ", sizeof message - strlen(message) - 1);
      }
      strncat(message, buffer, sizeof message - strlen(message) - 1);
      write_client(clients[i].sock, message);
    }
  }
}

static void send_message_to_specific_client(Client client, const char *buffer,
                                            char from_server)
{
  Client *client_arr_adapter = (Client *)malloc(sizeof(Client));
  client_arr_adapter[0] = client;
  Client dummy_client;
  send_message_to_all_clients(client_arr_adapter, dummy_client, 1, buffer, from_server);
}

static int init_connection(void)
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

static void end_connection(int sock) { closesocket(sock); }

static int read_client(SOCKET sock, char *buffer)
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

static void extract_props(const char *src, ParsedMessage *msg)
{
  if (!src || !msg || *src == '\0')
    return;

  char *copy = strdup(src);
  if (!copy)
    return;

  char *token = strtok(copy, " ");
  if (!token)
  {
    free(copy);
    return;
  }

  msg->command = strdup(token);
  msg->argc = 0;
  msg->argv = NULL;

  // Récupération des arguments restants
  while ((token = strtok(NULL, " ")) != NULL)
  {
    char **tmp = realloc(msg->argv, sizeof(char *) * (msg->argc + 1));
    if (!tmp)
    {
      // En cas d’erreur realloc, on libère ce qui a été alloué avant de sortir
      for (int i = 0; i < msg->argc; i++)
        free(msg->argv[i]);
      free(msg->argv);
      free(msg->command);
      free(copy);
      return;
    }
    msg->argv = tmp;
    msg->argv[msg->argc] = strdup(token);
    msg->argc++;
  }

  free(copy);
}

static void free_parsed_message(ParsedMessage *msg)
{
  if (!msg)
    return;
  free(msg->command);
  for (int i = 0; i < msg->argc; i++)
  {
    free(msg->argv[i]);
  }
  free(msg->argv);
}

static void write_client(SOCKET sock, const char *buffer)
{
  if (send(sock, buffer, strlen(buffer), 0) < 0)
  {
    perror("send()");
    exit(errno);
  }
}

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

// ---------------------------------------------------------------- //
// Creates a challenge from the client to the challenged client and sends it to the challenged client
static void create_challlenge(Client *client, Client *clients, ParsedMessage *props)
{
  if (client->game == NULL)
  {
    int clientFound = 0;
    for (int j = 0; j < MAX_CLIENTS; ++j)
    {
      if (strcmp(clients[j].name, props->argv[0]) == 0)
      {
        // the challenged client is found
        clientFound = 1;
        client->challenged = &clients[j];
        if (clients[j].challenged == client)
        {
          // launch a game
          Game *game = (Game *)malloc(sizeof(Game));
          game->capturedSeedClient1 = 0;
          game->client1 = client;
          game->capturedSeedClient2 = 0;
          game->client2 = &clients[j];
          game->currentPlayer = client;
          for (int k = 0; k < AWALE_BOARD_SIZE; k++)
          {
            game->awaleBoard[k] = 4;
          }

          clients[j].game = client->game = game;

          // Notify the players that the game is starting
          char message[2048];
          snprintf(message, sizeof(message),
                   "The awale game versus %s has started.",
                   clients[j].name);

          send_message_to_specific_client(*client, message, 1);

          snprintf(message, sizeof(message),
                   "The awale game versus %s has started.",
                   client->name);

          send_message_to_specific_client(clients[j], message, 1);
        }
        else
        {
          clients[j].challenger = client;
          char message[2048];
          snprintf(message, sizeof(message),
                   "Challenge sent to player %s.",
                   props->argv[0]);

          send_message_to_specific_client(*client, message, 1);

          snprintf(message, sizeof(message),
                   "Challenge received from player %s.",
                   client->name);

          send_message_to_specific_client(clients[j], message, 1);
        }
      }
    }
    if (!clientFound)
    {
      char message[2048];
      snprintf(message, sizeof(message),
               "The player %s is not connected.",
               props->argv[0]);

      send_message_to_specific_client(*client, message, 1);
    }
  }
}

// ---------------------------------------------------------------- //
static void forfeit(Client *client)
{
  if (client->game == NULL)
  {
    send_message_to_specific_client(*client, "You are currently not in a game.", 1);
  }
  else
  {
    Game *currentGame = client->game;
    Client *opponent = currentGame->client1 != client ? currentGame->client1 : currentGame->client2;
    char message[2048];
    snprintf(message, sizeof(message),
             "The player %s has forfeited. You won !",
             client->name);

    send_message_to_specific_client(*opponent, message, 1);

    send_message_to_specific_client(*client, "You forfeited ...", 1);

    client->game = NULL;
    opponent->game = NULL;
    client->challenged = NULL;
    opponent->challenged = NULL;
    free(currentGame);
  }
}

// ---------------------------------------------------------------- //
static void play_awale(Client *client, ParsedMessage *props)
{
  Game *game = client->game;
  if (game->currentPlayer != client)
  {
    send_message_to_specific_client(*client, "It's not your turn !", 1);
    return;
  }


  // get the parameters of the shot
  int caseIndex = atoi(props->argv[0]);

  // handle errors
  if (caseIndex < 0 || caseIndex >= (AWALE_BOARD_SIZE / 2))
  {
    send_message_to_specific_client(*client, "The case index should be between 0 and 5... try again.", 1);
    return;
  }

  // is the player the client1 ?
  short client1 = 1;
  // update the board associated with the game
  // client1 caseIndex goes from 0 to 5
  // however client2 caseIndex goes from 6 to 11
  // but both player choose between 0 and 6 so translation is needed
  if (client == game->client2)
  {
    caseIndex = (AWALE_BOARD_SIZE - 1) - caseIndex;
    client1 = 0;
  }

  int nbSeedsOnChooseCase = game->awaleBoard[caseIndex];
  game->awaleBoard[caseIndex] = 0;

  // add seeds to concerned holes
  for (int i = 0; i < nbSeedsOnChooseCase; ++i)
  {
    int currentIndex = (caseIndex + i + 1) % AWALE_BOARD_SIZE;
    game->awaleBoard[currentIndex]++;
  }

  // send the new version to both players
}
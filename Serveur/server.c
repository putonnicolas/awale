#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "server.h"

static void init(void) {
#ifdef WIN32
  WSADATA wsa;
  int err = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (err < 0) {
    puts("WSAStartup failed !");
    exit(EXIT_FAILURE);
  }
#endif
}

static void end(void) {
#ifdef WIN32
  WSACleanup();
#endif
}

static void app(void) {
  SOCKET sock = init_connection();
  char buffer[BUF_SIZE];
  /* the index for the array */
  int actual = 0;
  int max = sock;
  /* an array for all clients */
  Client clients[MAX_CLIENTS];

  fd_set rdfs;

  while (1) {
    int i = 0;
    FD_ZERO(&rdfs);

    /* add STDIN_FILENO */
    FD_SET(STDIN_FILENO, &rdfs);

    /* add the connection socket */
    FD_SET(sock, &rdfs);

    /* add socket of each client */
    for (i = 0; i < actual; i++) {
      FD_SET(clients[i].sock, &rdfs);
    }

    if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) {
      perror("select()");
      exit(errno);
    }

    /* something from standard input : i.e keyboard */
    if (FD_ISSET(STDIN_FILENO, &rdfs)) {

      printf("stoping...\n");
      /* stop process when type on keyboard */
      break;
    } else if (FD_ISSET(sock, &rdfs)) {
      /* new client */
      printf("New client\n");
      SOCKADDR_IN csin = {0};
      socklen_t sinsize = sizeof csin;
      int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
      if (csock == SOCKET_ERROR) {
        perror("accept()");
        continue;
      }

      /* after connecting the client sends its name */
      if (read_client(csock, buffer) == -1) {
        /* disconnected */
        continue;
      }

      /* what is the new maximum fd ? */
      max = csock > max ? csock : max;

      FD_SET(csock, &rdfs);

      Client c = {csock};
      strncpy(c.name, buffer, BUF_SIZE - 1);
      printf("%s\n", c.name);
      c.challenged = NULL;
      c.challenger = NULL;

      char message[2048];
      snprintf(message, sizeof(message),
               "Welcome %s to Online Awale XTrem Experience!\nTo get a list of "
               "all available commands type [help].\n\nPlayers online : %d.",
               c.name, actual);
      send_message_to_specific_client(c, message, 1);
      if (actual != 0)
        list(&c, clients, actual);

      clients[actual] = c;
      actual++;
    } else {
      printf("Client talking\n");
      int i = 0;
      for (i = 0; i < actual; i++) {
        /* a client is talking */
        if (FD_ISSET(clients[i].sock, &rdfs)) {
          Client *client = &clients[i];
          int c = read_client(clients[i].sock, buffer);

          ParsedMessage *props = (ParsedMessage *)malloc(sizeof(ParsedMessage));
          extract_props(buffer, props);

          /* client disconnected */
          if (c == 0) {
            closesocket(clients[i].sock);
            remove_client(clients, i, &actual);
            strncpy(buffer, client->name, BUF_SIZE - 1);
            strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
            send_message_to_all_clients(clients, *client, actual, buffer, 1);
          } else {
            // ----------------------------------------------------------------------------------//
            // args : player to challenge
            if ((strcmp(props->command, "challenge") == 0) &&
                (props->argc == 1)) {
              create_challenge(client, clients, props);
            }
            // ----------------------------------------------------------------------------------//
            // args : index of the hole to play
            else if ((strcmp(props->command, "play") == 0) &&
                     (props->argc == 1)) {
              play_awale(client, props);
            }
            // ----------------------------------------------------------------------------------//
            // args : none
            else if ((strcmp(props->command, "ff") == 0) &&
                     (props->argc == 0)) {
              forfeit(client);
            }
            else if ((strcmp(props->command, "list") == 0) && (props->argc == 0))
            {
              list(client, clients, actual);
            } else if ((strcmp(props->command, "say") == 0) &&
                       (props->argc > 0)) {
              chat(client, clients, actual, props);
            } else if ((strcmp(props->command, "deny") == 0) &&
                       (props->argc == 0)) {
              deny(client);
            }
            // ----------------------------------------------------------------------------------//
            // args : none
            else if ((strcmp(props->command, "help") == 0) && (props->argc == 0))
            {
              help(client);
            }
            // ----------------------------------------------------------------------------------//
            // args : name of the player to watch
            else if ((strcmp(props->command, "watch") == 0) && (props->argc == 1))
            {
              watch(client, clients, props);
            }
            // ----------------------------------------------------------------------------------//
            // args : name of the player to watch
            else if ((strcmp(props->command, "stopwatch") == 0) && (props->argc == 0))
            {
              stopwatch(client);
            }
            // ----------------------------------------------------------------------------------//
            else
            {
              printf("Unknown command : %s, or missing arguments.", buffer);
            }
          }
          break;

          // free the props
          free(props->command);
          for (int i = 0; i < props->argc; i++)
          {
            free(props->argv[i]);
          }
          free(props);
        }
      }
    }
  }

  clear_clients(clients, actual);
  end_connection(sock);
}

static void clear_clients(Client *clients, int actual) {
  int i = 0;
  for (i = 0; i < actual; i++) {
    closesocket(clients[i].sock);
  }
}

// ---------------------------------------------------- //
static void remove_client(Client *clients, int to_remove, int *actual)
{
  Client *removedClient = &clients[to_remove];

  printf("remove \n");
  // Retirer le client des watchers de toutes les games
  remove_specific_watcher(removedClient);
  
  // On retire le client du tableau principal
  memmove(clients + to_remove, clients + to_remove + 1,
    (*actual - to_remove - 1) * sizeof(Client));
    (*actual)--;
    printf("end remove \n");
}

static void send_message_to_all_clients(Client *clients, Client sender,
                                        int actual, const char *buffer,
                                        char from_server) {
  int i = 0;
  char message[BUF_SIZE];
  message[0] = 0;
  for (i = 0; i < actual; i++) {
    /* we don't send message to the sender */
    if (sender.sock != clients[i].sock) {
      if (from_server == 0) {
        strncpy(message, sender.name, BUF_SIZE - 1);
        strncat(message, " : ", sizeof message - strlen(message) - 1);
      }
      strncat(message, buffer, sizeof message - strlen(message) - 1);
      write_client(clients[i].sock, message);
    }
  }
}

// ---------------------------------------------------- //
static void send_message_to_specific_client(Client client, const char *buffer,
                                            char from_server) {
  Client *client_arr_adapter = (Client *)malloc(sizeof(Client));
  client_arr_adapter[0] = client;
  Client dummy_client;
  send_message_to_all_clients(client_arr_adapter, dummy_client, 1, buffer,
                              from_server);
}

static int init_connection(void) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = {0};

  if (sock == INVALID_SOCKET) {
    perror("socket()");
    exit(errno);
  }

  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR) {
    perror("bind()");
    exit(errno);
  }

  if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
    perror("listen()");
    exit(errno);
  }

  return sock;
}

static void end_connection(int sock) { closesocket(sock); }

static int read_client(SOCKET sock, char *buffer) {
  int n = 0;

  if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
    perror("recv()");
    /* if recv error we disonnect the client */
    n = 0;
  }

  buffer[n] = 0;

  return n;
}

static void extract_props(const char *src, ParsedMessage *msg) {
  if (!src || !msg || *src == '\0')
    return;

  char *copy = strdup(src);
  if (!copy)
    return;

  char *token = strtok(copy, " ");
  if (!token) {
    free(copy);
    return;
  }

  msg->command = strdup(token);
  msg->argc = 0;
  msg->argv = NULL;

  // Récupération des arguments restants
  while ((token = strtok(NULL, " ")) != NULL) {
    char **tmp = realloc(msg->argv, sizeof(char *) * (msg->argc + 1));
    if (!tmp) {
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

static void free_parsed_message(ParsedMessage *msg) {
  if (!msg)
    return;
  free(msg->command);
  for (int i = 0; i < msg->argc; i++) {
    free(msg->argv[i]);
  }
  free(msg->argv);
}

static void write_client(SOCKET sock, const char *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

int main(int argc, char **argv) {
  printf("Initializing server...\n");
  init();

  printf("Starting server...\n");
  app();

  printf("Terminating server...\n");
  end();

  return EXIT_SUCCESS;
}

// ---------------------------------------------------------------- //
// Creates a challenge from the client to the challenged client and sends it to
// the challenged client
static void create_challenge(Client *client, Client *clients,
                             ParsedMessage *props) {
  if (client->game == NULL && (strcmp(client->name, props->argv[0]) != 0)) {
    int clientFound = 0;
    for (int j = 0; j < MAX_CLIENTS; ++j) {
      if (strcmp(clients[j].name, props->argv[0]) == 0) {
        // the challenged client is found
        clientFound = 1;
        client->challenged = &clients[j];
        if (clients[j].challenged == client) {
          // launch a game
          Game *game = (Game *)malloc(sizeof(Game));
          game->capturedSeeds[0] = 0;
          game->capturedSeeds[1] = 0;
          game->clients[0] = client;
          game->clients[1] = &clients[j];

          // Random choice of the first player
          srand(time(NULL));
          int starterIndex = rand() % 2;
          game->currentPlayer = game->clients[starterIndex];
          for (int k = 0; k < HALF_AWALE_BOARD_SIZE; k++) {
            game->halfAwaleBoards[0][k] = 4;
            game->halfAwaleBoards[1][k] = 4;
          }

          clients[j].game = client->game = game;

          // Notify the players that the game is starting
          char message[2048];
          snprintf(message, sizeof(message),
                   "The awale game versus %s has started.", clients[j].name);

          send_message_to_specific_client(*client, message, 1);

          snprintf(message, sizeof(message),
                   "The awale game versus %s has started...", client->name);

          send_message_to_specific_client(clients[j], message, 1);
          sendEndOfTurnMessage(game, START);
          sendEndOfTurnMessageToWatchers(game, START);
        }
        else
        {
          clients[j].challenger = client;
          char message[2048];
          snprintf(message, sizeof(message), "Challenge sent to player %s.",
                   props->argv[0]);

          send_message_to_specific_client(*client, message, 1);

          snprintf(message, sizeof(message),
                   "Challenge received from player %s.", client->name);

          send_message_to_specific_client(clients[j], message, 1);
        }
      }
    }
    if (!clientFound) {
      char message[2048];
      snprintf(message, sizeof(message), "The player %s is not connected.",
               props->argv[0]);

      send_message_to_specific_client(*client, message, 1);
    }
  }
}

static void deny(Client *client) {
  if (client->challenger == NULL) {
    send_message_to_specific_client(*client, "No one has challenged you yet.",
                                    1);
  } else {

    char message[2048];
    snprintf(message, sizeof(message),
             "The player %s has denied your challenge.", client->name);
    send_message_to_specific_client(*client->challenger, message, 1);

    snprintf(message, sizeof(message), "You denied %s's challenge.",
             client->challenger->name);
    send_message_to_specific_client(*client, message, 1);
    client->challenger->challenged = NULL;
    client->challenger = NULL;
  }
}

// ---------------------------------------------------------------- //
static void forfeit(Client *client) {
  if (client->game == NULL) {
    send_message_to_specific_client(*client, "You are currently not in a game.",
                                    1);
  } else {
    Game *currentGame = client->game;
    Client *opponent = currentGame->clients[0] != client
                           ? currentGame->clients[0]
                           : currentGame->clients[1];
    char message[2048];
    snprintf(message, sizeof(message), "The player %s has forfeited. You won !",
             client->name);

    send_message_to_specific_client(*opponent, message, 1);

    send_message_to_specific_client(*client, "You forfeited ...", 1);

    client->game = NULL;
    opponent->game = NULL;
    client->challenged = NULL;
    opponent->challenged = NULL;
    client->challenger = NULL;
    opponent->challenger = NULL;
    currentGame->nbWatchers = 0;
    free(currentGame);
  }
}

// ---------------------------------------------------------------- //
static void play_awale(Client *client, ParsedMessage *props) {
  Game *game = client->game;
  if (game->currentPlayer != client) {
    send_message_to_specific_client(*client, "It's not your turn !", 1);
    return;
  }

  // get the parameters of the shot
  int holeIndex = atoi(props->argv[0]) - 1;

  // handle errors
  if (holeIndex < 0 || holeIndex >= HALF_AWALE_BOARD_SIZE) {
    send_message_to_specific_client(
        *client, "The hole index should be between 1 and 6... try again.", 1);
    return;
  }

  // is the player the client1 ?
  // clientIndex == 0 : player 1
  // clientIndex == 1 : player 2
  short clientIndex = (client == game->clients[1]) ? 1 : 0;
  if (game->halfAwaleBoards[clientIndex][holeIndex] == 0) {
    send_message_to_specific_client(*client, "The hole is empty... try again.",
                                    1);
    return;
  }
  int nbSeedsOnChooseHole = game->halfAwaleBoards[clientIndex][holeIndex];
  // add seeds to concerned holes
  game->halfAwaleBoards[clientIndex][holeIndex] = 0;
  for (int i = 1; i <= nbSeedsOnChooseHole; i++) {
    short currentBoardIndex =
        ((holeIndex + i) / HALF_AWALE_BOARD_SIZE + clientIndex) % 2;
    short currentHoleIndex = (holeIndex + i) % HALF_AWALE_BOARD_SIZE;

    // increment hole
    int *currentHole =
        &(game->halfAwaleBoards[currentBoardIndex][currentHoleIndex]);
    (*currentHole)++;

    // Check if we are in the opponent's board
    if (currentBoardIndex != clientIndex) {
      // Take the seeds if we can
      if (*currentHole == 2 || *currentHole == 3) {
        game->capturedSeeds[clientIndex] += *currentHole;
        *currentHole = 0;
      }
    }
  }

  // Check if one player's board is empty
  char isEmpty[2] = {1, 1};
  for (int i = 0; i < HALF_AWALE_BOARD_SIZE; i++) {

    isEmpty[0] &= game->halfAwaleBoards[0][i] == 0;
    isEmpty[1] &= game->halfAwaleBoards[1][i] == 0;
  }

  // if either side is empty or someone has 25 points
  char isWon = 0;
  for (int i = 0; i < 2; i++)
    isWon |= (isEmpty[i] | (game->capturedSeeds[i] >= 25));

  if (isWon) {
    char winnerIndex = game->capturedSeeds[1] > game->capturedSeeds[0];
    sendEndOfTurnMessage(game, ENDGAME);
    sendEndOfTurnMessageToWatchers(game, ENDGAME);

    send_message_to_specific_client(
        *game->clients[winnerIndex],
        "Félications ! Vous avez battu à plattes coutures votre adversaire !f",
        1);
    send_message_to_specific_client(*game->clients[(winnerIndex + 1) % 2],
                                    "Dommage... vous avez perdu...", 1);
  }
  // send the new version to both players
  else {
    game->currentPlayer = game->clients[(clientIndex + 1) % 2];
    sendEndOfTurnMessage(game, NORMAL);
    sendEndOfTurnMessageToWatchers(game, NORMAL);
  }
}

void chat(Client *client, Client *otherClients, int clientNb,
          ParsedMessage *props) {
  char message[2048] = {0};

  for (int i = 0; i < props->argc; i++) {
    strcat(message, props->argv[i]);
    strcat(message, " ");
  }

  for (int i = 1; i < props->argc; i++) {
  }

  for (int i = 0; i < clientNb; i++) {
    if (&otherClients[i] != client) {
      send_message_to_all_clients(otherClients, *client, clientNb, message, 0);
    }
  }
}

void sendEndOfTurnMessage(Game *game, EndOfTurnMessageMode mode) {
  char message[2048];
  char buffer[2048];

  for (int i = 0; i < 2; i++) {
    Client *client = game->clients[i];
    Client *opponent = game->clients[(i + 1) % 2];

    // Construction du message principal
    if (mode == START) {
      snprintf(message, sizeof(message), "C'est le tour de : %s\n\n",
               game->currentPlayer->name);
    } else {
      snprintf(message, sizeof(message),
               "\n===== Fin du tour =====\n"
               "C'est le tour de : %s\n\n",
               game->currentPlayer->name);
    }

    // Le joueur voit sa propre board en bas
    int top = (i == 0) ? 1 : 0;
    int bottom = !top;

    strncat(message, "Plateau actuel :\n\n",
            sizeof(message) - strlen(message) - 1);

    // Ligne du haut (adversaire)
    strncat(message, "  ", sizeof(message) - strlen(message) - 1);
    for (int j = HALF_AWALE_BOARD_SIZE - 1; j >= 0; j--) {
      snprintf(buffer, sizeof(buffer), "| %2d ", game->halfAwaleBoards[top][j]);
      strncat(message, buffer, sizeof(message) - strlen(message) - 1);
    }
    strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

    // Ligne du bas (joueur)
    strncat(message, "  ", sizeof(message) - strlen(message) - 1);
    for (int j = 0; j < HALF_AWALE_BOARD_SIZE; j++) {
      snprintf(buffer, sizeof(buffer), "| %2d ",
               game->halfAwaleBoards[bottom][j]);
      strncat(message, buffer, sizeof(message) - strlen(message) - 1);
    }
    strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

    // Graines capturées (on écrit ligne par ligne pour éviter les grands
    // snprintf)
    snprintf(buffer, sizeof(buffer), "\nGraines capturées :\n");
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);

    snprintf(buffer, sizeof(buffer), "  %s : %d\n", client->name,
             game->capturedSeeds[i]);
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);

    snprintf(buffer, sizeof(buffer), "  %s : %d\n", opponent->name,
             game->capturedSeeds[(i + 1) % 2]);
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);

    strncat(message, "\n========================\n",
            sizeof(message) - strlen(message) - 1);

    // Envoi du message final
    send_message_to_specific_client(*client, message, 1);
  }
}

static void sendEndOfTurnMessageToWatchers(Game *game, EndOfTurnMessageMode mode)
{
  if (game->nbWatchers == 0)
    return;

  char message[2048];
  char buffer[2048];

  // Construction du message de base (même visuel que les joueurs)
  if (mode == START)
  {
    snprintf(message, sizeof(message),
             "=== Début du tour ===\n"
             "C'est le tour de : %s\n\n",
             game->currentPlayer->name);
  }
  else
  {
    snprintf(message, sizeof(message),
             "=== Fin du tour ===\n"
             "C'est le tour de : %s\n\n",
             game->currentPlayer->name);
  }

  strncat(message, "Plateau actuel :\n\n", sizeof(message) - strlen(message) - 1);

  // Ligne du haut (client2)
  strncat(message, "  ", sizeof(message) - strlen(message) - 1);
  for (int j = HALF_AWALE_BOARD_SIZE - 1; j >= 0; j--)
  {
    snprintf(buffer, sizeof(buffer), "| %2d ", game->halfAwaleBoards[1][j]);
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);
  }
  strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

  // Ligne du bas (client1)
  strncat(message, "  ", sizeof(message) - strlen(message) - 1);
  for (int j = 0; j < HALF_AWALE_BOARD_SIZE; j++)
  {
    snprintf(buffer, sizeof(buffer), "| %2d ", game->halfAwaleBoards[0][j]);
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);
  }
  strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

  // Graines capturées
  snprintf(buffer, sizeof(buffer), "\nGraines capturées :\n");
  strncat(message, buffer, sizeof(message) - strlen(message) - 1);

  snprintf(buffer, sizeof(buffer), "  %s : %d\n",
           game->clients[0]->name, game->capturedSeeds[0]);
  strncat(message, buffer, sizeof(message) - strlen(message) - 1);

  snprintf(buffer, sizeof(buffer), "  %s : %d\n",
           game->clients[1]->name, game->capturedSeeds[1]);
  strncat(message, buffer, sizeof(message) - strlen(message) - 1);

  strncat(message, "\n========================\n", sizeof(message) - strlen(message) - 1);

  // Envoi à tous les spectateurs
  for (int i = 0; i < game->nbWatchers; i++)
  {
    Client *watcher = game->watchers[i];
    if (watcher != NULL && watcher->sock != INVALID_SOCKET)
    {
      send_message_to_specific_client(*watcher, message, 1);
    }
  }
}

// ---------------------------------------------------- //

static void list(Client *client, Client *clients, int nbClients) {
  char message[2048];
  char buffer[2048];

  snprintf(message, sizeof(message),
           "\nListe des joueurs actuellement connectés :\n");

  int found = 0;
  for (int i = 0; i < nbClients; i++) {
    if (clients[i].sock != INVALID_SOCKET && strlen(clients[i].name) > 0 &&
        strcmp(client->name, clients[i].name) != 0) {
      found = 1;
      snprintf(buffer, sizeof(buffer), "  - %s", clients[i].name);
      strncat(message, buffer, sizeof(message) - strlen(message) - 1);

      if (clients[i].game != NULL) {
        strncat(message, "\t(in game)", sizeof(message) - strlen(message) - 1);
      }

      strncat(message, "\n", sizeof(message) - strlen(message) - 1);
    }
  }

  if (!found)
    strncat(message, "  Aucun autre joueur connecté.\n",
            sizeof(message) - strlen(message) - 1);

  strncat(message, "\n", sizeof(message) - strlen(message) - 1);

  send_message_to_specific_client(*client, message, 1);
}

// ---------------------------------------------------- //
static void help(Client *client)
{
  char message[2048];
  snprintf(message, sizeof(message),
           "\n--- Liste des commandes disponibles ---\n"
           "  list                  → Affiche la liste des joueurs connectés\n"
           "  challenge <nom>       → Défie un joueur en duel\n"
           "  play <index>          → Joue un coup (index du trou à jouer)\n"
           "  ff                    → Abandonne la partie en cours\n"
           "  watch <nom>           → Observe la partie d’un joueur\n"
           "  stopwatch             → Arrête d'observer la partie actuellement observée\n"
           "  say <message>         → Envoie un message à tout les joueurs connectés\n"
           "  help                  → Affiche cette aide\n"
           "\n----------------------------------------\n");

  send_message_to_specific_client(*client, message, 1);
}

// ---------------------------------------------------- //
static void watch(Client *client, Client *clients, ParsedMessage *props)
{
  short clientFound;
  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    if (strcmp(clients[i].name, props->argv[0]) == 0)
    {
      if (clients[i].game != NULL)
      {
        // the client to watch is found
        clientFound = 1;
        clients[i].game->watchers[clients[i].game->nbWatchers] = client;
        clients[i].game->nbWatchers++;
        client->gameToWatch = clients[i].game;
      }
    }
  }

  if (!clientFound)
  {
    char message[2048];
    snprintf(message, sizeof(message),
             "The player %s is not in game.",
             props->argv[0]);

    send_message_to_specific_client(*client, message, 1);
  }
}

// ---------------------------------------------------- //
static void stopwatch(Client *client)
{
  if (client->gameToWatch == NULL)
  {
    send_message_to_specific_client(*client, "You are currently not watching a game.\n", 1);
    return;
  }

  Game *game = client->gameToWatch;

  remove_specific_watcher(client);
  client->gameToWatch = NULL;

  char message[2048];
  snprintf(message, sizeof(message),
           "You stopped watching the game of %s and %s.\n",
           game->clients[0]->name,
           game->clients[1] ? game->clients[1]->name : "(waiting for opponent)");

  send_message_to_specific_client(*client, message, 1);
}

// ---------------------------------------------------- //
static void remove_specific_watcher(Client *client)
{
  if (client->gameToWatch != NULL)
    return;

  for (int i = 0; i < client->gameToWatch->nbWatchers; i++)
  {
    if (client->gameToWatch->watchers[i] == client)
    {
      remove_watcher(client->gameToWatch, i);
      break;
    }
  }
}

// ---------------------------------------------------- //
static void remove_watcher(Game *game, int index)
{
  if (index < 0 || index >= game->nbWatchers)
    return;

  memmove(game->watchers + index,
          game->watchers + index + 1,
          (game->nbWatchers - index - 1) * sizeof(Client *));
  game->nbWatchers--;
}

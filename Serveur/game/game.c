#include "game.h"

// ---------------------------------------------------------------- //
void play_awale(Client *client, ParsedMessage *props)
{
  Game *game = client->game;
  if (game->currentPlayer != client)
  {
    send_message_to_specific_client(client, "It's not your turn !", 1);
    return;
  }

  // get the parameters of the shot
  int holeIndex = atoi(props->argv[0]) - 1;

  // handle errors
  if (holeIndex < 0 || holeIndex >= HALF_AWALE_BOARD_SIZE)
  {
    send_message_to_specific_client(
        client, "The hole index should be between 1 and 6... try again.", 1);
    return;
  }

  // is the player the client1 ?
  // clientIndex == 0 : player 1
  // clientIndex == 1 : player 2
  short clientIndex = (client == game->clients[1]) ? 1 : 0;
  if (game->halfAwaleBoards[clientIndex][holeIndex] == 0)
  {
    send_message_to_specific_client(client, "The hole is empty... try again.",
                                    1);
    return;
  }
  int nbSeedsOnChooseHole = game->halfAwaleBoards[clientIndex][holeIndex];
  // add seeds to concerned holes
  game->halfAwaleBoards[clientIndex][holeIndex] = 0;
  for (int i = 1; i <= nbSeedsOnChooseHole; i++)
  {
    short currentBoardIndex =
        ((holeIndex + i) / HALF_AWALE_BOARD_SIZE + clientIndex) % 2;
    short currentHoleIndex = (holeIndex + i) % HALF_AWALE_BOARD_SIZE;

    // increment hole
    int *currentHole =
        &(game->halfAwaleBoards[currentBoardIndex][currentHoleIndex]);
    (*currentHole)++;

    // Check if we are in the opponent's board
    if (currentBoardIndex != clientIndex)
    {
      // Take the seeds if we can
      if (*currentHole == 2 || *currentHole == 3)
      {
        game->capturedSeeds[clientIndex] += *currentHole;
        *currentHole = 0;
      }
    }
  }

  // Check if one player's board is empty
  char isEmpty[2] = {1, 1};
  for (int i = 0; i < HALF_AWALE_BOARD_SIZE; i++)
  {

    isEmpty[0] &= game->halfAwaleBoards[0][i] == 0;
    isEmpty[1] &= game->halfAwaleBoards[1][i] == 0;
  }

  // if either side is empty or someone has 25 points
  char isWon = 0;
  for (int i = 0; i < 2; i++)
    isWon |= (isEmpty[i] | (game->capturedSeeds[i] >= 25));

  if (isWon)
  {
    short winnerIndex = game->capturedSeeds[1] > game->capturedSeeds[0];
    send_end_of_turn_message(game, ENDGAME);
    send_end_of_turn_message_to_watchers(game, ENDGAME);

    send_message_to_specific_client(
        game->clients[winnerIndex],
        "Félications ! Vous avez battu à plattes coutures votre adversaire !f",
        1);
    increment_user_win_count(game->clients[winnerIndex]);
    send_message_to_specific_client(game->clients[(winnerIndex + 1) % 2],
                                    "Dommage... vous avez perdu...", 1);

    free(game);
  }
  // send the new version to both players
  else
  {
    game->currentPlayer = game->clients[(clientIndex + 1) % 2];
    send_end_of_turn_message(game, NORMAL);
    send_end_of_turn_message_to_watchers(game, NORMAL);
  }
}

// ---------------------------------------------------------------- //
 void forfeit(Client *client)
{
  if (client->game == NULL)
  {
    send_message_to_specific_client(client, "You are currently not in a game.",
                                    1);
  }
  else
  {
    Game *currentGame = client->game;
    Client *opponent = currentGame->clients[0] != client
                           ? currentGame->clients[0]
                           : currentGame->clients[1];
    char message[2048];
    snprintf(message, sizeof(message), "The player %s has forfeited. You won !",
             client->name);

    send_message_to_specific_client(opponent, message, 1);

    send_message_to_specific_client(client, "You forfeited ...", 1);

    increment_user_win_count(opponent);

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

// ----------------------------------------------------------------------------------//
void send_end_of_turn_message(Game *game, EndOfTurnMessageMode mode)
{
  char message[2048];
  char buffer[2048];

  for (int i = 0; i < 2; i++)
  {
    Client *client = game->clients[i];
    Client *opponent = game->clients[(i + 1) % 2];

    // Construction du message principal
    if (mode == START)
    {
      snprintf(message, sizeof(message), "C'est le tour de : %s\n\n",
               game->currentPlayer->name);
    }
    else
    {
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
    for (int j = HALF_AWALE_BOARD_SIZE - 1; j >= 0; j--)
    {
      snprintf(buffer, sizeof(buffer), "| %2d ", game->halfAwaleBoards[top][j]);
      strncat(message, buffer, sizeof(message) - strlen(message) - 1);
    }
    strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

    // Ligne du bas (joueur)
    strncat(message, "  ", sizeof(message) - strlen(message) - 1);
    for (int j = 0; j < HALF_AWALE_BOARD_SIZE; j++)
    {
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
    send_message_to_specific_client(client, message, 1);
  }
}

// ---------------------------------------------------------------- //
 void deny(Client *client)
{
  if (!client)
    return;

  if (client->challenger == NULL)
  {
    send_message_to_specific_client(client, "No one has challenged you yet.",
                                    1);
  }
  else
  {
    char message[2048];
    snprintf(message, sizeof(message),
             "The player %s has denied your challenge.", client->name);
    send_message_to_specific_client(client->challenger, message, 1);

    snprintf(message, sizeof(message), "You denied %s's challenge.",
             client->challenger->name);
    send_message_to_specific_client(client, message, 1);
    client->challenger->challenged = NULL;
    client->challenger = NULL;
  }
}


// ---------------------------------------------------------------- //
// Creates a challenge from the client to the challenged client and sends it to
// the challenged client
 void create_challenge(Client *client, Client **clients,
                             ParsedMessage *props)
{
  if (!client || !clients)
  {
    return;
  }

  if (strcmp(client->name, props->argv[0]) == 0)
  {
    send_message_to_specific_client(client, "You cannot challenge yourself...",
                                    1);
    return;
  }
  if (client->game == NULL)
  {
    int clientFound = 0;
    for (int j = 0; j < MAX_CLIENTS; ++j)
    {
      if (clients[j] != NULL)
      {
        if (strcmp(clients[j]->name, props->argv[0]) == 0)
        {
          // the challenged client is found
          clientFound = 1;
          client->challenged = clients[j];
          if (clients[j]->challenged == client)
          {
            // launch a game
            Game *game = (Game *)malloc(sizeof(Game));
            if (!game)
            {
              return;
            }
            game->capturedSeeds[0] = 0;
            game->capturedSeeds[1] = 0;
            game->clients[0] = client;
            game->clients[1] = clients[j];
            game->nbWatchers = 0;

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
              game->watchers[i] = NULL;
            }

            // Random choice of the first player
            srand(time(NULL));
            int starterIndex = rand() % 2;
            game->currentPlayer = game->clients[starterIndex];
            for (int k = 0; k < HALF_AWALE_BOARD_SIZE; k++)
            {
              game->halfAwaleBoards[0][k] = 4;
              game->halfAwaleBoards[1][k] = 4;
            }

            clients[j]->game = client->game = game;

            // Notify the players that the game is starting
            char message[2048];
            snprintf(message, sizeof(message),
                     "The awale game versus %s has started.", clients[j]->name);

            send_message_to_specific_client(client, message, 1);

            snprintf(message, sizeof(message),
                     "The awale game versus %s has started...", client->name);

            send_message_to_specific_client(clients[j], message, 1);
            send_end_of_turn_message(game, START);
            send_end_of_turn_message_to_watchers(game, START);
            return;
          }
          else
          {
            clients[j]->challenger = client;
            char message[2048];
            snprintf(message, sizeof(message), "Challenge sent to player %s.",
                     props->argv[0]);

            send_message_to_specific_client(client, message, 1);

            snprintf(message, sizeof(message),
                     "Challenge received from player %s.", client->name);

            send_message_to_specific_client(clients[j], message, 1);
            return;
          }
        }
      }
    }
    if (!clientFound)
    {
      char message[2048];
      snprintf(message, sizeof(message), "The player %s is not connected.",
               props->argv[0]);
      send_message_to_specific_client(client, message, 1);
    }
  }
  else
  {
    send_message_to_specific_client(client, "You need to finish your actual game before starting another one !",
                                    1);
  }
}


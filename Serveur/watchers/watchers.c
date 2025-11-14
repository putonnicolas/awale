#include "watchers.h"

// ---------------------------------------------------- //
 void watch(Client *client, Client **clients, ParsedMessage *props)
{
  short clientFound;
  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    if (clients[i] != NULL)
    {
      if (strcmp(clients[i]->name, props->argv[0]) == 0)
      {
        if (clients[i]->game != NULL)
        {
          // the client to watch is found
          clientFound = 1;
          clients[i]->game->watchers[clients[i]->game->nbWatchers] = client;
          clients[i]->game->nbWatchers++;
          client->gameToWatch = clients[i]->game;
        }
      }
    }
  }

  if (!clientFound)
  {
    char message[2048];
    snprintf(message, sizeof(message), "The player %s is not in game.",
             props->argv[0]);

    send_message_to_specific_client(client, message, 1);
  }
}

// ---------------------------------------------------- //
 void remove_watcher(Game *game, int index)
{
  if (index < 0 || index >= game->nbWatchers)
    return;

  memmove(game->watchers + index, game->watchers + index + 1,
          (game->nbWatchers - index - 1) * sizeof(Client *));
  game->nbWatchers--;
}

// ---------------------------------------------------- //
 void remove_specific_watcher(Client *client)
{
  if (!client)
    return;

  Game *g = client->gameToWatch;
  if (g == NULL)
    return;

  if (g->nbWatchers < 0 || g->nbWatchers > MAX_CLIENTS)
  {
    printf("nbWatchers corrompu = %d\n", g->nbWatchers);
    return;
  }

  for (int i = 0; i < g->nbWatchers; i++)
  {
    if (g->watchers[i] == NULL)
    {
      printf("watchers[%d] == NULL, continue\n", i);
      continue;
    }

    if (g->watchers[i] == client)
    {
      remove_watcher(g, i);
      return;
    }
  }

  printf("watcher pas trouvé pour client %s\n", client->name);
}

// ---------------------------------------------------- //
 void stopwatch(Client *client)
{
  if (client->gameToWatch == NULL)
  {
    send_message_to_specific_client(
        client, "You are currently not watching a game.\n", 1);
    return;
  }

  Game *game = client->gameToWatch;

  remove_specific_watcher(client);
  client->gameToWatch = NULL;

  char message[2048];
  snprintf(
      message, sizeof(message), "You stopped watching the game of %s and %s.\n",
      game->clients[0]->name,
      game->clients[1] ? game->clients[1]->name : "(waiting for opponent)");

  send_message_to_specific_client(client, message, 1);
}

// ----------------------------------------------------------------------------------//
 void send_end_of_turn_message_to_watchers(Game *game,
                                                 EndOfTurnMessageMode mode)
{
  if (game == NULL || game->nbWatchers == 0)
    return;

  char message[2048];
  char buffer[2048];

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

  strncat(message, "Plateau actuel :\n\n",
          sizeof(message) - strlen(message) - 1);

  // Top tab (client2)
  strncat(message, "  ", sizeof(message) - strlen(message) - 1);
  for (int j = HALF_AWALE_BOARD_SIZE - 1; j >= 0; j--)
  {
    snprintf(buffer, sizeof(buffer), "| %2d ", game->halfAwaleBoards[1][j]);
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);
  }
  strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

  // Bottom tab (client1)
  strncat(message, "  ", sizeof(message) - strlen(message) - 1);
  for (int j = 0; j < HALF_AWALE_BOARD_SIZE; j++)
  {
    snprintf(buffer, sizeof(buffer), "| %2d ", game->halfAwaleBoards[0][j]);
    strncat(message, buffer, sizeof(message) - strlen(message) - 1);
  }
  strncat(message, "|\n", sizeof(message) - strlen(message) - 1);

  // Captured seeds
  snprintf(buffer, sizeof(buffer), "\nGraines capturées :\n");
  strncat(message, buffer, sizeof(message) - strlen(message) - 1);

  snprintf(buffer, sizeof(buffer), "  %s : %d\n", game->clients[0]->name,
           game->capturedSeeds[0]);
  strncat(message, buffer, sizeof(message) - strlen(message) - 1);

  snprintf(buffer, sizeof(buffer), "  %s : %d\n", game->clients[1]->name,
           game->capturedSeeds[1]);
  strncat(message, buffer, sizeof(message) - strlen(message) - 1);

  strncat(message, "\n========================\n",
          sizeof(message) - strlen(message) - 1);

  // Send to all spectateurs
  for (int i = 0; i < game->nbWatchers; i++)
  {
    Client *watcher = game->watchers[i];
    if (watcher != NULL && watcher->sock != INVALID_SOCKET)
    {
      send_message_to_specific_client(watcher, message, 1);
    }
  }
}
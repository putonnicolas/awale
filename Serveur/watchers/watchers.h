#ifndef WATCHERS_H
#define WATCHERS_H

#include "../clients/client.h"
#include "../game/game.h"
#include "../parsing/parsing.h"

void watch(Client *client, Client **clients, ParsedMessage *props);
void stopwatch(Client *client);
void remove_watcher(Game *game, int index);
void remove_specific_watcher(Client *client);
void send_end_of_turn_message_to_watchers(Game *game, EndOfTurnMessageMode mode);

#endif

#ifndef GAME_H
#define GAME_H

#include "../utils/config.h"
#include "../clients/client.h"
#include "../parsing/parsing.h"
#include "../watchers/watchers.h"

void create_challenge(Client *client, Client **clients, ParsedMessage *props);
void deny(Client *client);
void forfeit(Client *client);
void play_awale(Client *client, ParsedMessage *props);
void send_end_of_turn_message(Game *game, EndOfTurnMessageMode modes);

#endif

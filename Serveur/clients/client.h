#ifndef CLIENTS_H
#define CLIENTS_H

#include "../utils/config.h"
#include "../game/game.h"
#include "../parsing/parsing.h"
#include <stdio.h>

typedef struct Client Client;

void send_message_to_all_clients(Client **clients, Client client,
                                 int actual, const char *buffer, char from_server);
void send_message_to_specific_client(Client *client, const char *buffer, char from_server);
void remove_client(Client **clients, int toRemove, int *actual);
void clear_clients(Client **clients, int actual);
char check_existing_user(Client *client);
void increment_user_win_count(Client *client);
void update_user_bio(Client *client, char *newBio);
void load_user_data(Client *client);
void chat(Client *client, Client **otherClients, ParsedMessage *props);
#endif

#ifndef SERVER_H
#define SERVER_H

#include "./utils/config.h"
#include "./utils/utils.h"
#include "./clients/client.h"
#include "./parsing/parsing.h"
#include "./watchers/watchers.h"
#include "./game/game.h"
#include <stdio.h>

void init(void);
void end(void);
void app(void);
int init_connection(void);
void end_connection(int sock);
int read_client(SOCKET sock, char *buffer);
void write_client(SOCKET sock, const char *buffer);
void list(Client *client, Client **clients, int nbClients, char showBio);

#endif

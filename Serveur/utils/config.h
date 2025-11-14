#ifndef CONFIG_H
#define CONFIG_H
#define HALF_AWALE_BOARD_SIZE 6

#ifdef WIN32

#include <winsock2.h>

#elif defined(linux)

#include <arpa/inet.h>
#include <netdb.h> /* gethostbyname */
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> /* close */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100

#define BUF_SIZE 1024

#define USER_DB_FILE "users.db"

typedef struct Client Client;
typedef struct Game Game;
typedef struct ParsedMessage ParsedMessage;
typedef enum { NORMAL, START, ENDGAME } EndOfTurnMessageMode;

struct Client {
  SOCKET sock;
  char name[BUF_SIZE];
  unsigned int wins;
  char bio[1024];
  Client *challenged;
  Client *challenger;
  Game *game;
  Game *gameToWatch;
};


struct ParsedMessage {
  char *command;
  char **argv;
  int argc;
};


struct Game {
  ; /*
    Client1 is always the challenger.
    null : not in fight at all,
    not null and client2 = null : not in fight, challenger waiting for fight
    not null and client2 != null : in fight with challenger
    */
  Client *clients[2];
  Client *currentPlayer;
  Client *watchers[MAX_CLIENTS];
  int nbWatchers;
  int halfAwaleBoards[2][HALF_AWALE_BOARD_SIZE];
  int capturedSeeds[2];
};

#endif
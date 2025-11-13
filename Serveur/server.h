#ifndef SERVER_H
#define SERVER_H
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

struct ParsedMessage {
  char *command;
  char **argv;
  int argc;
};

typedef enum { NORMAL, START, ENDGAME } EndOfTurnMessageMode;

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client **clients, Client client,
                                        int actual, const char *buffer,
                                        char from_server);
static void send_message_to_specific_client(Client* client, const char *buffer,
                                            char from_server);
static void remove_client(Client **clients, int to_remove, int *actual);
static void clear_clients(Client **clients, int actual);
static void extract_props(const char *src, ParsedMessage *msg);
static void create_challenge(Client *client, Client **clients,
                             ParsedMessage *props);
static void deny(Client *client);
static void forfeit(Client *client);
static void chat(Client *client, Client **otherClients, int clientNb,
                 ParsedMessage *props);
static void play_awale(Client *client, ParsedMessage *props);
static void send_end_of_turn_message(Game *game, EndOfTurnMessageMode modes);
static void list(Client *client, Client **clients, int nbClients, char showBio);
static void watch(Client *client, Client **clients, ParsedMessage *props);
static void stopwatch(Client *client);
static void remove_watcher(Game *game, int index);
static void remove_specific_watcher(Client *client);
static void help(Client *client);
static void send_end_of_turn_message_to_watchers(Game *game,
                                           EndOfTurnMessageMode mode);
static char check_existing_user(Client *client);
static void increment_user_win_count(Client *client);
static void update_user_bio(Client *client, char *newBio);
static void load_user_data(Client *client);

#endif

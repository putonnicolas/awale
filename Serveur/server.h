#ifndef SERVER_H
#define SERVER_H
#define AWALE_BOARD_SIZE 12

#ifdef WIN32

#include <winsock2.h>

#elif defined(linux)

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
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

typedef struct Client Client; 
typedef struct Game Game; 
typedef struct ParsedMessage ParsedMessage;

struct Client {
    SOCKET sock;
    char name[BUF_SIZE];
    Client* challenged;
    Client* challenger;
    Game* game;
};

struct Game
{
  int capturedSeedClient1;
  Client *client1; /*
  Client1 is always the challenger.
  null : not in fight at all,
  not null and client2 = null : not in fight, challenger waiting for fight
  not null and client2 != null : in fight with challenger
  */
  Client *client2;
  Client* currentPlayer;
  int capturedSeedClient2;
  int awaleBoard[AWALE_BOARD_SIZE];
};

struct ParsedMessage
{
  char *command;
  char **argv;
  int argc;
};

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void send_message_to_specific_client(Client client, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static void extract_props(const char *src, ParsedMessage *msg);
static void create_challlenge(Client *client, Client *clients, ParsedMessage *props);
static void forfeit(Client *client);
static void play_awale(Client* client, ParsedMessage* props);


#endif /* guard */
/* Serveur sockets TCP
 * affichage de ce qui arrive sur la socket
 *    socket_server port (port > 1024 sauf root)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_USER 4
#define MAX_BUFFER_SIZE 256

struct Player
{
  int* tab;
  int capturedSeeds;
  Player* challenger; /*
  null : not in fight at all, 
  not null and challenger.challenger = null : not in fight, challenger waiting for fight
  not null and challenger.challenger != null : in fight with challenger    
*/
}

void extract_command(const char *src, char *cmd) {
  if (strlen(src) <= 0) return;

  int i = 0;
  do 
  { 
    char c = src[i];
    cmd[i] = c;
    i++;
  } while(c != ' ');

  cmd[i] = '\0';
}

int main(int argc, char** argv )
{ char datas[] = "hello\n";
  int    sockfd,newsockfd,clilen,chilpid,ok,nleft,nbwriten;
  char c;
  struct sockaddr_in cli_addr,serv_addr;
  
  if (argc!=2) {printf ("usage: socket_server port\n");exit(0);}
 
  printf ("Awale server starting...\n");  
  
  /* ouverture du socket */
  sockfd = socket (AF_INET,SOCK_STREAM,0);
  if (sockfd<0) {printf ("Impossible to open socket\n");
    exit(0);}

  /* initialisation des parametres */
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family       = AF_INET;
  serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
  serv_addr.sin_port         = htons(atoi(argv[1]));

  /* effecture le bind */
  if (bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
     {printf ("Impossible to bind socket\n");exit(0);}

  /* petit initialisation */
  listen(sockfd,MAX_USER);
     
  /* attend la connection d'un client */
  clilen = sizeof (cli_addr);
  signal(SIGCHLD,SIG_IGN)
  while (1)
  {
    int scomm = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
    if (scomm<0 || ) {printf ("Accept error\n"); exit(0);}
    printf ("Connection accepted\n");

      int pid = fork();
      if (pid == 0) /* c’est le fils */
      {
        close(sockfd); /* socket inutile pour le fils */
        
        /* traiter la communication */
        while (1)
        { 
          char* buffer = malloc(MAX_BUFFER_SIZE * sizeof(char));
          int i = 0;

          // fill the buffer
          while (read(scomm,&c,1) <= 0)
          {
            buffer[i] = c;
            ++i;
          }

          char* command;
          extract_command(buffer, command);
          if(command != NULL)
          {
            if(strcmp(command, "challenge"))
            {
  
            }
            else if(strcmp(command, "play"))
            {
              
            }
            else if(strcmp(command, "ff"))
            {
              
            }
            else
            {
              // invalid command
            }

          }
          break;
          printf("%d %c",pid, c); 
        }
        close(scomm);
        exit(0); /* on force la terminaison du fils */
      }
      else /* c’est le pere */
      {
        close(scomm); /* socket inutile pour le pere */
      }
  }

   return 1;
 }

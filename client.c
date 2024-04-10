#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

typedef int bool;
#define true 1
#define false 0

#define BUF_SIZE 1024

void erro(char *msg);
int serverConnection(int argc, char *argv[]);
void sendString(int fd, char *msg);
char *receiveString(int fd);

int main(int argc, char *argv[]){
  printf("\e[1;1H\e[2J"); 
  int fd = serverConnection(argc, argv);
  char *msgReceived = NULL;
  char msgToSend[BUF_SIZE];

  sleep(3);
  printf("\e[1;1H\e[2J"); 

  while(msgReceived == NULL || strcmp(msgReceived, "\nUntil next time! Thanks for chattingRC with us :)\n") != 0){
    free(msgReceived);
    msgReceived = receiveString(fd);
    if(strcmp(msgReceived, "\nUntil next time! Thanks for chattingRC with us :)\n")==0)break;
    fgets(msgToSend, sizeof(msgToSend), stdin);
    sendString(fd, msgToSend); 
    fflush(stdout);
  }
  sleep(3);
  printf("\e[1;1H\e[2J"); 
  close(fd);
  exit(0);
}

int serverConnection(int argc, char *argv[]){
  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  if (argc != 3){
    printf("client <host> <port>\n");
    fflush(stdout);
    exit(-1);
  }
  if ((hostPtr = gethostbyname(argv[1])) == 0)
    erro("Nao consegui obter endereço");

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short)atoi(argv[2]));

  bool connection_success = true;
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    erro("Socket");
    connection_success = false;
  }
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    erro("Connect");
    connection_success = false;
  }

  if (connection_success) {
    printf("Connected with server\n\n");
    fflush(stdout);
  }

  return fd;
}

void sendString(int fd, char *msg){
  write(fd, msg, 1 + strlen(msg));
}

char *receiveString(int fd){
  int nread = 0;
  char buffer[BUF_SIZE];
  char *string;

  nread = read(fd, buffer, BUF_SIZE - 1);
  buffer[nread] = '\0';
  string = (char *)malloc(strlen(buffer) + 1);
  strcpy(string, buffer);
  if(string[0]=='\n')printf("\e[1;1H\e[2J");
  printf("%s", string);
  fflush(stdout);
  return string;
}

void erro(char *msg){
  printf("Erro: %s\n", msg);
  fflush(stdout);
  exit(-1);
}

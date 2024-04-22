#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <ctype.h>

typedef int bool;
#define true 1
#define false 0

#define BUF_SIZE 1024

typedef struct {
    char username[20];
    int port;
} User;

void erro(char *msg);
int serverConnection(int argc, char *argv[]);
void connectingToClientServer(const User user, const User conversa);
void createNewServer(const User user);
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
    
    // Make the client a server
    User user;
    const char* compareString = "Creating a new server:";
    size_t compareLength = strlen(compareString);
    if(strncmp(msgReceived, compareString, compareLength) == 0){
      sleep(2);
      printf("\e[1;1H\e[2J"); 
      if (sscanf(msgReceived, "Creating a new server: %s %d", user.username, &user.port) == 2){
        printf("username: %s\n", user.username);
        createNewServer(user);

        sendString(fd, "Created server successfully\n");
        continue;
      }
    }

    const char* compareString2 = "Start a conversation:";
    size_t compareLength2 = strlen(compareString2);
    if(strncmp(msgReceived, compareString2, compareLength2) == 0){
      printf("\n\nConnecting with another user\n");
      sleep(2);
      printf("\e[1;1H\e[2J");
      User conversa;
      if (sscanf(msgReceived, "Start a conversation: %s -> %s %d", user.username, conversa.username, &conversa.port) == 3){
        printf("username: %s\n", user.username);
        // Connect to server
        connectingToClientServer(user, conversa);
        sendString(fd, "Connected successfully\n");
        continue;
      }
    }
    
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

void connectingToClientServer(const User user, const User conversa){
  //printf("\e[1;1H\e[2J");
  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  hostPtr = gethostbyname("127.0.0.1");

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons(conversa.port);

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
    //sleep(1);
    //printf("\e[1;1H\e[2J");
    
    char aux[100];
    sprintf(aux, "Chatting with %s in port %d\n\n", user.username, conversa.port);
    sendString(fd, aux);
    
    receiveString(fd);
    
    char *msgReceived = NULL;
    char msgToSend[BUF_SIZE-22];
    while(1){
      printf("%s: ", user.username);
      fgets(msgToSend, sizeof(msgToSend), stdin);
      
      if(strcmp(msgToSend, "\n")==0){
        sendString(fd, "Disconnected\n");
        break;
      }

      char formattedMsg[BUF_SIZE];
      sprintf(formattedMsg, "%s: %s", user.username, msgToSend);
      sendString(fd, formattedMsg); 
      fflush(stdout);
      
      free(msgReceived);
      msgReceived = receiveString(fd);

      if(strcmp(msgReceived, "Disconnected\n")==0){ 
        printf("Recebi aqui o disconnected\n");
        sendString(fd, "Disconnected\n");
        break;
      }

    }
    fflush(stdout);
  }
}

void createNewServer(const User user){
  //printf("\e[1;1H\e[2J"); 
  int fd, client;
  struct sockaddr_in addr, client_addr;
  int client_addr_size;

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(user.port);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    erro("na funcao socket");
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    erro("na funcao bind");
  if (listen(fd, 5) < 0)
    erro("na funcao listen");

  client_addr_size = sizeof(client_addr);

  // clean finished child processes, avoiding zombies
  // must use WNOHANG or would block whenever a child process was working
  while (waitpid(-1, NULL, WNOHANG) > 0);
  // wait for new connection
  client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
  if (client > 0){
      close(fd);
      printf("Connected with another user\n");
      //sleep(1);
      //printf("\e[1;1H\e[2J"); 
      fflush(stdout);
      // Menu da conversa
      User conversa;
      
      char aux_string[BUF_SIZE];
      strcpy(aux_string, receiveString(client));
      const char* compareString = "Chatting with";
      size_t compareLength = strlen(compareString);
      if(strncmp(aux_string, compareString, compareLength) == 0){
        sscanf(aux_string, "Chatting with %s in port %d\n\n!", conversa.username, &conversa.port);
      }

      char aux[100];
      sprintf(aux, "Chatting with %s in port %d\n\n", user.username, user.port);
      sendString(client, aux);
      
      char *msgReceived = NULL;
      char msgToSend[BUF_SIZE-22];
      while(1){
        free(msgReceived);
        msgReceived = receiveString(client);

        if(strcmp(msgReceived, "Disconnected\n")==0){
          sleep(1); 
          close(client);
          return;
        }

        printf("%s: ", user.username);
        fgets(msgToSend, sizeof(msgToSend), stdin);
        if(strcmp(msgToSend, "\n")==0){ 
          sendString(fd, "Disconnected\n");
          free(msgReceived);
          msgReceived = receiveString(client);
          if(strcmp(msgReceived, "Disconnected\n")==0){ 
            printf("FOI AQUI\n");
            sleep(1);
            close(client);
            return;
          }
        }

        char formattedMsg[BUF_SIZE];
        sprintf(formattedMsg, "%s: %s", user.username, msgToSend);
        sendString(client, formattedMsg); 

        

        fflush(stdout);
      }
    close(client);
  }

  return ;
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

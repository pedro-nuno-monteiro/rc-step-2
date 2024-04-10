#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024
#define MAX_USERS 20

typedef int bool;
#define true 1
#define false 0

typedef struct {
    char username[20];
    char password[20];
} User;

void erro(char *msg);
int serverSetup();

void sendString(int client_fd, char *msg);
char *receiveString(int client_fd);

void mainMenu(int client_fd);
void signupMenu(int client_fd);
void loginMenu(int client_fd);
void conversationsMenu(int client_fd, const User user);

void create_user(char *username, char *password);
bool checkDuplicateUsername(char *username);
bool checkCredentials(char *username, char *password);


int main(){
  printf("\e[1;1H\e[2J");
  printf("ChatRC Main Server\n");
  int fd, client;
  struct sockaddr_in addr, client_addr;
  int client_addr_size;

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(SERVER_PORT);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    erro("na funcao socket");
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    erro("na funcao bind");
  if (listen(fd, 5) < 0)
    erro("na funcao listen");

  client_addr_size = sizeof(client_addr);

  while (1){
    // clean finished child processes, avoiding zombies
    // must use WNOHANG or would block whenever a child process was working
    while (waitpid(-1, NULL, WNOHANG) > 0);
    // wait for new connection
    client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
    if (client > 0){
      if (fork() == 0){
        close(fd);
        printf("Connected with a client\n");
        fflush(stdout);
        mainMenu(client);
        exit(0);
      }
      close(client);
    }
  }
  return 0;
}

void sendString(int client_fd, char *msg){
  write(client_fd, msg, 1 + strlen(msg));
}

char *receiveString(int client_fd){
  int nread = 0;
  char buffer[BUF_SIZE];
  char *string;

  nread = read(client_fd, buffer, BUF_SIZE - 1);
  buffer[nread] = '\0';
  string = (char *)malloc(strlen(buffer) + 1);
  strcpy(string, buffer);
  fflush(stdout);

  printf("Received from client %d: %s", getpid(), string);

  return string;
}

void mainMenu(int client_fd){
  int selection = -1;
  while(selection != 3){
    sendString(client_fd, "\nWELCOME TO chatRC\n\n1. Sign Up\n2. Log in\n3. Exit\n\nSelection: ");
    selection = atoi(receiveString(client_fd));
    if (selection == 1 || selection == 2 || selection == 3){
      if (selection == 1){
        signupMenu(client_fd);
        selection = -1;
        continue;
      }
      if (selection == 2){
        loginMenu(client_fd);
        selection = -1;
        continue;
      }
      if (selection == 3){
        sendString(client_fd, "\nUntil next time! Thanks for chattingRC with us :)\n");
      }
    }
    else {
      sendString(client_fd, "\nINVALID SELECTION, please press 1, 2 or 3! (Press ENTER to continue...)\n");
      receiveString(client_fd);
    }
  }
}

void signupMenu(int client_fd){
  bool leave_menu = false;
  while(!leave_menu){
    char username[50]; 
    sendString(client_fd, "\nSign up for chatRC (Press ENTER to return)\n\nusername: ");
    strcpy(username, receiveString(client_fd));
    if (strcmp(username, "\n") != 0) {
      if (checkDuplicateUsername(username)){
        sendString(client_fd, "\nusername already exists (Press ENTER to continue...)\n");
        receiveString(client_fd);
      }
      else{
        char password[50]; 
        sendString(client_fd, "password: ");
        strcpy(password, receiveString(client_fd));
        if (strcmp(password, "\n") == 0){
          leave_menu = false;
          break;
        }
        else{
          create_user(username, password);
          sendString(client_fd, "\nUser created successfully!!! (Press ENTER to continue...)\n");
          receiveString(client_fd);
          leave_menu = true;
        }
      }
    }
    else{
      leave_menu = true;
    }
  }
}
void loginMenu(int client_fd){
  bool leave_menu = false;
  while(!leave_menu){
    char username[50]; 
    sendString(client_fd, "\nLog in chatRC (Press ENTER to return)\n\nusername: ");
    strcpy(username, receiveString(client_fd));
    if (strcmp(username, "\n") != 0) {
      char password[50]; 
      sendString(client_fd, "password: ");
      strcpy(password, receiveString(client_fd));
      if (strcmp(password, "\n") == 0){
        leave_menu = true;
      }
      else{
        if(checkCredentials(username, password)){
          sendString(client_fd, "\nLogged in successfully!!! (Press ENTER to continue...)\n");
          receiveString(client_fd);
          User user;
          strcpy(user.username, username);
          strcpy(user.password, password);
          conversationsMenu(client_fd, user);
          leave_menu = true;
        }
        else{
          sendString(client_fd, "\nWrong username or password (Press ENTER to continue...)\n");
          receiveString(client_fd);
        }
      }
    }
    else{ 
     leave_menu = true;
    }
  }
}

void conversationsMenu(int client_fd, const User user){
  int selection = -1;
  while(selection != 3){
    char string[100];
    snprintf(string, sizeof(string), "\nWELCOME %s\n1. Private\n2. Group\n3. Log out\n\nSelection: ", user.username);
    sendString(client_fd, string);
    selection = atoi(receiveString(client_fd));
    if (selection == 1 || selection == 2 || selection == 3){
      if (selection == 1){
        sendString(client_fd, "\nThis feature is currently under development. Stay tuned for updates! (Press ENTER to continue...)\n");
        receiveString(client_fd);
        selection = -1;
      }
      if (selection == 2){
        sendString(client_fd, "\nThis feature is currently under development. Stay tuned for updates! (Press ENTER to continue...)\n");
        receiveString(client_fd);
        selection = -1;
      }
      if (selection == 3){
        sendString(client_fd, "\nReturning to Main Menu! (Press ENTER to continue...)\n");
        receiveString(client_fd);
      }
    }
    else {
      sendString(client_fd, "\nINVALID SELECTION, please press 1, 2 or 3! (Press ENTER to continue...)\n");
      receiveString(client_fd);
    }
  }
}

bool checkDuplicateUsername(char *username) {
    FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        return false;
    }

    User users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    for (size_t i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

void create_user(char *username, char *password){
  FILE *file = fopen("users.bin", "ab");
    if (file == NULL) {
        file = fopen("users.bin", "wb");
        if (file == NULL) {
            erro("Creating file");
            exit(0);
        }
        fclose(file);
        printf("Created users.bin\n");
        fflush(stdout);
    }

    file = fopen("users.bin", "ab");
    if (file == NULL) {
      erro("Opening file");
      exit(0);
    }

    User user;
    strcpy(user.username, username);
    strcpy(user.password, password);

    fwrite(&user, sizeof(User), 1, file);

    fclose(file);
}

bool checkCredentials(char *username, char *password) {
    FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        return false;
    }

    User users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    for (size_t i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if(strcmp(users[i].password, password) == 0){
              return true;
            }
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return false;
}

void erro(char *msg){
  printf("Error: %s\n", msg);
  exit(-1);
}


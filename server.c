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
#define MAX_USERS 50

typedef int bool;
#define true 1
#define false 0

typedef struct {
    char username[20];
    char password[20];
    int port;
} User;

typedef struct {
  char username1[20];
  char username2[20];
  int port;
} Conversation;

void erro(char *msg);
// COMMUNICATION
void sendString(int client_fd, char *msg);
char *receiveString(int client_fd);
// MENUS
void mainMenu(int client_fd);
void signupMenu(int client_fd);
void loginMenu(int client_fd);
void conversationsMenu(int client_fd, const User user);
void privateCommunicationMenu(int client_fd, const User user);
User chooseAvailableUsers(int client_fd, const User user);
// USERS FILE
void createUsersFile();
void seeUsers();
void create_user(char *username, char *password);
bool checkDuplicateUsername(char *username);
int checkCredentials(char *username, char *password);
// CONVERSATIONS FILE
void createConversationsFileLog();
void addConversation(const char *username1, const char *username2, int port);
void deleteConversation(const char *username1, const char *username2);
bool checkExistingConversation(const char *username1, const char *username2);
User activeConversations(int client_fd, const User user);

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
      createUsersFile();
      createConversationsFileLog();
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

// COMMUNICATION

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

// MENUS

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
        seeUsers();
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
        int user_port = checkCredentials(username, password);
        if(user_port != -1){
          
          User user;
          strcpy(user.username, username);
          strcpy(user.password, password);
          user.port = user_port;
          
          sendString(client_fd, "\nLogged in successfully!!! (Press ENTER to continue...)\n");
          receiveString(client_fd);
          
          // enviar um processo para criar um server
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
    snprintf(string, sizeof(string), "\nWELCOME %.*s TO THE CONVERSATIONS MENU\n\n1. Private\n2. Group\n3. Log out\n\nSelection: ", (int)(strlen(user.username) - 1), user.username);
    sendString(client_fd, string);
    selection = atoi(receiveString(client_fd));
    if (selection == 1 || selection == 2 || selection == 3){
      if (selection == 1){
        privateCommunicationMenu(client_fd, user);
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

void privateCommunicationMenu(int client_fd, const User user){
  int selection = -1;
  while(selection != 4){
    char string[300];
    snprintf(string, sizeof(string), "\nWELCOME %.*s TO THE PRIVATE CONVERSATIONS MENU\n\n1. See ongoing conversations\n2. Start a new conversation\n3. Block users\n4. Exit Menu\n\nSelection: ", (int)(strlen(user.username) - 1), user.username);
    sendString(client_fd, string);
    selection = atoi(receiveString(client_fd));
    if (selection == 1 || selection == 2 || selection == 3 || selection == 4){
      if (selection == 1){ // Show active conversations
        //activeConversations
        User conversa = activeConversations(client_fd, user);
        if(strcmp(conversa.username, "") != 0){
          char string[100];
          snprintf(string, sizeof(string), "Start a conversation: %.*s -> %.*s %d\n", (int)(strlen(user.username) - 1), user.username, (int)(strlen(conversa.username) - 1), conversa.username, conversa.port);
          sendString(client_fd, string);
          receiveString(client_fd);
        }
        selection = -1;
      }
      if (selection == 2){ // Start a new conversation
        User conversa = chooseAvailableUsers(client_fd, user);
        if(strcmp(conversa.username, "") != 0 && strcmp(conversa.password, "") != 0){ // Se não retornar nulo
          if(checkExistingConversation(user.username, conversa.username)){ 
            // Enter conversation with existing person
            char string[100];
            snprintf(string, sizeof(string), "Start a conversation: %.*s -> %.*s %d\n", (int)(strlen(user.username) - 1), user.username, (int)(strlen(conversa.username) - 1), conversa.username, conversa.port);
            sendString(client_fd, string);
            receiveString(client_fd);
          }
          else {// Create a new server on the client side
            addConversation(user.username, conversa.username, user.port);
            char string[100];
            snprintf(string, sizeof(string), "Creating a new server: %.*s %d\n", (int)(strlen(user.username) - 1), user.username, user.port);
            sendString(client_fd, string);
            receiveString(client_fd);
            deleteConversation(user.username, conversa.username);
          }
        }
        selection = -1;
      }
      if (selection == 3){ // Block users -> será que não faz mais sentido ser no conversations menu?
        sendString(client_fd, "\nThis feature is currently under development. Stay tuned for updates! (Press ENTER to continue...)\n");
        receiveString(client_fd);
        selection = -1;
      }
      if (selection == 4){
        sendString(client_fd, "\nReturning to Conversations Menu! (Press ENTER to continue...)\n");
        receiveString(client_fd);
      }
    }
    else {
      sendString(client_fd, "\nINVALID SELECTION, please press 1, 2, 3 or 4! (Press ENTER to continue...)\n");
      receiveString(client_fd);
    }
  }
}

User chooseAvailableUsers(int client_fd, const User user){
  FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        User nulo = { "", "", 0 };
        return nulo;
    }

    User users[MAX_USERS];
    User available_users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    fclose(file);

    char choices_to_send[1000] = "";
    int counter = 0;
    char counter_str[5];

  	strcat(choices_to_send, "\nSTART A NEW CONVERSATION\n\n");
    for(size_t i = 0; i < num_users; i++){
      // Check if the user is logged in
      if(strcmp(users[i].username, user.username) != 0 /*&& user[i].logged_in && check if itself is not blocked by the user*/){ // If it isn't itself
        available_users[counter] = users[i];
        counter++;
        sprintf(counter_str, "%d", counter);
        strcat(choices_to_send, counter_str);
        strcat(choices_to_send, ". ");
        strcat(choices_to_send, users[i].username);
      }
    }

    counter++;
    sprintf(counter_str, "%d", counter);
    strcat(choices_to_send, counter_str);
    strcat(choices_to_send, ". ");
    strcat(choices_to_send, "Return to the PRIVATE CONVERSATIONS MENU\n\nSelection: ");
    // Send the string to the user
    // Receive the choice and return the user
    int selection = -1; 
    while(1){
      sendString(client_fd, choices_to_send);
      selection = atoi(receiveString(client_fd));

      if(selection == counter){ // Exit
        User nulo = { "", "", 0 }; 
        return nulo; // retorna um user nulo
      }
      else if(selection >= 0 && selection < counter){
        return available_users[selection - 1];
      }
    }

    User nulo = { "", "", 0 };
    return nulo;

}

// USERS FILE
void createUsersFile() {
    FILE *file = fopen("users.bin", "r");
    if (file == NULL) {
        file = fopen("users.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            exit(1);
        }

        User initial_user;
        strcpy(initial_user.username, "admin\n");
        strcpy(initial_user.password, "admin\n");
        initial_user.port = 9001;
        fwrite(&initial_user, sizeof(User), 1, file);

        fclose(file);
    } else {
        fclose(file);
    }
}

void seeUsers() {
    FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        return;
    }

    User users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    for (size_t i = 0; i < num_users; i++) {
        // Remove trailing newline character if it exists
        size_t username_len = strlen(users[i].username);
        size_t password_len = strlen(users[i].password);
        if (username_len > 0 && users[i].username[username_len - 1] == '\n') {
            users[i].username[username_len - 1] = '\0';
        }
        if (password_len > 0 && users[i].password[password_len - 1] == '\n') {
            users[i].password[password_len - 1] = '\0';
        }
        printf("User %ld -> %s, %s, %d\n", i, users[i].username, users[i].password, users[i].port);
    }

    fclose(file);
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
  FILE *file = fopen("users.bin", "ab+");

    if (file == NULL) {
      erro("Opening file");
      exit(0);
    }

    User existing_users[MAX_USERS];
    size_t num_users = fread(existing_users, sizeof(User), MAX_USERS, file);

    int highest_port = 0;
    for (size_t i = 0; i < num_users; i++) {
        if (existing_users[i].port > highest_port) {
            highest_port = existing_users[i].port;
        }
    }

    int new_port = highest_port + 1;

    User user;
    strcpy(user.username, username);
    strcpy(user.password, password);
    user.port = new_port;

    fwrite(&user, sizeof(User), 1, file);

    fclose(file);
}

int checkCredentials(char *username, char *password) {
    FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        return false;
    }

    User users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    for (size_t i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if(strcmp(users[i].password, password) == 0){
              return users[i].port;
            }
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return -1;
}

// CONVERSATIONS FILE

void createConversationsFileLog() {
    FILE *file = fopen("conversationsLog.bin", "r");
    if (file == NULL) {
        file = fopen("conversationsLog.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            exit(1);
        }

        fclose(file);
    } else {
        fclose(file);
    }
}

void addConversation(const char *username1, const char *username2, int port) {
    // Open the file in append mode
    FILE *file = fopen("conversationsLog.bin", "ab");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Create a Conversa struct with the provided usernames
    Conversation conversation;
    strcpy(conversation.username1, username1);
    strcpy(conversation.username2, username2);
    conversation.port = port;

    // Write the Conversa struct to the file
    fwrite(&conversation, sizeof(Conversation), 1, file);

    // Close the file
    fclose(file);

    printf("Conversation added successfully.\n");
}

void deleteConversation(const char *username1, const char *username2) {
    // Open the file in read mode
    FILE *file = fopen("conversationsLog.bin", "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Open a temporary file to write the updated contents
    FILE *tempFile = fopen("tempFile.bin", "wb");
    if (tempFile == NULL) {
        perror("Error creating temporary file");
        fclose(file);
        exit(1);
    }

    Conversation conversation;
    bool deleted = false;
    // Read conversations from the original file
    while (fread(&conversation, sizeof(Conversation), 1, file) == 1) {
        // Check if the conversation involves either of the specified usernames
        if ((strcmp(conversation.username1, username1) == 0 && strcmp(conversation.username2, username2) == 0) ||
            (strcmp(conversation.username1, username2) == 0 && strcmp(conversation.username2, username1) == 0)) {
            deleted = true;
        } else {
            // Write the conversation to the temporary file
            fwrite(&conversation, sizeof(Conversation), 1, tempFile);
        }
    }

    // Close both files
    fclose(file);
    fclose(tempFile);

    if (!deleted) {
        printf("Conversation not found.\n");
    } else {
        printf("Conversation deleted successfully.\n");
    }

    // Remove the original file
    remove("conversationsLog.bin");
    // Rename the temporary file to the original file
    rename("tempFile.bin", "conversationsLog.bin");
}

bool checkExistingConversation(const char *username1, const char *username2) {
    // Open the file for reading
    FILE *file = fopen("conversationsLog.bin", "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Create a Conversa struct to read data from the file
    Conversation conversation;

    // Read Conversa structs from the file and compare usernames
    while (fread(&conversation, sizeof(Conversation), 1, file) == 1) {
        // Check if the conversation exists with the provided usernames
        if ((strcmp(conversation.username1, username1) == 0 && strcmp(conversation.username2, username2) == 0) ||
            (strcmp(conversation.username1, username2) == 0 && strcmp(conversation.username2, username1) == 0)) {
            // Conversation found
            fclose(file);
            return true;
        }
    }

    // Conversation not found
    fclose(file);
    return false;
}

User activeConversations(int client_fd, const User user) {
    FILE *file = fopen("conversationsLog.bin", "rb");
    if (file == NULL) {
        User nulo = { "", "", 0 };
        return nulo;
    }

    User available_users[MAX_USERS];
    int counter = 0;
    char choices_to_send[1000] = "\nACTIVE CONVERSATIONS\n\n";
    char counter_str[5];

    Conversation conversations[MAX_USERS - 1];
    size_t num_conversations = fread(conversations, sizeof(Conversation), MAX_USERS - 1, file);

    fclose(file);

    for (size_t i = 0; i < num_conversations; i++) {
        if (strcmp(conversations[i].username1, user.username) == 0) {
            strcpy(available_users[counter].username, conversations[i].username2);
            available_users[counter].port = conversations[i].port;
            counter++;
            sprintf(counter_str, "%d", counter);
            strcat(choices_to_send, counter_str);
            strcat(choices_to_send, ". ");
            strcat(choices_to_send, conversations[i].username2);
        } else if (strcmp(conversations[i].username2, user.username) == 0) {
            strcpy(available_users[counter].username, conversations[i].username1);
            available_users[counter].port = conversations[i].port;
            counter++;
            sprintf(counter_str, "%d", counter);
            strcat(choices_to_send, counter_str);
            strcat(choices_to_send, ". ");
            strcat(choices_to_send, conversations[i].username1);
        }
    }

    // Add an option to return to the PRIVATE CONVERSATIONS MENU
    counter++;
    sprintf(counter_str, "%d", counter);
    strcat(choices_to_send, counter_str);
    strcat(choices_to_send, ". ");
    strcat(choices_to_send, "Return to the PRIVATE CONVERSATIONS MENU\n\nSelection: ");

    // Send the string to the user
    // Receive the choice and return the user
    int selection = -1;
    while (1) {
        sendString(client_fd, choices_to_send);
        selection = atoi(receiveString(client_fd));

        if (selection == counter) { // Exit
            User nulo = { "", "", 0 };
            return nulo;
        } else if (selection >= 0 && selection < counter) {
          return available_users[selection - 1];
        }
    }

    User nulo = { "", "", 0 };
    return nulo;
}



void erro(char *msg){
  printf("Error: %s\n", msg);
  exit(-1);
}


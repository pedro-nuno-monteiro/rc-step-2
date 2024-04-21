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
#define MAX_WORDS 50

typedef int bool;
#define true 1
#define false 0

bool firstConnection = false;

typedef struct {
    char username[20];
    char password[20];
    int port;
	bool logged_in;
} User;

typedef struct {
	char word[20];
} Word;

typedef struct {
    char blocker_username[20];
	char bloqueado_username[20];
} BlockedUser;

void erro(char *msg);

// COMMUNICATION
void sendString(int client_fd, char *msg);
char *receiveString(int client_fd);

// MENUS
void mainMenu(int client_fd);
void signupMenu(int client_fd);
void loginMenu(int client_fd);
void setAllUsersLoggedOut();
void logoutUser(User user);
void conversationsMenu(int client_fd, const User user, bool admin);
void privateCommunicationMenu(int client_fd, const User user);
User chooseAvailableUsers(int client_fd, const User user);
void startConversation(int client_fd, const User user, const User conversa);
void getOnlineUsers(int client_fd, const User user);
void updateUserStatus(const User user);
void filterWords(int client_fd);
void blockUsers(int client_fd, User user);
bool notBlocked(User user, User user_lista);

// FILES
void createUsersFile();
void seeUsers();
void seeWords(int client_fd);
void create_user(char *username, char *password);
void addWord (char *word);
void deleteWord(int client_fd);
bool checkDuplicateUsername(char *username);
int checkCredentials(char *username, char *password);
void createBlockUsersFile();

int main() {
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

	if (!firstConnection) {
		setAllUsersLoggedOut();
		printf("First log in\n");
		firstConnection = true;
    }

	while (1) {
		
		// clean finished child processes, avoiding zombies
		// must use WNOHANG or would block whenever a child process was working
		while (waitpid(-1, NULL, WNOHANG) > 0);
		
		// wait for new connection
		client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
		if (client > 0) {
			createUsersFile();
			createBlockUsersFile();

			if (fork() == 0) {
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

void sendString(int client_fd, char *msg) {
	write(client_fd, msg, 1 + strlen(msg));
}

char *receiveString(int client_fd) {
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

void mainMenu(int client_fd) {
	int selection = -1;
	while(selection != 3) {
		sendString(client_fd, "\nWELCOME TO chatRC\n\n1. Sign Up\n2. Log in\n3. Exit\n\nSelection: ");
		selection = atoi(receiveString(client_fd));
		if (selection == 1 || selection == 2 || selection == 3) {
			if (selection == 1) {
				signupMenu(client_fd);
				selection = -1;
				continue;
			}
			if (selection == 2) {
				seeUsers();
				loginMenu(client_fd);
				selection = -1;
				continue;
			}
			if (selection == 3) {
				sendString(client_fd, "\nUntil next time! Thanks for chattingRC with us :)\n");
			}
		}
		else {
			sendString(client_fd, "\nINVALID SELECTION, please press 1, 2 or 3! (Press ENTER to continue...)\n");
			receiveString(client_fd);
		}
	}
}

void signupMenu(int client_fd) {

	bool leave_menu = false;
	while(!leave_menu) {
		char username[50]; 
		sendString(client_fd, "\nSign up for chatRC (Press ENTER to return)\n\nusername: ");
		strcpy(username, receiveString(client_fd));

		if (strcmp(username, "\n") != 0) {
			if (checkDuplicateUsername(username)) {
				sendString(client_fd, "\nusername already exists (Press ENTER to continue...)\n");
				receiveString(client_fd);
			}
			else {
				char password[50]; 
				sendString(client_fd, "password: ");
				strcpy(password, receiveString(client_fd));
				
				if (strcmp(password, "\n") == 0) {
					leave_menu = false;
					break;
				}
				else {
					create_user(username, password);
					sendString(client_fd, "\nUser created successfully!!! (Press ENTER to continue...)\n");
					receiveString(client_fd);
					leave_menu = true;
				}
			}
		}
		else {
			leave_menu = true;
		}
	}
}

void loginMenu(int client_fd) {

	bool leave_menu = false;
	while(!leave_menu) {
		char username[50]; 
		sendString(client_fd, "\nLog in chatRC (Press ENTER to return)\n\nusername: ");
		strcpy(username, receiveString(client_fd));

		if (strcmp(username, "\n") != 0) {
			char password[50]; 
			sendString(client_fd, "password: ");
			strcpy(password, receiveString(client_fd));

			if (strcmp(password, "\n") == 0) {
				leave_menu = true;
			}
			else {
				int user_port = checkCredentials(username, password);
				
				if(user_port != -1) {
					sendString(client_fd, "\nLogged in successfully!!! (Press ENTER to continue...)\n");
					receiveString(client_fd);
					User user;
					strcpy(user.username, username);
					strcpy(user.password, password);
					user.port = user_port;
					bool admin_or_not = false;
					if(user.port == 9001){
						admin_or_not =true;
					}

					user.logged_in = true;
					updateUserStatus(user);
					conversationsMenu(client_fd, user, admin_or_not);
					leave_menu = true;
				}
				else {
					sendString(client_fd, "\nWrong username or password (Press ENTER to continue...)\n");
					receiveString(client_fd);
				}
			}
		}
		else { 
			leave_menu = true;
		}
  	}
}

void setAllUsersLoggedOut() {
	FILE *file = fopen("users.bin", "r+b");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    User users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    for (size_t i = 0; i < num_users; i++) {
        users[i].logged_in = false;
    }

    rewind(file);
    fwrite(users, sizeof(User), num_users, file);
    fclose(file);
}

void logoutUser(User user) {
	user.logged_in = false;
    updateUserStatus(user);
}

void updateUserStatus(const User user) {

	FILE *file = fopen("users.bin", "r+b");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    User users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

    for (size_t i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, user.username) == 0) {
            users[i].logged_in = user.logged_in;
            fseek(file, i * sizeof(User), SEEK_SET);
            fwrite(&users[i], sizeof(User), 1, file);
            break;
        }
    }

    fclose(file);
}

void conversationsMenu(int client_fd, const User user, bool admin) {

	int selection = -1;

	while(selection != 4) {
		char string[200]; 
		if (admin == false) {
			snprintf(string, sizeof(string), "\nWELCOME %.*s TO THE CONVERSATIONS MENU\n1. Private\n2. Group\n3. Block users\n4. Log out\n\nSelection: ", (int)(strlen(user.username) - 1), user.username);
		}
		else {
			snprintf(string, sizeof(string), "\nWELCOME %.*s TO THE CONVERSATIONS MENU\n1. Private\n2. Group\n3. Block users\n4. Log out\n5. Filter Words\n\nSelection: ", (int)(strlen(user.username) - 1), user.username);
		}
		
		sendString(client_fd, string);
		selection = atoi(receiveString(client_fd));

		if (selection == 1 || selection == 2 || selection == 3 || selection == 4 || (selection == 5 && admin)) {
			if (selection == 1) {
				privateCommunicationMenu(client_fd, user);
				selection = -1;
			}
			if (selection == 2) {
				sendString(client_fd, "\nThis feature is currently under development. Stay tuned for updates! (Press ENTER to continue...)\n");
				receiveString(client_fd);
				selection = -1;
			}
			if (selection == 3) {
				blockUsers(client_fd, user);
				selection = -1;
			}
			if (selection == 4) {
				logoutUser(user);
				sendString(client_fd, "\nReturning to Main Menu! (Press ENTER to continue...)\n");
				receiveString(client_fd);
			}

			if (selection == 5 && admin) {
				filterWords(client_fd);
				selection = -1;
			}
		}
		else {
			sendString(client_fd, "\nINVALID SELECTION, please press 1, 2 or 3! (Press ENTER to continue...)\n");
			receiveString(client_fd);
		}
	}
}

void privateCommunicationMenu(int client_fd, const User user) {
	int selection = -1;
	while(selection != 4) {
		char string[300];
		snprintf(string, sizeof(string), "\nWELCOME %.*s TO THE PRIVATE CONVERSATIONS MENU\n\n1. See ongoing conversations\n2. Start a new conversation\n3. See who's online\n4. Exit Menu\n\nSelection: ", (int)(strlen(user.username) - 1), user.username);
		sendString(client_fd, string);
		selection = atoi(receiveString(client_fd));

		if (selection == 1 || selection == 2 || selection == 3 || selection == 4) {
			if (selection == 1) { // Show ongoing conversations
				sendString(client_fd, "\nThis feature is currently under development. Stay tuned for updates! (Press ENTER to continue...)\n");
				receiveString(client_fd);
				selection = -1;
			}
			if (selection == 2) { // Start a new conversation
				User conversa = chooseAvailableUsers(client_fd, user);

				if(strcmp(conversa.username, "") != 0 && strcmp(conversa.password, "") != 0) { // Se não retornar nulo
					//Começa a conversa
					startConversation(client_fd, user, conversa);
				}
				selection = -1;
			}
			if (selection == 3) {
				getOnlineUsers(client_fd, user);
				selection = -1;
			}
			if (selection == 4) {
				sendString(client_fd, "\nReturning to Conversations Menu! (Press ENTER to continue...)\n");
				receiveString(client_fd);
			}
		}
		else {
			sendString(client_fd, "\nINVALID SELECTION, please press 1, 2, 3, 4 or 5 (Press ENTER to continue...)\n");
			receiveString(client_fd);
		}
	}
}

bool notBlocked(User user, User user_lista) {
	FILE *file_2 = fopen("block_users.bin", "rb");
    if (file_2 == NULL) {
		printf("erro no file Blocked");
    }

	BlockedUser users_bloqueados[MAX_USERS];
	size_t num_users_bloqueados = fread(users_bloqueados, sizeof(BlockedUser), MAX_USERS, file_2);

    fclose(file_2);

	for(size_t i = 0; i < num_users_bloqueados; i++) {
		if(strcmp(users_bloqueados[i].blocker_username, user.username) == 0) {
			if(strcmp(users_bloqueados[i].bloqueado_username, user_lista.username) == 0) {
				return false;	// user está blocked
			}
		}
	}
	return true;
}

void blockUsers(int client_fd, User user) {

	FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        return;
    }

    User users[MAX_USERS];
    User available_users[MAX_USERS];
    size_t num_users = fread(users, sizeof(User), MAX_USERS, file);

	FILE *file_2 = fopen("block_users.bin", "rb");
    if (file_2 == NULL) {
		return;
    }

	BlockedUser users_bloqueados[MAX_USERS];
	BlockedUser users_que_o_user_logado_bloqueou[MAX_USERS];
	size_t num_users_bloqueados = fread(users_bloqueados, sizeof(BlockedUser), MAX_USERS, file_2);

    fclose(file);
    fclose(file_2);

	// ler todos os users que o user logado bloqueou
	int counter_2 = 0;
	for(size_t i = 0; i < num_users_bloqueados; i++) {
		printf("blocker = %s", users_bloqueados[i].blocker_username);
		printf("bloqued = %s", users_bloqueados[i].bloqueado_username);
		if(strcmp(users_bloqueados[i].blocker_username, user.username) == 0) {
			users_que_o_user_logado_bloqueou[counter_2] = users_bloqueados[i];
			counter_2++;
		}
	}

    char choices_to_send[1000] = "";
    int counter = 0;
    char counter_str[5];
	strcat(choices_to_send, "\nLIST OF USERS\n\n");
    for(size_t i = 0; i < num_users; i++) {
     	if(strcmp(users[i].username, user.username) != 0) {
			available_users[counter] = users[i];
			printf("available_users[counter] = %s", users[i].username);
			counter++;
			sprintf(counter_str, "%d", counter);
			strcat(choices_to_send, counter_str);
			strcat(choices_to_send, ". ");

			size_t username_len = strlen(users[i].username);
			if (users[i].username[username_len - 1] == '\n') {
				users[i].username[username_len - 1] = '\0';
			}	
			strcat(choices_to_send, users[i].username);
			users[i].username[username_len - 1] = '\n';
			for(size_t j = 0; j < counter_2; j++) {
				if (strcmp(users[i].username, users_que_o_user_logado_bloqueou[j].bloqueado_username) == 0) {
					strcat(choices_to_send, " | Blocked");
					break;
				}
			}
			strcat(choices_to_send, "\n");
		}
    }

    counter++;
    sprintf(counter_str, "%d", counter);
    strcat(choices_to_send, counter_str);
    strcat(choices_to_send, ". ");
    strcat(choices_to_send, "Return to the previous menu\n\nSelection: ");

    int selection = -1;
	User user_a_bloquear;
    while(1) {
      	sendString(client_fd, choices_to_send);
		selection = atoi(receiveString(client_fd));

		if(selection == counter) {
			return; 
		}
		else if(selection >= 0 && selection < counter) {
			user_a_bloquear = available_users[selection - 1];
			
			bool already_blocked = false;
			for(size_t j = 0; j < counter_2; j++) {
				if (strcmp(user_a_bloquear.username, users_que_o_user_logado_bloqueou[j].bloqueado_username) == 0) {
					already_blocked = true;
					break;
				}
			}
			if (already_blocked) {
            	sendString(client_fd, "\nThis user is already blocked. Please select another user (press ENTER).\n\n");
				receiveString(client_fd);
				selection = -1;
				already_blocked = false;
			}
			else {
				sendString(client_fd, "\nUser blocked!\nReturning to Main Menu! (Press ENTER to continue...)\n");
				receiveString(client_fd);
				break;
			}
		}
	}

	BlockedUser blockedUser;

	strcpy(blockedUser.blocker_username, user.username);				// define quem bloqueia (o user logado)
	strcpy(blockedUser.bloqueado_username, user_a_bloquear.username);	// define quem é bloqueado (o user escolhido)

	FILE *file_3 = fopen("block_users.bin", "ab+");
    if (file_3 == NULL) {
		printf("erro no file");
		return;
    }

	fwrite(&blockedUser, sizeof(BlockedUser), 1, file_3);
	fclose(file_3);

	return;

}

void getOnlineUsers(int client_fd, const User user) {
	FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        return;
    }

	User users[MAX_USERS];
	User users_online[MAX_USERS];
	size_t num_users = fread(users, sizeof(User), MAX_USERS, file);
    fclose(file);

	char choices_to_send[1000] = "";
	strcat(choices_to_send, "\nONLINE USERS\n\n");
	
	int counter = 0;
	char counter_str[5];
	for(size_t i = 0; i < num_users; i++) {
		if(strcmp(users[i].username, user.username) != 0 && users[i].logged_in) {
			users_online[counter] = users[i];
			printf("users_online[counter] = %s", users[i].username);
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

	int selection = -1; 
    while(selection != counter) {
      	sendString(client_fd, choices_to_send);
		selection = atoi(receiveString(client_fd));
    }
	// faremos mais alguma coisa?
}

User chooseAvailableUsers(int client_fd, const User user) {
	
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
    for(size_t i = 0; i < num_users; i++) {
     	if(strcmp(users[i].username, user.username) != 0 && users[i].logged_in && notBlocked(user, users[i])) {
			available_users[counter] = users[i];
			printf("available_users[counter] = %s", users[i].username);
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
    while(1) {
      	sendString(client_fd, choices_to_send);
		selection = atoi(receiveString(client_fd));

		if(selection == counter) { // Exit
			User nulo = { "", "", 0 }; 
			return nulo; // retorna um user nulo
		}
		else if(selection >= 0 && selection < counter) {
			return available_users[selection - 1];
		}
    }

    User nulo = { "", "", 0 };
    return nulo;
}

void startConversation(int client_fd, const User user, const User conversa) {
   printf("\nA conversa vai começar com entre %.*s e %.*s no porto %d\n", (int)(strlen(user.username) - 1), user.username, (int)(strlen(conversa.username) - 1), conversa.username, user.port);
}

void filterWords(int client_fd) {

	int selection = -1;
	
	while(selection != 4) {
		char string[100]; 
		snprintf(string, sizeof(string), "\nWELCOME TO THE FILTER WORDS MENU\n1. See Words\n2. Add Word\n3. Delete Word\n4. Exit Menu\n\nSelection: ");

		sendString(client_fd, string);
		selection = atoi(receiveString(client_fd));

		if (selection == 1 || selection == 2 || selection == 3 || selection == 4) {
			
			if (selection == 1) {
				seeWords(client_fd);
				selection = -1;
			}

			if (selection == 2) {
				char word_add[50]; 
				sendString(client_fd, "\n(WORD TO ADD)\n\nWord: ");
				strcpy(word_add, receiveString(client_fd));
				addWord(word_add);
				selection = -1;
			}

			if (selection == 3) {
				deleteWord(client_fd);
				selection = -1;
			}

			if (selection == 4) {
				//sendString(client_fd, "\nReturning to Main Menu! (Press ENTER to continue...)\n");
				//receiveString(client_fd);
			}

		
		}

		else{
			sendString(client_fd, "\nINVALID SELECTION, please press 1, 2 or 3! (Press ENTER to continue...)\n");
			receiveString(client_fd);
		}
	}
}


// FILES

void seeWords(int client_fd) {

	FILE *file = fopen("words.bin", "rb");
	if (file == NULL) {
        return;
    }

	Word words[MAX_WORDS];
	size_t num_words = fread(words, sizeof(Word), MAX_WORDS, file);

	char menu_words_to_send[1000] = "";
	strcat(menu_words_to_send, "\nWORDS\n\n");

	if (num_words == 0) {
		strcat(menu_words_to_send, "\nNo words in the file\n\nPress ENTER to continue... ");
		sendString(client_fd, menu_words_to_send);
		receiveString(client_fd);
		fclose(file);
		return;
	}

	int counter = 0;
	char counter_str[5];

	for (size_t i = 0; i < num_words; i++) {
        size_t palavra = strlen(words[i].word);

        if (palavra > 0 && words[i].word[palavra - 1] == '\n') {
            words[i].word[palavra - 1] = '\0';
        }
		sprintf(counter_str, "%d", counter);
        strcat(menu_words_to_send, counter_str);
		strcat(menu_words_to_send, ". ");
		strcat(menu_words_to_send, words[i].word);
		strcat(menu_words_to_send, "\n");
		counter++;
    }

	strcat(menu_words_to_send, "Press ENTER to continue...");
	sendString(client_fd, menu_words_to_send);
    fclose(file);
}

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
		initial_user.logged_in = false;
        initial_user.port = 9001;
        fwrite(&initial_user, sizeof(User), 1, file);

        fclose(file);
    } else {
        fclose(file);
    }
}

void createBlockUsersFile() {
	FILE *file = fopen("block_users.bin", "r");
    if (file == NULL) {
        file = fopen("block_users.bin", "wb");
        if (file == NULL) {
            perror("Error creating file");
            exit(1);
        }

		BlockedUser user;
		strcpy(user.blocker_username, "teste\n");
		strcpy(user.bloqueado_username, "teste_2\n");

		fwrite(&user, sizeof(BlockedUser), 1, file);
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

// FICHEIROS

void addWord(char *word) {
	
	FILE *file = fopen("words.bin", "ab+");
	if (file == NULL) {
        return;
    }

	Word words[MAX_WORDS];
	size_t num_words = fread(words, sizeof(Word), MAX_WORDS, file);

	Word palavra;
	strcpy(palavra.word, word);
	fwrite(&palavra, sizeof(Word), 1, file);
	fclose(file);
}

void deleteWord(int client_fd) {

	FILE *file = fopen("words.bin", "rb+");
	if (file == NULL) {
        return;
    }

	int indice = 0;
	char menu_words_to_send[1000] = "";
	strcat(menu_words_to_send, "\n Word to Delete\n\n");

	Word words[MAX_WORDS];
	size_t num_words = fread(words, sizeof(Word), MAX_WORDS, file);

	int counter = 0;
	char counter_str[5];
	for (size_t i = 0; i < num_words; i++) {
        size_t palavra = strlen(words[i].word);

        if (palavra > 0 && words[i].word[palavra - 1] == '\n') {
            words[i].word[palavra - 1] = '\0';
        }
		sprintf(counter_str, "%d", counter);
        strcat(menu_words_to_send, counter_str);
		strcat(menu_words_to_send, ". ");
		strcat(menu_words_to_send, words[i].word);
		strcat(menu_words_to_send, "\n");
		counter++;
    }
	strcat(menu_words_to_send, "Selecione: ");

	sendString(client_fd, menu_words_to_send);
	indice = atoi(receiveString(client_fd));
	
	if (num_words == 0) {
		sendString(client_fd, "\nNão há palavras no ficheiro\n\nPress ENTER to continue... ");
		receiveString(client_fd);
        fclose(file);
        return;
    }

	if (indice < 0 || indice >= num_words) {
        sendString(client_fd, "\nIndice invalido\n\nPress ENTER to continue... ");
		receiveString(client_fd);
        fclose(file);
        return;
    }

	for (size_t i = indice; i < num_words - 1; ++i) {
        strcpy(words[i].word, words[i + 1].word);
    }
	num_words--;

	sendString(client_fd, "\n Word deleted \n\nPress ENTER to continue... ");
	receiveString(client_fd);

	fclose(file);

	file = fopen("words.bin", "wb");
	fwrite(words, sizeof(Word), num_words, file);
	fclose(file);
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

void erro(char *msg) {
	printf("Error: %s\n", msg);
	exit(-1);
}
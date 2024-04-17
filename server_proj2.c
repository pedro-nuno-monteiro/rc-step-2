/***********************************
Server RC 2023-2024
************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SERVER_PORT 	9000
#define BUF_SIZE	1024
#define MAX_IP_LENGTH 	16 // Including null terminator

struct client_info {
	int socket_id;
	char ip_address[MAX_IP_LENGTH];
};

void process_client(int client_fd, int sem_id, char ip_addr[]);
void erro(char *msg);
void print_clients();


int main() {

	int fd, client;
  	struct sockaddr_in addr, client_addr;
	int client_addr_size;
	char ip_addr[MAX_IP_LENGTH];

  	bzero((void *) &addr, sizeof(addr));
  	addr.sin_family = AF_INET;
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	addr.sin_port = htons(SERVER_PORT);

  	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		erro("ERROR: function socket");
  	if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
		erro("ERROR: bind function");
  	if( listen(fd, 5) < 0) 
		erro("ERROR: listen function");
  
  	// Create semaphore
    	key_t sem_key = ftok("server.c", 'R');
    	int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
   	if (sem_id == -1)
        	erro("semget");

   	// Initialize semaphore
    	semctl(sem_id, 0, SETVAL, 1);


  	while (1) {
    		client_addr_size = sizeof(client_addr);
    		client = accept(fd,(struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
    		if (client > 0) {
			strncpy(ip_addr, inet_ntoa(client_addr.sin_addr), MAX_IP_LENGTH);
      			if (fork() == 0) {
        			close(fd);
        			process_client(client, sem_id, ip_addr);
        			exit(0);
      			}
    			close(client);
    		}
  	}
  	return 0;
}


void process_client(int client_fd, int sem_id, char ip_addr[]) {
	int  nread = 0;
	char login[BUF_SIZE];
	char passwd[BUF_SIZE];

	write(client_fd, " Welcome to the SRC Authorization Server \n\n Login: ", 1 + strlen(" Welcome to the SRC Authorization Server \n\n Login: "));
	
	nread = read(client_fd, login, BUF_SIZE-1);	
	login[nread] = '\0';

	write(client_fd,"\nPassword: ", 1 + strlen("\nPassword: "));
	
	nread = read(client_fd, passwd, BUF_SIZE-1);	
	passwd[nread] = '\0';

	if (strcmp(login, "RC")==0 && strcmp(passwd,"1234")==0)
		write(client_fd, "Menu:\n\n 1 - opcao 1\n 2 - opcao 2 \n 3 - opcao 3", 1 + strlen("Menu:\n\n 1 - opcao 1\n 2 - opcao 2 \n 3 - opcao 3"));
	else
		write(client_fd, "Credenciais invalidas", 1 + strlen("Credenciais invalidas"));

	// Lock semaphore before accessing file
        struct sembuf sem_lock = {0, -1, SEM_UNDO};
        if (semop(sem_id, &sem_lock, 1) == -1) 
       		erro("semop");
            
        // Open file for writing
        int fd;
        if ((fd = open("clients.dat", O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1)
            	erro("open");
	
	struct client_info new_client;
  	new_client.socket_id = client_fd;
  	strncpy(new_client.ip_address, ip_addr, MAX_IP_LENGTH);

  	// Write client info to file
  	if (write(fd, &new_client, sizeof(struct client_info)) == -1)
            erro("write");
            
	// Close file and unlock semaphore
  	close(fd);
  	struct sembuf sem_unlock = {0, 1, SEM_UNDO};
  	if (semop(sem_id, &sem_unlock, 1) == -1)
            erro("semop");

	close(client_fd);

	print_clients();
}


void print_clients() {
    	int fd;
    	struct client_info client;

    	// Open the file
    	if ((fd = open("clients.dat", O_RDONLY)) == -1)
        	erro("open");
        
    	// Read and print each struct in the file
    	while (read(fd, &client, sizeof(struct client_info)) > 0) {
        	// Add null terminator to IP address
        	client.ip_address[MAX_IP_LENGTH - 1] = '\0'; // Ensure null termination
        
        	printf("Socket ID: %d, IP Address: %s\n", client.socket_id, client.ip_address);
    	}
    	// Close the file
    	close(fd);
}


void erro(char *msg) {
	printf("Erro: %s \n", msg);
	exit(-1);
	}

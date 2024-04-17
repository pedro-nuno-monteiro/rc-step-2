/*************************************************************
 * CLIENTE Projecto 2023-2024
 *************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define BUF_SIZE	1024

void erro(char *msg);

int main(int argc, char *argv[]) {
  char endServer[100];
  char teclado[100];
  int nread = 0;

  int fd, client;
  struct sockaddr_in addr, client_addr;
  int client_addr_size;
  struct hostent *hostPtr;

  if (argc != 3) {
    printf("cliente <host> <port> \n");
    exit(-1);
  }

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0) {
    printf("Couldn't get host address.\n");
    exit(-1);
  }

  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	erro("socket");
  if( connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
	erro("Connect");

  nread = read(fd, endServer, BUF_SIZE-1);	
  endServer[nread-1] = '\0';
  printf("%s",endServer);
  
  scanf(" %s", teclado);
  write(fd, teclado, 1 + strlen(teclado));

  nread = read(fd, endServer, BUF_SIZE-1);	
  endServer[nread-1] = '\0';
  printf("%s",endServer);

  scanf(" %s", teclado);
  write(fd, teclado, 1 + strlen(teclado));

  nread = read(fd, endServer, BUF_SIZE-1);	
  endServer[nread-1] = '\0';
  printf("%s \n",endServer);

  close(fd);
}

void erro(char *msg)
{
	printf("Erro: %s\n", msg);
	exit(-1);
}

/* CSD 304 Computer Networks, Fall 2016
   Lab 2, client
   Team: 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 5432
#define BUF_SIZE 256
#define RWS 3

int data_received[5];
int lfr,laf;

   int main(int argc, char * argv[]){

   	struct hostent *hp;
   	struct sockaddr_in sin;
   	char *host;
   	int s;
   	int len;

   	if ((argc==2)||(argc == 3)) {
   		host = argv[1];
   	}
   	else {
   		fprintf(stderr, "usage: client serverIP [download_filename(optional)]\n");
   		exit(1);
   	}


  /* translate host name into peer's IP address */
   	hp = gethostbyname(host);
   	if (!hp) {
   		fprintf(stderr, "client: unknown host: %s\n", host);
   		exit(1);
   	}
   	else
   		printf("Host %s found!\n", argv[1]);

  /* build address data structure */
   	memset((char *)&sin, 0, sizeof(sin));
   	sin.sin_family = AF_INET;
   	memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
   	sin.sin_port = htons(SERVER_PORT);
   	socklen_t client_addr_len = sizeof(sin);

  /* create socket */
   	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
   		perror("client: socket");
   		exit(1);
   	}

   	printf("Client will get data from to %s:%d.\n", argv[1], SERVER_PORT);

int num;
     int index=-1;
  int temp;
  /* send message to server */  
   	while(laf-lfr<=RWS)
   	{
      len = recvfrom(s,temp,sizeof(int), 0,(struct sockaddr *)&sin, &client_addr_len);
      data_received[temp]=temp;
      num = sendto(s,temp, sizeof(int), 0, (struct sockaddr *)&sin, sizeof(sin)); 
      printf("Received %d and sending ack for %d\n",temp,temp );       
  	}
}

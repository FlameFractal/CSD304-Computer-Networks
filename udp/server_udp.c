/*

LAB 2 SUBMISSION


TEAM MEMBERS
--------------------

Prerna - 1410110306
Vishal Gauba - 1410110501
Pranjal Mathur - 1410110296
Saketh Vallakatla - 1410110352

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "bitrate.h"

#define SERVER_PORT_TEXT 5432
#define SERVER_PORT_VID 6005
#define BUF_SIZE_TEXT 4096
#define BUF_SIZE_VID 40480
#define MAX_CLIENTS 5



typedef struct clientDetails
{
	int clientID;
	struct sockaddr_storage client_addr;
	char clientIP[INET_ADDRSTRLEN];
	char buf[BUF_SIZE_VID];
	int sentBytes;
	int counter;
	FILE* fp;
	pthread_t* tid;
} clientDetails_t;

int port = SERVER_PORT_VID;
int s;
char *host;
struct hostent *hp;
socklen_t client_addr_len;
char* fileName = "test.wav";
unsigned int bitRate = 0;
long idealSleep = 0;
struct timespec ts;
int clientConn[MAX_CLIENTS];
int noOfClients;

void resetC(clientDetails_t *client){
	// clientConn[client->clientID] = 0;
	pthread_t *tid = client->tid;
	free(client);
	free(tid);
	if(noOfClients > 0)
		--noOfClients;
}

void *sendToClient(void* carg){
	clientDetails_t *client = (clientDetails_t*)carg;

	inet_ntop(client->client_addr.ss_family,&(((struct sockaddr_in *)&(client->client_addr))->sin_addr),client->clientIP, INET_ADDRSTRLEN);
	printf("BitRate required = %u\n", getBitRate(fileName));
    client->fp = fopen(fileName, "rb");
    if(client->fp == NULL){
    	perror("File:");
    	resetC(client);
    	return 0;
    }
    int len = strlen(client->buf)+1;
    printf("Server got message from %s: %s [%d bytes]\n", client->clientIP, client->buf, len);
    int size = BUF_SIZE_VID;
	while(!(size < BUF_SIZE_VID)){
  			size = fread(client->buf, 1, sizeof(client->buf), client->fp);

  			if ((len = sendto(s, client->buf, size, 0,(struct sockaddr *)&(client->client_addr),client_addr_len)) == -1) {
  			  perror("server: sendto");
  			  exit(1);
  			}	
  			if(len != size){
  				printf("ERROR!!");
  				exit(0);
  			}
  			++(client->counter);
//        	printf("Counter : %d , Size = %d , Len = %d\n", client->counter, size, len);
  			nanosleep(&ts, NULL);
  		  memset(client->buf, 0, sizeof(client->buf));
  		  (client->sentBytes)+=size;
  		}
  	strcpy(client->buf, "BYE");
  	sendto(s, client->buf, strlen(client->buf)+1, 0, (struct sockaddr*)&(client->client_addr), client_addr_len);
  	printf("\nSentBytes = %d Sent Packets = %d\n\n\n", client->sentBytes, client->counter);
  	client->sentBytes = 0;
	resetC(client);

}

int main(int argc, char * argv[]){

  printf("This is a Multi-Client server. Thanks for using ;p\n");
  struct sockaddr_in sin;
  
  ts.tv_sec = 0;
  ts.tv_nsec = 2000000L; // default
  // ts.tv_nsec = 200000L;
  char clientIP[INET_ADDRSTRLEN]; /* For IPv4 addresses */
  struct sockaddr_storage client_addr_temp;
//  clientDetails_t clients[MAX_CLIENTS];
  
  clientDetails_t *client;
  
  // pthread_t tid[MAX_CLIENTS];
  // pthread_t *tid;
  char buf[BUF_SIZE_VID];
  int i=0;

  if(argc == 2){
  	if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
  		printf("usage:./server nanopsleep IP_ADDR PORT FILE_NAME\n");
  		exit(0);
  	}
  }

  if(argc >= 2){
  	ts.tv_nsec = atoi(argv[1]);
  }

  if(argc >= 4){
  	port = atoi(argv[3]);
  }

  if(argc >= 5){
  	fileName = argv[4];
  	bitRate = getBitRate(fileName);
  }

  printf("BitRate = %d\n", bitRate);

  // Calc time delay
  idealSleep = ((40480*8)/bitRate)*1000000;
  
  if(idealSleep < 0)
  	idealSleep = ts.tv_nsec;

  if(ts.tv_nsec > idealSleep)
  	ts.tv_nsec = idealSleep;

  printf("Time delay : %ld idealSleep = %ld\n", ts.tv_nsec, idealSleep);
   
  /* Create a socket */
  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server: socket");
    exit(1);
  }
 
 
  /* build address data structure and bind to all local addresses*/
  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
 
  /* If socket IP address specified, bind to it. */
  if(argc >= 3) {
    host = argv[2];
    hp = gethostbyname(host);
    if (!hp) {
      fprintf(stderr, "server: unknown host %s\n", host);
      exit(1);
    }
    memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
  }
  /* Else bind to 0.0.0.0 */
  else
    sin.sin_addr.s_addr = INADDR_ANY;
  
  sin.sin_port = htons(port);
  
  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("server: bind");
    exit(1);
  }
  else{
    /* Add code to parse IPv6 addresses */
    inet_ntop(AF_INET, &(sin.sin_addr), clientIP, INET_ADDRSTRLEN);
    printf("Server is listening at address %s:%d\n", clientIP, SERVER_PORT_VID);
  }
  
  printf("Client needs to send \"GET\" to receive the file %s\n", host);  
  
  client_addr_len = sizeof(client_addr_temp);
  int len = 0;  
  /* Receive messages from clients*/
  while(len = recvfrom(s, buf, sizeof(buf), 0,(struct sockaddr *)&client_addr_temp, &client_addr_len)){
  	if(noOfClients == MAX_CLIENTS)
  		continue;
  	// int cur = 0;
  	
  	// for(cur=0;cur<MAX_CLIENTS;cur++){
  		// if(clientConn[cur] == 0) break;
  	// }
  	
  	// clientConn[cur] = 1;
  	client = (clientDetails_t*)malloc(sizeof(clientDetails_t));
  	memset(client, 0, sizeof(clientDetails_t));
  	client->tid = (pthread_t*) malloc(sizeof(pthread_t));
  	
  	memcpy(&(client->client_addr), &client_addr_temp, sizeof(struct sockaddr_storage));
  	
  	// clients[cur].clientID = cur;
  	++noOfClients;
  	
  	//if(strcmp(buf, "GET") == 0){
  		pthread_create(client->tid, NULL, sendToClient, client);
    	//pthread_join(tid, NULL);
  	//}
  	printf("Waiting for the next client...\n");
  }
}


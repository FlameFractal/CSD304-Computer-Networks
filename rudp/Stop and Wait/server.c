/*
	LAB 3 SUBMISSION

	USAGE: ./server FILENAME [TIME_OUT (30000us)] [IP_ADDR (localhost)] [PORT (6005)]

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

#define SERVER_PORT 6005
#define PACKET_SIZE 40480+1

int main(int argc, char * argv[]){

	printf("\nThis is a Reliable UDP based Server. \nUSAGE: ./server FILENAME [TIME_OUT (30000us)] [IP_ADDR (localhost)] [PORT (6005)]. \nInitialising processes.\n\n");


	/* Variable declarations */
	struct sockaddr_in sin;
	struct sockaddr_storage client_addr;
	char clientIP[INET_ADDRSTRLEN]; /* For IPv4 addresses */
	char *host;
	socklen_t client_addr_len;
	struct hostent *hp;	
	int port;

	char packet[PACKET_SIZE];
	int len;
	int sock;
	int timeout = 30000;

	FILE* fp;  
	char* fileName = "in.mp4";




	/* Extracting arguments */
	if (argc == 1)
	{
		printf("Wrong usage: Please enter file name to serve.\n\n");
		exit(1);
	}

	if (argc >= 2) {
		fileName = argv[1];  
		fp = fopen(fileName, "rb");
		if(fp == NULL){
			printf("Input file %s: not found",fileName);
			exit(1);
		}
	}

	if (argc >= 3)
	{
		timeout = atoi (argv[2]);
	}

	if (argc >= 4)
	{
		host = argv[3];
	}

	if (argc == 5)
	{
		port = atoi (argv[4]);
	}

	if (argc > 5)
	{
		printf("Wrong usage: Expected  max 5 arguments.\n");
		exit(1);
	}




	/* Create a socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("server: socket creation error");
		exit(1);
	}

	/* build address data structure and bind to all local addresses*/
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	/* If socket IP address specified, bind to it. */
	if(argc >= 4) {
		hp = gethostbyname(host);
		if (!hp) {
		  fprintf(stderr, "Server: unknown host. Binding to default.\n");
		  exit(1);
		}
		memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	}
	else /* Else bind to 0.0.0.0 */{
		sin.sin_addr.s_addr = INADDR_ANY;
	}
	
	sin.sin_port = htons(port);
  	if (!sin.sin_port)
  	{
  		fprintf(stderr, "Server: unknown port. Binding to default.\n");
  		sin.sin_port = htons(SERVER_PORT);
  	}
  
	if ((bind(sock, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("server: bind");
		exit(1);
	}
	else{ 	/* Add code to parse IPv6 addresses */
		inet_ntop(AF_INET, &(sin.sin_addr), clientIP, INET_ADDRSTRLEN);
	}
  
	client_addr_len = sizeof(client_addr);
  	
  	int c=0;
  	printf("\n\nServer is listening at address %s:%d\n", clientIP, SERVER_PORT);
	printf("Client needs to send \"get\" to receive the file : '%s'\nTimeout = %dus\n\n", argv[1], timeout);

	/* Receive messages from clients*/
	while(len = recvfrom(sock, packet, sizeof(packet), 0,(struct sockaddr *)&client_addr, &client_addr_len)){

		if(c!=0){ /* Print for second onwards clients*/
			printf("\n\nServer is listening at address %s:%d\n", clientIP, SERVER_PORT);
			printf("Client needs to send \"get\" to receive the file : '%s'\nTimeout = %dus\n\n", argv[1], timeout);
		}
		c=1;
		inet_ntop(client_addr.ss_family,&(((struct sockaddr_in *)&client_addr)->sin_addr),clientIP, INET_ADDRSTRLEN);
		rewind(fp);
		if(strcmp(packet, "get") == 0){
			
			printf("Server got message from %s: '%s' [%d bytes]\n\n", clientIP, packet, len);
		 	
		 	int size = PACKET_SIZE;
		 	int packet_counter = 0;
		 	char expectation = '0';
			int retransmit_counter = 0;
			int sentBytes = 0;
			char ack = '\0';

	 		while(size >= PACKET_SIZE){

				size = fread (packet, 1, sizeof(packet)-1, fp);
				packet[size] = expectation;  /* Append expectation at the last byte */
				size++; /* Increment packet size by 1 for sequence number appended to end */

				len = sendto(sock, packet, size, 0,(struct sockaddr *)&client_addr,client_addr_len);
				printf("Packet %d [size=%d] sending.\n", packet_counter++, size);

				if (len == -1) {
				  perror("server: sendto");
				  exit(1);
				}
				if(len != size){
				  printf("ERROR!! Couldn't send all the bytes that were read from the file.");
				  exit(0);
				}   

				/* Set recieve timeout in socket options for Stop and Wait timeout */
				struct timeval timeout_t;
				timeout_t.tv_sec = 0;
				timeout_t.tv_usec = timeout;
				if (setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO, &timeout_t, sizeof(timeout_t)) < 0){
					perror("server: socket option - set timeout error");
				}

				while(1){

					recvfrom(sock, &ack, sizeof(ack), 0,(struct sockaddr *)&client_addr, &client_addr_len); /* Try to recieve ack within a set timeout period */
					printf("Ack = '%c',  ",ack);

					if(ack =='\0'){
						printf("Timed out. Retransmitting packet  %d.\n",packet_counter);
						len = sendto(sock, packet, size, 0,(struct sockaddr *)&client_addr,client_addr_len);
						retransmit_counter++;
					}
					else if (ack == expectation){
						printf("Packet %d acknowledged! Sending next.\n", packet_counter);
						memset(packet, 0, sizeof(packet));
						sentBytes+=size;
						/* Reset ack and switch expectation (for next packet) */
			  			ack = '\0'; 
			  			expectation = (expectation == '0') ? '1': '0';
						break;
					}
					else{ /* ack == previous ack (or junk) */
					  printf("Recieved duplicate ack (of previous packet). Ignoring.\n");
					  ack='\0';
					}
			  	} 
			}


			/*	End of transmission.
				Sending BYE 		*/
			strcpy(packet, "BYE");
			sendto(sock, packet, strlen(packet)+1, 0, (struct sockaddr*)&client_addr, client_addr_len);
			memset(packet, '\0', sizeof(packet));

			printf("\n\n\nSent Bytes = %d,\nSent Packets (unique) = %d, \nRetransmitted Packets = %d, \nEfficiency = %d%% \n", sentBytes, packet_counter, retransmit_counter, (packet_counter*100)/(retransmit_counter+packet_counter));
			
			/* Reset socket option of timeout to recieve request from next client*/
			struct timeval timeout_t;
			timeout_t.tv_sec = 0;
			timeout_t.tv_usec = 0;
			if (setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO, &timeout_t, sizeof(timeout_t)) < 0){
				perror("server: socket option timeout error");
		   }
		}
	} 
} 

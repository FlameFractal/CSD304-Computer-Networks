/*
	LAB 3 SUBMISSION

	USAGE: ./client IP_ADDR [OUTPUT_FILE_NAME (out.mp4)] [PORT (6005)]

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
#include <unistd.h>
#include <sys/time.h>

#define SERVER_PORT 6005
#define PACKET_SIZE 40480+1

long curTimeMillis() {
	struct timeval te; 
    gettimeofday(&te, NULL); /* get current time */
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;  /* caculate milliseconds */
    return milliseconds;
}

int main(int argc, char * argv[]){

	printf("\nThis is a Reliable UDP based Client. \nUSAGE: ./client IP_ADDR [OUTPUT_FILE_NAME (out.mp4)] [PORT (6005)]. \nInitialising processes.\n\n");


	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	int port;

	char* fileName = "out.mp4";
	char packet[PACKET_SIZE];
	int sock;
	int len;

	/* Extract arguments */
	if (argc == 1){
		printf("Wrong usage: IP_ADDR of server necessary.\n\n");
		exit(1);
	}

	if (argc>=2)
	{
		host = argv[1];
	}

	if (argc>=3)
	{
		fileName = argv[2];
		fp = fopen(fileName, "w");
		if (fp == NULL) {
			fprintf(stderr, "Error opening output file\n");
			exit(1);
		}
	}

	if (argc==4)
	{
		port  = atoi(argv[2]);
	}

	if (argc>4)
	{
		printf("Wrong usage: Expected max 3 arguments.\n");
	}



	
	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr, "Client: unknown host: %s\n", host);
		exit(1);
	}
	else
		printf("Host '%s' found!\n", argv[1]);

	/* build address data structure */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);


	/* create socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Client: socket creation error");
		exit(1);
	}

	/* active open */
	int c = connect(sock, (struct sockaddr *)&sin, sizeof(sin));
	if (c < 0) {
		perror("client: connect");
		close(sock);
		exit(1);
	}
	else{
		
		printf("Client connected to %s:%d.\n\n", argv[1], SERVER_PORT);
	}


	/* send "get" message to server */  
	fgets(packet, 4, stdin);
	strtok(packet,"\n");	
	packet[PACKET_SIZE-1] = '\0';
	len = strlen(packet) + 1;
	send(sock, packet, len, 0);
	
	printf("Waiting to recieve.\n");
	socklen_t client_addr_len = sizeof(sin);
	long before = curTimeMillis();

	int packet_counter = 0;
	char expectation='0';
	int recvBytes = 0;
	int duplicate_counter = 0;

	while(len = PACKET_SIZE){

		len = recvfrom(sock, packet, PACKET_SIZE, 0,(struct sockaddr *)&sin, &client_addr_len);

		if (packet[len-1] == expectation){
			send(sock, &expectation, sizeof(expectation), 0); /* send acknowledgement */
			printf("Recieved new packet. (Sequence = %c)\n", expectation);
			
			packet_counter++;
			recvBytes+=len;
			expectation = (expectation == '0') ? '1': '0';	/* Switch expectation */
			
			fp = fopen(fileName, "ab+");
			fwrite(packet, 1,len-1, fp);
			fclose(fp);
		}
		else if(packet[len-1] == (expectation == '0') ? '1': '0'){	/* Duplicate packet has been recieved */
			expectation = (expectation == '0') ? '1': '0';		/* Send previous packet's acknowledgement */
			send(sock, &expectation, sizeof(expectation), 0);
			printf("Received duplicate packet. (Sequence = %c)\n", expectation);
			expectation = (expectation == '0') ? '1': '0';		/* Switch back expectation to current*/
			duplicate_counter++;
		}
		else{
			printf("dont know wtf going on\n");
		}

		if(strstr(packet, "BYE")) break;
		
	}

	long after = curTimeMillis();
	long timeTakenMs = (after - before);
	
	printf("\n\n\nReceived Bytes = %d, \nReceived Packets (unique) =  %d, \nDuplicate Packets = %d, \nEfficiency = %d%%", recvBytes, packet_counter, duplicate_counter, (packet_counter*100)/(packet_counter+duplicate_counter));
	printf("\nTime Taken = %.2f seconds\n\n", (float)(after - before)/1000);
}
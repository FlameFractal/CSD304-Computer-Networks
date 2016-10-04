/*

LAB 2 SUBMISSION


TEAM MEMBERS
--------------------
Saketh Vallakatla - 1410110352
Prerna - 1410110306
Vishal Gauba - 1410110501
Pranjal Mathur - 1410110296
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
#define BUF_SIZE 40480+1

long curTimeMillis() {
	struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

int main(int argc, char * argv[]){

	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char buf[BUF_SIZE];
	int s;
	int len;
	int recvBytes = 0;
	char ack='a';
	char expectation = '\0';


	if ((argc==2)||(argc == 3)) {
		host = argv[1];
	}
	else {
		fprintf(stderr, "usage: client serverIP [download_filename(optional)]\n");
		exit(1);
	}

	if(argc == 3) {
		fp = fopen(argv[2], "w");
		if (fp == NULL) {
			fprintf(stderr, "Error opening output file\n");
			exit(1);
		}
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


	  /* create socket */
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("client: socket");
		exit(1);
	}

	  /* active open */
	int c = connect(s, (struct sockaddr *)&sin, sizeof(sin));
	if (c < 0) {
		perror("client: connect");
		close(s);
		exit(1);
	}
	else{
		
		printf("Client connected to %s:%d. c=%d\n", argv[1], SERVER_PORT,c );
	}


	  /* send message to server */  
	// fgets(buf, sizeof(buf), stdin);
	//strcpy(buf,"GET\n");
	fgets(buf, 4, stdin);
	strtok(buf,"\n");	
	buf[BUF_SIZE-1] = '\0';
//	  char* buf1 = "GET";
	len = strlen(buf) + 1;
	send(s, buf, len, 0);
	printf("Waiting...\n");
	socklen_t client_addr_len = sizeof(sin);
	int counter = 0;

	long before = curTimeMillis();

	//Initialize expectation
	expectation='0';

	while(len<40481){

		len = recvfrom(s, buf, BUF_SIZE, 0,(struct sockaddr *)&sin, &client_addr_len);

		if (buf[len] == expectation){
			int expectlen = send(s, &expectation, sizeof(expectation), 0);
			printf("expectation=%c (Next)\n", expectation);
			//Switch expectation
			expectation = (expectation == '0') ? '1': '0';
			//printf("packet ln=%d",len);
			
			if(argc == 3){
				fp = fopen(argv[2], "ab+");
				//For last packet where 
				fwrite(buf, 1,len-1, fp);
				fclose(fp);
			}
			
		}
		else{
			
			//Duplicate has been recieved
			//Send previous expection's acknowledgement
			
			expectation = (expectation == '0') ? '1': '0';	
			int expectlen = send(s, &expectation, sizeof(expectation), 0);
			printf("expectation=%c (Duplicate)\n", expectation);
			//Switch to current expectation
			expectation = (expectation == '0') ? '1': '0';

		}

		
		if(strstr(buf, "BYE")) break;
		
		recvBytes+=len;
		++counter;
	}

	long after = curTimeMillis();

	long timeTakenMs = (after - before);

	printf("\n");
	printf("\nRecvBytes = %d Received Packets : %d\n\n", recvBytes, counter);
	printf("Time Taken(msec) = %ld ms\n\n", (after - before));
	//printf("Time Taken(sec) = %ld s\n", (after - before)/1000);
}

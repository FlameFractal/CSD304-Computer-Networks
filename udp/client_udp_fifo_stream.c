#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>

#define SERVER_PORT 6005
#define BUF_SIZE 40480
#define VLCPIPER "vP"

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
		// printf("Host %s found!\n", argv[1]);

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
		
		// printf("Client connected to %s:%d. c=%d\n", argv[1], SERVER_PORT,c );
		// printf("To play the music, pipe the downlaod file to a player, e.g., ALSA, SOX, VLC: cat recvd_file.wav | vlc -\n"); 
	}



	  /* send message to server */  
	// fgets(buf, sizeof(buf), stdin);
	strcpy(buf,"GET\n");
	buf[BUF_SIZE-1] = '\0';
//	  char* buf1 = "GET";
	len = strlen(buf) + 1;
	send(s, buf, len, 0);
	// printf("Waiting...\n");

	// printf("Waiting... in while LLLLLL\n"); 

	socklen_t client_addr_len = sizeof(sin);
	int counter = 0;

	long before = curTimeMillis();
	const int maxBuffer = 5;
	int bufCounter = 0;
	char longBuf[BUF_SIZE*maxBuffer];
	// fp = fopen(".tet.wav", "wb+");
	int oi = 0;
	int o = 0; int p = 0;
	int status = mkfifo(VLCPIPER, 0777);
	int fifo = open(VLCPIPER, O_RDWR);

	while(1){
		  //fputs(buf, stdout);
		// printf("Waiting... in while\n");
//		len = recv(s, buf, sizeof(buf), 0);
		len = recvfrom(s, buf, BUF_SIZE, 0,(struct sockaddr *)&sin, &client_addr_len);
		// fputs(buf, stdout);
		if(strcmp(buf, "BYE") == 0) break;
		printf("Counter : %d , Size = %d , Len = %d\n", counter++, len, len);
		
		if(argc == 2){
			// fifo = open(VLCPIPER, O_RDWR);
			// if(bufCounter < maxBuffer){
			// 	memcpy(&(longBuf[BUF_SIZE*bufCounter]), buf, BUF_SIZE);
			// 	bufCounter++;
			// } else{
				printf("ELSING\n");
				// if(bufCounter++ == maxBuffer)
					// write(fifo, longBuf, sizeof(longBuf));
				// fwrite(buf, 1, len, stdout);
				// fwrite(buf, 1, len, stdout);
				write(fifo, buf, len);
				// close(fifo);
				// system("cat vP &");
				// fflush(stdout);
				// fclose(fp);
				// if(o == 0){
				// 	int p = fork();
				// 	o = 1;
				// }
					
				// printf("IFFING\n");
				if((p ==0) && (oi == 0)){
					// system("cat .tet.mp3 | vlc - &");
					printf("VLCING\n");
					system("vlc fd://vP &");
					oi = 1;
				}
			// }
		}

		else if(argc == 3){
			fp = fopen(argv[2], "ab+");
			fwrite(buf, 1,len, fp);
			fclose(fp);
		}
		
		recvBytes+=len;
		// printf("Counter : %d ,Len = %d\n", ++counter, len);
	}
	system("rm vlcPiper");
	printf("OUT of loop\n");

	long after = curTimeMillis();

	long timeTakenMs = (after - before);

	printf("\n");
	printf("\nRecvBytes = %d\n", recvBytes);
	printf("Time Taken(msec) = %ld ms\n", (after - before));
	printf("Time Taken(sec) = %ld s\n", (after - before)/1000);
}
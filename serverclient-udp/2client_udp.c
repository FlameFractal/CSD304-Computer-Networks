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
#include <unistd.h>

#define SERVER_PORT 5432
#define BUF_SIZE 40480
#define FILEBUF 4

int main(int argc, char * argv[]){
  
  FILE *fp;
  struct hostent *hp;
  struct sockaddr_in sin;
  char *host;
  char buf[BUF_SIZE];
  char filebuf[FILEBUF];
  int s;
  int len;



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
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    
      perror("client: connect");
      close(s);
      exit(1);
    }
  else{
    
    printf("Client connected to %s:%d.\n", argv[1], SERVER_PORT);
    printf("To play the music, pipe the downlaod file to a player, e.g., ALSA, SOX, VLC: cat recvd_file.wav | vlc -\n"); 
    }


    fp = fopen(argv[2], "w+");
    fclose(fp);
  
  

  /* send message to server */  
  fgets(buf, sizeof(buf), stdin);

  buf[BUF_SIZE-1] = '\0';
  len = strlen(buf) + 1;
  
  send(s, buf, len, 0);
  
  //printf("Waiting for first byte\n\n"); 
  
  while((strcmp(filebuf, "BYE") != 0)){
	  //fputs(buf, stdout);
  	//printf("Waiting... in while\n");
  	len += recv(s, filebuf, sizeof(filebuf), 0);
    printf("len = %d",len);
  	fp = fopen(argv[2], "ab+");
  	//fputs(buf, stdout);
  	//printf("\n");
    fwrite(filebuf, 1,1, fp);
  	fclose(fp);
	
  }
  printf("out. \n");
}

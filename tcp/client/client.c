#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define SERVER_PORT 5432
#define MAX_LINE 256


int main(int argc, char * argv[]){
  FILE *fp;
  FILE *file;

  struct hostent *hp;
  struct sockaddr_in sin;
  char *host;
  
  char byte;
  long int size;
  char buf[MAX_LINE];
  char fileRecv[256];
  int s;
  int len;
  
//  unsigned char buffer[BUFFER_SIZE];

  
  if (argc==2) {
    host = argv[1];
  }
  else {
    fprintf(stderr, "usage: simplex-talk host\n");
    exit(1);
  }
  /* translate host name into peer's IP address */
  hp = gethostbyname(host);
  if (!hp) {
    fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
    exit(1);
  }
  else
    printf("Client's remote host: %s\n", argv[1]);
  
  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
  sin.sin_port = htons(SERVER_PORT);

  /* active open */
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("simplex-talk: socket");
    exit(1);
  }
  else
    printf("Client created socket.\n");


  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("simplex-talk: connect");
    close(s);
    exit(1);
  }
  else
    printf("Client connected.\n");

  char handshake[20];
  recv(s,handshake,sizeof(handshake),0);
  printf("%s\n\n",handshake);

  memset(buf, 0, MAX_LINE);
  /* main loop: get and send lines of text */
  while (fgets(buf, sizeof(buf), stdin)) {
    buf[MAX_LINE-1] = '\0';
    //strtok(buf,"\n");
    char *newline;
    if(newline=strchr(buf,'\n'));
    buf[newline-buf]='\0';  //removing \n that comes with fgets
  
    len = strlen(buf);
//    printf("len=%d\n",len);  
    send(s, buf, len, 0);
    
    printf("buf = %s .\n",buf);
    
    if(strcasecmp("bye",buf)==0){
      printf("bye. exiting.\n");
      exit(1);
    }

    recv(s, &fileRecv, sizeof(fileRecv), 0);

    if(strcasecmp(fileRecv,"ok") == 0){
      printf("Ok. File available. Ready for transfer.\n");  
      long int filesize;
	long int f;
      recv(s, &f, sizeof(long int), 0);
	filesize = ntohl(f);
      printf("Recieving %ld bytes\n",filesize);     
    
      char byte;
      long size;
      file = fopen(buf,"wb+");
      for(size = 0;size<filesize;size++)
      { 
        recv(s,&byte,sizeof(byte),0);
        fwrite(&byte,1,1,file);//write into file
      }
      fclose(file);
      
      printf("File \"%s\" recieved.\n",buf);



    }
    else{
      printf("File not found at server\n");
    }
}
}

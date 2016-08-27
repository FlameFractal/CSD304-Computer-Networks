////////////////CLIENT PROGRAM////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 5432
#define MAX_LINE 256



int main(int argc, char * argv[]){
  FILE *fp;
  struct hostent *hp;
  struct sockaddr_in sin; 
  char *host;
  char buf[MAX_LINE];
  char recvbuffer[256];
  char filebuff[256];
  int s;
  int length;
  
  
  if (argc==2) {
    host = argv[1];
  }
  else {
    fprintf(stderr, "usage: simplex-talk host\n");
    exit(1);
  }
  
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
    
    //Server message
    char message[30];
    recv(s, message, sizeof(message), 0);
    printf("%s\n\n",message);
    
  //File Name to the Server
  while (fgets(buf, sizeof(buf), stdin)) {
   	buf[MAX_LINE-1] = '\0';
   	length = strlen(buf);
    	send(s, buf, length, 0);
    	buf[len-1] = '\0';
	if(strcmp("bye",buf)==0)
	return 0;
    
	
    recv(s,recvbuffer,sizeof(recvbuffer), 0);
    
    if(strcmp(recvbuffer,"OK") == 0)
    {
    
    	printf("Reply from Server: OK\n");
    	long size;
    	recv(s,&size,sizeof(long),0);
    	
    	
    	
    	if(ClientCopy(buf,0,size,s) == 1)
    		printf("File Recieved\n");
    	
    }
    else
    {
    	printf("Reply from Server: Not Found\n");
    }
   
  }
}

//Create Client Copy and Recieve character by character
int ClientCopy(char* fileName,int t, long sizefile,int socket_ID)
{
	char c;
	long i;
	FILE* fp= fopen(fileName,"wb");
	for(i = 0;i<sizefile;i++)
	{	
		recv(socket_ID,&c,sizeof(c),0);
		fwrite(&c,1,1,fp);//write into file
	}
	fclose(fp);
	return 1;
}


/////////////////////SERVER PROGRAM/////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256

long Size(FILE* fp,double j);
int main(){

  struct sockaddr_in sin;
  char buf[MAX_LINE],filebuff[256];
  int len;
  int s, new_s;
  char str[INET_ADDRSTRLEN];
	long sizel;

  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(SERVER_PORT);
  /* setup passive open */
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("simplex-talk: socket");
    exit(1);
  }
 
  inet_ntop(AF_INET, &(sin.sin_addr), str, INET_ADDRSTRLEN); // Converts the network address into a string
  printf("Server is using address %s and port %d.\n", str, SERVER_PORT);

  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("simplex-talk: bind");
    exit(1);
  }
  else
    printf("Server bind done.\n");
    
    
   

  listen(s, MAX_PENDING);
  
  while(1) {
    if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
      perror("simplex-talk: accept");
      exit(1);
    }
    printf("Server Listening.\n\n");
    
   
   char greetings[3][30]={"Now you can request the files."};
    
    send(new_s, greetings, 30, 0 );
    
    char fileID[256];
    
    while (len = recv(new_s, fileID, sizeof(fileID), 0)){
      	printf("Requested File: %s",fileID);
	strtok(fileID,"\n");
 
 		
      	int p = tracename(fileID);
      	
		if(p == -1)
		{
			send(new_s,"Not Found",10, 0);
				
		}
		else
		{
			send(new_s,"OK",3, 0);
			
			
			
			FILE* fp = fopen(fileID,"rb");
			sizel = Size(fp,0);
			fclose(fp);
			send(new_s,&sizel,sizeof(long),0);
			
			
			
			if(transfer(fileID,0,sizel, new_s) == -1)
				printf("Some Error\n");
			else
				printf("The requested file must be received by the client.\n");
			
      	}
      	     
     }
    close(new_s);
  }
}

int tracename(char* fileName)
{
FILE* fp = fopen(fileName,"rb");
	if(fp == NULL)
		return -1;
	else
	{
		fclose(fp);//closing file
		return 1;
	}
}

long Size(FILE* fp,double k)//calculate size of file
{
long j;			
fseek(fp,0L, 2);//sets the file position of the stream to the given offset.
			j = ftell(fp);
			rewind(fp);//sets the file position to the beginning of the file of the given stream.
			
			return j;
}

int transfer(char fileID[],int k, long filesize, int SID)
{
	char a;
	long i;
	
	FILE* fp = fopen(fileID,"rb");
	if(fp == NULL)
		return -1;
	
		
	for(i = 0;i<filesize;i++)
	{
		fread(&a,1,1,fp);
		send(SID,&a,sizeof(a),0);
	}
	return 1;
			
}

/* CSD 304 Computer Networks, Fall 2016
   Lab 2, server
   Team: 
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

#define SERVER_PORT_TEXT 5432
#define SERVER_PORT_VID 6005
#define BUF_SIZE_TEXT 4096
#define BUF_SIZE_VID 40480+1


int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  struct sockaddr_storage client_addr;
  char clientIP[INET_ADDRSTRLEN]; /* For IPv4 addresses */
  socklen_t client_addr_len;
  char buf[BUF_SIZE_VID];
  int len;
  int s;
  char *host;
  int sentBytes = 0;
  struct hostent *hp;
  char ack = '\0';
  char expectation = '\0';

  FILE* fp;  
  char* fileName = "in.mp4";
//  struct timespec ts;
  // ts.tv_sec = 0;
  // // ts.tv_nsec = 200000000L;
  // ts.tv_nsec = 2000L;

  
  
  if (argc > 2) {
    fileName = argv[2];  
  }
  
  fp = fopen(fileName, "rb");
  if(fp == NULL){
    printf("file %s: not found",fileName);
    exit(1);
  }


  /* Create a socket */
  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server: socket");
    exit(1);
  }


  /* build address data structure and bind to all local addresses*/
  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;

  /* If socket IP address specified, bind to it. */
  if(argc == 2) {
    host = argv[1];
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
  
  sin.sin_port = htons(SERVER_PORT_VID);
  
  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("server: bind");
    exit(1);
  }
  else{
    /* Add code to parse IPv6 addresses */
    inet_ntop(AF_INET, &(sin.sin_addr), clientIP, INET_ADDRSTRLEN);
    printf("Server is listening at address %s:%d\n", clientIP, SERVER_PORT_VID);
  }
  
  printf("Client needs to send \"GET\" to receive the file %s\n", argv[1]);  
  
  client_addr_len = sizeof(client_addr);


  
  /* Receive messages from clients*/
  while(len = recvfrom(s, buf, sizeof(buf), 0,(struct sockaddr *)&client_addr, &client_addr_len)){

    // len = recvfrom(s, buf, sizeof(buf), 0,(struct sockaddr *)&client_addr, &client_addr_len);

    inet_ntop(client_addr.ss_family,&(((struct sockaddr_in *)&client_addr)->sin_addr),clientIP, INET_ADDRSTRLEN);

    printf("Server got message from %s: %s [%d bytes]\n", clientIP, buf, len);
    rewind(fp);

    if(strcmp(buf, "get") == 0){
      int size = BUF_SIZE_VID;
      int counter = 0;
      expectation = '0';

      while(!(size < BUF_SIZE_VID)){

        size = fread(buf, 1, sizeof(buf)-1, fp);
        buf[size] = expectation;  //Append expectation
        size++; //Increment packet size by 1 for sequence number appended to end

        len = sendto(s, buf, size, 0,(struct sockaddr *)&client_addr,client_addr_len);

        if (len == -1) {
          perror("server: sendto");
          exit(1);
        }
        if(len != size){
          printf("ERROR!!");
          exit(0);
        }   

      /* Set recieve timeout in socket options */
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 30000;
        if (setsockopt(s,SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
         perror("server: socket option timeout error");
       }

       while(1){

        int acklen = recvfrom(s, &ack, sizeof(ack), 0,(struct sockaddr *)&client_addr, &client_addr_len);
        printf("ack = %c. ",ack);

        if(ack =='\0'){
          printf("Timed out. Retransmitting packet counter %d \n",counter);
          len = sendto(s, buf, size, 0,(struct sockaddr *)&client_addr,client_addr_len);    
        }
        else if (ack == expectation){
          printf("Packet acknowledged. Sending next.\n");
          expectation = (expectation == '0') ? '1': '0';  //Switch expectation
          break;
        }
        else{ //ack !=expectation i.e. ack = 1-exp
          printf("Recieved duplicate ack of previous packet. Ignoring.\n"); //if opposite of expectation is recieved in ack
        }

      }
      
      //Reset ack
      ack = '\0'; 

      printf("Counter : %d , Size = %d , Len = %d\n", counter++, size, len);
      memset(buf, 0, sizeof(buf));
      sentBytes+=size;
    }

    //SENDING BYE
    strcpy(buf, "BYE");
    sendto(s, buf, strlen(buf)+1, 0, (struct sockaddr*)&client_addr, client_addr_len);
    memset(buf, '\0', sizeof(buf));

    printf("\nSentBytes = %d\n", sentBytes);

    //Reset socket option of timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (setsockopt(s,SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
     perror("server: socket option timeout error");
   }

 }
} 
} 

/* CSD 304 Computer Networks, Fall 2016
   Lab 2, server
   Team: 
*/

#include <stdio.h>
#include<pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT 5432
#define BUF_SIZE 256
#define SWS 3
   #define TIMEOUT 5

   int check_timeout(clock_t start);
   void * receive_acks();


   int lar,lfs;
   clock_t start_vals[5];
   int ack_recv[5];
   struct sockaddr_in sin_variable;
   struct sockaddr_storage client_addr;
   char clientIP[INET_ADDRSTRLEN]; /* For IPv4 addresses */
   socklen_t client_addr_len;
   char *host;
   struct hostent *hp;
   int s;
   

   int main(int argc, char * argv[]){
    pthread_t ack_thread;
    lar=-1;
    lfs=-1;
    int index;

  /* Create a socket */
     if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("server: socket");
      exit(1);
    }


  /* build address data structure and bind to all local addresses*/
    memset((char *)&sin_variable, 0, sizeof(sin_variable));
    sin_variable.sin_family = AF_INET;

  /* If socket IP address specified, bind to it. */
    if(argc == 2) {
      host = argv[1];
      hp = gethostbyname(host);
      if (!hp) {
        fprintf(stderr, "server: unknown host %s\n", host);
        exit(1);
      }
      memcpy((char *)&sin_variable.sin_addr, hp->h_addr, hp->h_length);
    }
  /* Else bind to 0.0.0.0 */
    else
      sin_variable.sin_addr.s_addr = INADDR_ANY;

    sin_variable.sin_port = htons(SERVER_PORT);

    if ((bind(s, (struct sockaddr *)&sin_variable, sizeof(sin_variable))) < 0) {
      perror("server: bind");
      exit(1);
    }
    else{
    /* Add code to parse IPv6 addresses */
      inet_ntop(AF_INET, &(sin_variable.sin_addr), clientIP, INET_ADDRSTRLEN);
      printf("Server is listening at address %s:%d\n", clientIP, SERVER_PORT);
    }

    printf("Client needs to send \"GET\" to receive the file %s\n", argv[1]);
    int i=0;
    for(i=0;i<5;i++)
    {
      ack_recv[i]=0;
    }

    int data[]={1,2,3,4,5};
    index=-1;
    int len=0;
    pthread_create(&ack_thread,NULL,receive_acks,NULL);
    printf("Created new thread\n");
    while(index<5)
    {
      while(lfs-lar<=SWS)
      {
        lfs++;
        len = sendto(s,&data[index++], sizeof(int), 0,(struct sockaddr *)&client_addr, client_addr_len);
        start_vals[index]=clock();       
      }
      //will comeout of loop if timeout of ack or lost
      {
        len = sendto(s,&data[lar+1], sizeof(int), 0,(struct sockaddr *)&client_addr, client_addr_len);
        start_vals[lar+1]=clock();
      }
      
    }
  }

  void * receive_acks()
  {
    int length,i;
    int ack_num;
    length = recvfrom(s,&ack_num,sizeof(int), 0,(struct sockaddr *)&client_addr, &client_addr_len);
    if(check_timeout(start_vals[ack_num])==0)
    {
      ack_recv[ack_num]=1;
      printf("Adding ack for %d\n",ack_num);
      for(i=lar;i<ack_num;i++)
      {
        if(ack_recv[i+1]==1)
        {
          lar=i;
        }
        else
        {
          break;
        }
      }
      printf("The LAR is %d\n",lar );
    }
    else{
      printf("ACK no. %d received after timeout\n",ack_num );
    }
    fflush(stdout);
  }

  int check_timeout(clock_t start)
  {
    clock_t curr=clock();
    if((curr-start)>=(TIMEOUT*CLOCKS_PER_SEC))
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
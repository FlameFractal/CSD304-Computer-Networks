/*
	LAB 3 SUBMISSION

	USAGE: 

	TEAM MEMBERS
	--------------------
	Prerna - 1410110306
	Vishal Gauba - 1410110501
	Pranjal Mathur - 1410110296
	Saketh Vallakatla - 1410110352
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
     #include "structures.h"

     #define SERVER_PORT 5432
     #define BUF_SIZE 256
     #define SWS 3
     #define TIMEOUT 1
//ADD MUTEX LOCKS
  //   #define SEND_SIZE 10000
     /*FUNCTION PROTOTYPES*/
int check_timeout(clock_t start,int num);
void * receive_acks();

    /*Function Definitions*/
int type_find(void * buf)
{
  f_req * var;
  var=buf;
  printf("%d\n",var->type);
  return var->type;
}

int getSize(FILE * fp)
{
  FILE * tem=fp;
  int size;
  fseek(tem,0L,SEEK_END);
  size=ftell(tem);
  rewind(fp);
  return size;
}

f_nf init_f_nf(f_nf a,int b,char * c)
{
  a.type=4;
  a.filename_size=b;
  memcpy(&a.filename,c,b);
  return a;
}

f_data init_f_data(f_data a,int b,int c,char * d)
{
  a.type=3;
  a.sequence_number=b;
  a.block_size=c;
  memcpy(&a.data,d,c);
  return a;
}

f_info init_f_info(f_info a,int b,int c,char * d,int e,int f,char * g)
{
  a.type=2;
  a.sequence_number=b;
  a.filename_size=c;
  memcpy(&a.filename,d,c);
  a.file_size=e;
  a.block_size=f;
  memcpy(&a.data,g,f);
  return a;
}

     /*GLOBAL VARIABLES*/
pthread_mutex_t count_mutex;
int lar,lfs,sendSize,fileSize;
int no_of_blocks;
clock_t * start_vals;
int * ack_recv;
FILE * fp;
char buf[BUF_SIZE];
f_req * fileReqstd;
ack * ackRecvd;
f_nf ReqNF;
f_info fileInfo;
f_data fileData;
     /*variables for UDP setup*/
int udpSocket;

struct sockaddr_in serverAddr,clientAddr;

struct sockaddr_storage serverStorage;
socklen_t addr_size,client_addr_size;
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char const *argv[])
{
  char blank[1000];
  void * temp;
  temp=(void *)(&blank);
  int len=0;
  int index,i;
  pthread_t ack_thread;
  lar=0;
  lfs=0;
//  ack_recv[0]=1;

        /*create socket*/
  if ((udpSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server: socket");
    exit(1);
  }

     /*configuration of server address*/
  serverAddr.sin_family=AF_INET;
  serverAddr.sin_port=htons(5432);
  serverAddr.sin_addr.s_addr=INADDR_ANY;

     /*binding socket to address structure*/
  bind(udpSocket,(struct sockaddr *)&serverAddr,sizeof(serverAddr));

     /*initialize client address size*/
  addr_size=sizeof(serverStorage);
  printf("Client needs to send \"GET\" to receive the file %s\n", argv[1]);
  //Receive GET message
  recvfrom(udpSocket,buf,BUF_SIZE, 0,(struct sockaddr *)&serverStorage, &addr_size);
  
  while(strcmp(buf,"GET")!=0)
  {}

printf("out of while loop\n");

  //Receive filename request
recvfrom(udpSocket,temp,sizeof(blank), 0,(struct sockaddr *)&serverStorage, &addr_size);
int type=type_find(temp);

index=1;

if(type==0)
{
  fileReqstd=temp;
  fp=fopen(fileReqstd->filename,"rb");
  if(fp==NULL)
  {
    ReqNF=init_f_nf(ReqNF,fileReqstd->filename_size,fileReqstd->filename);
    sendto(udpSocket,&(ReqNF), sizeof(f_nf), 0,(struct sockaddr *)&serverStorage, addr_size);
  }
  else
  {
      //create ack_thread
    pthread_create(&ack_thread,NULL,receive_acks,NULL);
    printf("Created new thread\n");
    fileSize=getSize(fp);
    if((fileSize%BUF_SIZE)==0)
    {
      sendSize=(fileSize/BUF_SIZE)-1;     
    }
    else
    {
      sendSize=(fileSize/BUF_SIZE);    
    }
      //initializing ack_buffer
    printf("cp11\n");  
    ack_recv=calloc(sendSize+1,sizeof(int));
   //  for(i=1;i<=sendSize;i++)
   //  {
   //   ack_recv[i]=0;
   // }

  //initialize time buffers
    start_vals=malloc(sizeof(clock_t)*(sendSize+1));
    printf("cp12\n");
    //initialize data buffer
    char * data=malloc(sizeof(char)*BUF_SIZE*sendSize);
    printf("cp13\n");

    int count_read=fread(buf,1,BUF_SIZE,fp);
    printf("%d\n",count_read );
    fprintf(stdout, "%s\n",buf );
    fileInfo=init_f_info(fileInfo,index,fileReqstd->filename_size,fileReqstd->filename,fileSize,BUF_SIZE,buf);
    len=sendto(udpSocket,&(fileInfo), sizeof(f_info), 0,(struct sockaddr *)&serverStorage, addr_size);
    count_read=fread(buf,1,BUF_SIZE,fp);
    while(index<sendSize||(lar<=sendSize))
    {
      while((lfs-lar<=SWS)&&(index<sendSize))
      {    
        fileData=init_f_data(fileData,index,count_read,buf);
        sendto(udpSocket,&(fileData), sizeof(f_data), 0,(struct sockaddr *)&serverStorage, addr_size);
        start_vals[index]=clock();
        lfs++;
        index++;
        memcpy(&data[(lfs%SWS)*BUF_SIZE],buf,BUF_SIZE);
        count_read=fread(buf,1,BUF_SIZE,fp);    
        if(index>sendSize)
        {
         break;
       }
       if(len==-1)
       {
        perror("Sendto error:");
      }
    }

    printf("ENtering wait loop\n");
    int lar_prev=lar+1;
    while((check_timeout(start_vals[lar+1],lar+1)==0)&&(ack_recv[lar+1]==0)){}
    printf("Exited wait loop\n");
    //will come out of loop either on timeout or on receiving the ack
    if((ack_recv[lar_prev]==0)&&(check_timeout(start_vals[lar_prev],lar_prev)==1))
    {
      //mutex lock
     printf("Retransmitting %d while lfs=%d and lar=%d and SWS=%d\n",(lar+1),lfs,lar,SWS);
     fileData=init_f_data(fileData,lar+1,BUF_SIZE,&data[(lar+1)%SWS]);
     sendto(udpSocket,&(fileData), sizeof(f_data), 0,(struct sockaddr *)&serverStorage, addr_size);
     start_vals[lar+1]=clock();
     //mutex unlock
     printf("Added new transmit time as %d\n",start_vals[lar+1] );
   }
 }
}
}
else
{
  printf("No request received and quitting\n");
  exit(0);  
}


}

void * receive_acks()
{
  printf("Inside new thread\n");
  fflush(stdout);
  printf("cp1\n");
  char blank2[1000];
  void * temp2;
  temp2=(void *)(&blank2);

  int length,i,check,type_recv;
  int ack_num;
  while(1){
    printf("cp2\n");
    length = recvfrom(udpSocket,temp2,sizeof(blank2), 0,(struct sockaddr *)&serverStorage, &addr_size);
    type_recv=type_find(temp2);
    if(type_recv==1)
    {
      ackRecvd=temp2;
      ack_num=ackRecvd->sequence_no[0];
      
      //see if the ack was received within the timeout of the frame
      //mutex lock
      if(check_timeout(start_vals[ack_num],ack_num)==0)
      {
       ack_recv[ack_num]=1;
       check=1;
       printf("Added ack for %d\n",ack_num );
       //to check for frame number to be retransmitted
       for(i=lar+1;i<=ack_num;i++)
       {
        if(ack_recv[i]==0)
        {
         check=0;
         printf("ACK missing for %d\n",i );
         lar=i-1;
         break;
       }
     }
//mutex unlock
     if(check==1)
     {
      lar=ack_num;
    }
          // printf("The LAR is %d\n",lar );
  }
  else{
    printf("ACK no. %d received after timeout\n",ack_num );
  }
  memset(&ack_num,'\0',sizeof(int));
  fflush(stdout); 
}
else{
  printf("Received from client unexpected or undecodable message\n");
  exit(0);
}

}
}

//returns 1 if timed out and 0 if there is time left
int check_timeout(clock_t start,int num)
{
  clock_t curr=clock();
    //printf("current time is %d\n",(int)curr );
  if((curr-start)>=(TIMEOUT*CLOCKS_PER_SEC))
  {
//     printf("Timeout occured for %d \n",num);
   return 1;
 }
 else
 {
  return 0;
}
}

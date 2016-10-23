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
     #define BUF_SIZE 1024
     #define SWS 50
     #define TIMEOUT 1
//ADD MUTEX LOCKS
  //   #define SEND_SIZE 10000
     /*FUNCTION PROTOTYPES*/
int check_timeout(clock_t start,clock_t current);
void * receive_acks();
void * send_data();
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
pthread_mutex_t mutex;
int lar,lfs,sendSize,fileSize;
int no_of_blocks;
clock_t * start_vals;
int * ack_recv;
FILE * fp,* log_fp;
char buf[BUF_SIZE];
f_req * fileReqstd;
ack * ackRecvd;
f_nf ReqNF;
f_info fileInfo;
f_data fileData;
char blank[1000];
void * temp;
int len=0;
int sending_index,i;
pthread_t ack_thread,main_thread;

     /*variables for UDP setup*/
int udpSocket;

struct sockaddr_in serverAddr,clientAddr;

struct sockaddr_storage serverStorage;
socklen_t addr_size,client_addr_size;
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char const *argv[])
{
  temp=(void *)(&blank);
  lar=0;
  lfs=0;
  pthread_mutex_init(&mutex,NULL);
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

pthread_create(&main_thread,NULL,send_data,NULL);
printf("Created the data sending thread\n");
pthread_join(main_thread,NULL);
pthread_join(ack_thread,NULL);
}

void * send_data()
{
  recvfrom(udpSocket,temp,sizeof(blank), 0,(struct sockaddr *)&serverStorage, &addr_size);
  int type=type_find(temp);

  sending_index=1;

  if(type==0)
  {
    fileReqstd=temp;
    fp=fopen(fileReqstd->filename,"rb");
    log_fp=fopen("log.txt","wb");
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
      //initialize ack_recv buffer
      for (i = 0; i <= sendSize; i++)
      {
        ack_recv[i]=0;
      }
    //initialize time buffers
      start_vals=malloc(sizeof(clock_t)*(sendSize+1));
      printf("cp12\n");
      //initialize data buffer
      char * data=malloc(sizeof(char)*BUF_SIZE*sendSize+1);
      int data_size[sendSize+1];
      printf("cp13\n");
      //start reading from file
      rewind(fp);     
      memset(buf,'\0',sizeof(buf));
      int count_read=fread(buf,1,BUF_SIZE,fp);
      printf("count_read_initial=%d\n",count_read);
      fwrite(buf,1,BUF_SIZE,log_fp);
      fileInfo=init_f_info(fileInfo,sending_index,fileReqstd->filename_size,fileReqstd->filename,fileSize,count_read-1,buf);
      fclose(log_fp);
      len=sendto(udpSocket,&(fileInfo), sizeof(f_info), 0,(struct sockaddr *)&serverStorage, addr_size);
      
      while(sending_index<=sendSize||(lar<=sendSize))
      {
        while((lfs-lar<=SWS)&&(sending_index<=sendSize))
        {    
          memset(buf,'\0',sizeof(buf));
          count_read=fread(buf,1,BUF_SIZE,fp);
          fileData=init_f_data(fileData,sending_index,count_read-1,buf);
          data_size[sending_index]=count_read;
          printf("count_read of %d is %d\n",sending_index,data_size[sending_index]);
          sendto(udpSocket,&(fileData), sizeof(f_data), 0,(struct sockaddr *)&serverStorage, addr_size);
          start_vals[sending_index]=clock();
          lfs++;
          sending_index++;
          memcpy(&data[(lfs%SWS)*BUF_SIZE],buf,count_read);
          
          if(sending_index>sendSize)
          {
           break;
         }
         if(len==-1)
         {
          perror("Sendto error:");
        }
      }

      printf("Entering wait loop\n");
      int lar_prev=lar+1;
      printf("BEfore loop lar_prev=%d and lar is %d and ack status was %d\n",lar_prev,lar,ack_recv[lar_prev]);
      while((check_timeout(start_vals[lar_prev],clock())==0)&&(ack_recv[lar_prev]==0)){}
      printf("Timeoout value=%d and ack status =%d. Hence lar_prev=%d and lar_new=%d and lfs=%d \n", check_timeout(start_vals[lar_prev],clock()),ack_recv[lar_prev],lar_prev,lar,lfs);
      printf("Exited wait loop\n");
      //will come out of loop either on timeout or on receiving the ack
      
      if((ack_recv[lar_prev]==0)&&(check_timeout(start_vals[lar_prev],clock())==1))
      {
       
        printf("Entered this area \n");
        fflush(stdout);
        //mutex lock
       pthread_mutex_lock(&mutex);
       printf("Retransmitting %d while lfs=%d and lar=%d and SWS=%d\n",(lar+1),lfs,lar,SWS);
       fileData=init_f_data(fileData,lar+1,data_size[lar+1],&data[(lar+1)]);
       sendto(udpSocket,&(fileData), sizeof(f_data), 0,(struct sockaddr *)&serverStorage, addr_size);
       start_vals[lar+1]=clock();
       pthread_mutex_unlock(&mutex);
       //mutex unlock
     printf("Added new transmit time \n");
     }
      if (lar==sendSize)
   {
     exit(0);
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
  int lar_temp=0;
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
      pthread_mutex_lock(&mutex);
      clock_t curr_for_check=clock();
      if(check_timeout(start_vals[ack_num],curr_for_check)==0)
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
     if(check==1)
     {
      lar=ack_num;
    }
    while(ack_recv[lar+1]==1)
    {
      lar++;
    }

          printf("The LAR is %d\n",lar );
  }
  else{
    printf("ACK no. %d received after timeout\n",ack_num );
  }
  pthread_mutex_unlock(&mutex);
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
int check_timeout(clock_t start,clock_t current)
{
      //printf("current time is %d\n",(int)curr );
  if((current-start)>=(TIMEOUT*CLOCKS_PER_SEC))
  {
//     printf("Timeout occured for %d \n",num);
   return 1;
 }
 else
 {
  return 0;
}
}

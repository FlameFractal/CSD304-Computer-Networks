#include <stdio.h>
#include <pthread.h>
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
#define BUF_SIZE 20000
#define SWS 10	
#define TIMEOUT 3

int check_timeout(clock_t start);
void * receive_acks();
void * send_data();
/*Function Definitions*/
int type_find(void * buf)
{
	f_req * var;
	var=buf;
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
int transmission_count=0;
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
	
	//Wait for the client to send 'GET' message
	while(strcmp(buf,"GET")!=0)
	{}
	
	//create data sending thread receiving thread
	pthread_create(&main_thread,NULL,send_data,NULL);
	printf("Created the data sending thread\n");
	pthread_join(main_thread,NULL);
	pthread_join(ack_thread,NULL);
}

void * send_data()
{
	//receive the name of the file to be sent
	recvfrom(udpSocket,temp,sizeof(blank), 0,(struct sockaddr *)&serverStorage, &addr_size);
	
	int type=type_find(temp);
	
	if(type==0)
	{
		
		fileReqstd=temp;
		fp=fopen(fileReqstd->filename,"rb");
		//if the file requested doesnt exist send the file not found message
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
			//determining the fileSize
			if((fileSize%BUF_SIZE)==0)
			{
				sendSize=(fileSize/BUF_SIZE)-1;
			}
			else
			{
				sendSize=(fileSize/BUF_SIZE);
			}
			printf("Send Size=%d\n",sendSize);
			
			//initializing ack_buffer
			ack_recv=calloc(sendSize+1,sizeof(int));
			
			//initialize ack_recv buffer
			for (i = 0; i <= sendSize; i++)
			{
				ack_recv[i]=0;
			}
			
			//initialize time buffers
			start_vals=malloc(sizeof(clock_t)*(sendSize+5));
			
			//initialize data buffer
			char * data=malloc(sizeof(char)*BUF_SIZE*(sendSize+5));
			int data_size[sendSize+1];
			
			//start reading from file
			rewind(fp);
			memset(buf,'\0',sizeof(buf));
			int count_read=fread(buf,1,BUF_SIZE,fp);
			printf("count_read_initial=%d\n",count_read);
			fileInfo=init_f_info(fileInfo,0,fileReqstd->filename_size,fileReqstd->filename,fileSize,count_read,buf);
			
			//send the initial file_info message
			len=sendto(udpSocket,&(fileInfo), sizeof(f_info), 0,(struct sockaddr *)&serverStorage, addr_size);
			
			sending_index=1;
			
			while((sending_index<=sendSize)||(lar<=sendSize))
			{
				while((lfs-lar<SWS)&&(sending_index<=sendSize))
				{
					
					memset(buf,'\0',sizeof(buf));
					count_read=fread(buf,1,BUF_SIZE,fp);
					if(count_read==0)
					{
						printf("error reading\n");
						exit(0);
					}
					
					fileData=init_f_data(fileData,sending_index,count_read,buf);
					data_size[sending_index]=count_read;
					
					//sending the data packet
					len=sendto(udpSocket,&(fileData), sizeof(f_data), 0,(struct sockaddr *)&serverStorage, addr_size);
					//printf("sent packet num. %d of block size=%d and send returned %d\n",sending_index,fileData.block_size,len);
					pthread_mutex_lock(&mutex);
					start_vals[sending_index]=clock();
					lfs++;
					sending_index++;
					memcpy(&data[(lfs)*BUF_SIZE],buf,count_read);
					pthread_mutex_unlock(&mutex);
					//printf("sent %d with %ld \n",sending_index-1,start_vals[sending_index-1]);
					
					if(sending_index>sendSize)
					{
						break;
					}
					if(len==-1)
					{
						perror("Sendto error:");
					}
				}
				
				pthread_mutex_lock(&mutex);
				int numToBeRecvd=lar+1;
				pthread_mutex_unlock(&mutex);
				//printf("Before wait state for %d\n",numToBeRecvd);
					
				while((check_timeout(start_vals[numToBeRecvd])==0)&&(ack_recv[numToBeRecvd]==0)){}
				//will come out of loop either on timeout or on receiving the ack
				
				//printf("out of wait state\n");
				pthread_mutex_lock(&mutex);
				if((ack_recv[numToBeRecvd]==0)&&(check_timeout(start_vals[numToBeRecvd])==1))
				{
					fileData=init_f_data(fileData,numToBeRecvd,data_size[numToBeRecvd],&data[(numToBeRecvd)]);
					sendto(udpSocket,&(fileData), sizeof(f_data), 0,(struct sockaddr *)&serverStorage, addr_size);
					start_vals[numToBeRecvd]=clock();
					//printf("set new clock value as %ld\n",start_vals[numToBeRecvd]);
					transmission_count++;
					printf("Retransmitted %d while lfs=%d and lar=%d \n",numToBeRecvd,lfs,lar);
					
				}
				pthread_mutex_unlock(&mutex);
				if (lar==sendSize)
				{
					return NULL;
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
	printf("acknowledgement receiving thread\n");
	fflush(stdout);
	char blank2[1000];
	void * temp2;
	temp2=(void *)(&blank2);
	
	int length,i,check,type_recv;
	int lar_temp=0;
	int ack_num;
	while(lar<sendSize){
		length = recvfrom(udpSocket,temp2,sizeof(blank2), 0,(struct sockaddr *)&serverStorage, &addr_size);
		type_recv=type_find(temp2);
		if(type_recv==1)
		{
			ackRecvd=temp2;
			ack_num=ackRecvd->sequence_no[0];
							//see if the ack was received within the timeout of the frame
			if((ack_num>lar)&&(ack_num<=lfs))
			{	
				pthread_mutex_lock(&mutex);
				int check_value=check_timeout(start_vals[ack_num]);
				//printf("Received ack for %d\n",ack_num );
				pthread_mutex_unlock(&mutex);
				
				if(check_value==0)
				{
					//setting the ack_recv value as 1 if the acknowledgement was received before timeout
					pthread_mutex_lock(&mutex);
					ack_recv[ack_num]=1;
					pthread_mutex_unlock(&mutex);
					check=1;
					
					//to check for missing ack values before the received ack
					for(i=lar+1;i<=ack_num;i++)
					{
						if(ack_recv[i]==0)
						{
							check=0;
							//check==0 indicates ack is missing for some frame
							pthread_mutex_lock(&mutex);
							lar=i-1;
							pthread_mutex_unlock(&mutex);
							break;
						}
					}
					
					if(check==1)
					{
						pthread_mutex_lock(&mutex);
						lar=ack_num;
						pthread_mutex_unlock(&mutex);
					}
					
					//the following accounts for acks received in advance but not taken into account due to previous missing acks
					while(ack_recv[lar+1]==1)
					{
						pthread_mutex_lock(&mutex);
						lar++;
						pthread_mutex_unlock(&mutex);
					}
					
					if (lar==sendSize)
					{
						printf("Sending BYE\n");
						sendto(udpSocket,"BYE", sizeof("BYE")+1, 0,(struct sockaddr *)&serverStorage, addr_size);
						printf("Number of retransmissions=%d\n",transmission_count);
						exit(0);
					}
					
				}
				else{
					printf("ACK no. %d received after timeout\n",ack_num );
				}
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
int check_timeout(clock_t start)
{
	clock_t diff;
	clock_t curr=clock();
	diff=curr-start;
	//printf("current time is %d\n",(int)curr );
	//TIMEOUT in seconds
	if((diff/CLOCKS_PER_SEC)>TIMEOUT)
	{
		printf("Timeout with start as %ld and curr as %ld \n",start,curr);
		return 1;
	}
	else
	{
		return 0;
	}
}

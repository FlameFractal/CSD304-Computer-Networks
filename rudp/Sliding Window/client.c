#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "structures.h"

#define SERVER_PORT 5432
#define BUF_SIZE 10240
#define RWS 50

int lfr,laf,incoming_size;
FILE *output_file;
f_info *infoRecvd;
f_data *dataRecvd;
f_nf * fnfRecvd;
f_req fileReq;
ack ackSending;

f_req init_f_req(f_req a,int b,char * c)
{
	a.type=0;
	a.filename_size=b;
	strcpy(a.filename,c);
	return a;
}

ack init_ack(ack a,int b)
{
	a.type=1;
	a.num_sequences=1;
	a.sequence_no[0]=b;
	return a;
}

int type_find(void * buf)
{
	f_info * var;
	var=buf;
	printf("\n");
	return var->type;
}


int main(int argc, char * argv[]){
	
	char * file_name_input="video_coke.avi";
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	int s;
	int len;
	char blank[65000];
	void * temp;
	temp=(void *)(&blank);
	int length;
	int index=-1;
	int type_recvd;
	int num_recvd;
	int written;
	int content_write=0;
	int * ack_counter;
	int i;
	
	if ((argc==2)||(argc == 3)) {
		host = argv[1];
	}
	else {
		fprintf(stderr, "usage: client serverIP [download_filename(optional)]\n");
		exit(1);
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
	socklen_t client_addr_len = sizeof(sin);
	
	/* create socket */
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("client: socket");
		exit(1);
	}
	
	printf("Client will get data from to %s:%d.\n", argv[1], SERVER_PORT);
	char buf[BUF_SIZE];
	fgets(buf,BUF_SIZE,stdin);
	strtok(buf,"\n");
	fileReq=init_f_req(fileReq,strlen(buf),buf);
	
	//send File Request
	sendto(s,&fileReq,sizeof(f_req), 0, (struct sockaddr *)&sin, sizeof(sin));
	
	//Receive info/File not found message from server
	memset(blank,'\0',sizeof(blank));
	recvfrom(s,temp,sizeof(blank), 0,(struct sockaddr *)&sin, &client_addr_len);
	type_recvd=type_find(temp);
	
	if(type_recvd==4)
	{
		printf("No such file found\n");
	}
	
	else
	{
		output_file=fopen(file_name_input,"wb");
		
		infoRecvd=temp;
		if(((infoRecvd->file_size)%(infoRecvd->block_size))==0)
		{
			incoming_size=((infoRecvd->file_size)/(infoRecvd->block_size))-1;
		}
		else
		{
			incoming_size=((infoRecvd->file_size)/(infoRecvd->block_size));
		}
		fwrite(&infoRecvd->data,1,infoRecvd->block_size,output_file);
		printf("Determined incoming_size is %d\n",incoming_size);
		
		//set up ack_counter
		ack_counter=malloc(sizeof(int)*(incoming_size+1));
		for(i=0;i<=incoming_size;i++)
		{
			ack_counter[i]=0;
		}
		
		//initialize data buffer
		char data_buffer[infoRecvd->block_size*RWS];
		int blocksize_recvd[RWS];
		int end_packet_num;
		memset(data_buffer,'\0',sizeof(data_buffer));
		laf=RWS;
		written=0;
		lfr=0;
		
		while(1)
		{
			memset(blank,'\0',sizeof(blank));
			length = recvfrom(s,temp,sizeof(blank), 0,(struct sockaddr *)&sin, &client_addr_len);
			if(strcmp(temp,"BYE")==0)
			{
				printf("Received BYE\n");
				exit(0);
			}
			type_recvd=type_find(temp);
			if (type_recvd==3)
			{
				dataRecvd=temp;
				num_recvd=dataRecvd->sequence_number;
				printf("Received %d of size %d and laf is %d and incoming_size is %d \n",num_recvd,dataRecvd->block_size,laf,incoming_size);
				
				//check if the incoming frame lies in the acceptable range
				if((length>0)&&(num_recvd<laf))
				{
					{
						//
						if((dataRecvd->block_size)<BUF_SIZE && (dataRecvd->block_size)>0)
						{
							end_packet_num=num_recvd;
							printf("Last packet Received\n");
						}
						
						ackSending=init_ack(ackSending,num_recvd);
						//sending the acknowledgement
						//if(num_recvd!=2)
						{
							length = sendto(s,&ackSending, sizeof(ack), 0, (struct sockaddr *)&sin, sizeof(sin));
						}
						memcpy(&data_buffer[(num_recvd*infoRecvd->block_size)%RWS],&dataRecvd->data,dataRecvd->block_size);
						blocksize_recvd[num_recvd%RWS]=dataRecvd->block_size;
						printf("Sent ack for %d\n",num_recvd );
							if(written<incoming_size)
						{
							ack_counter[num_recvd]=1;
							
							for(i=written;i<=num_recvd;i++)
							{
								if(ack_counter[i+1]==1)
								{
									lfr=i+1;
									laf++;
								}
								else
								{
									break;
								}
							}
							
							if(lfr!=written)
							{
								for (i = written+1; i <= lfr;i++)
								{
									printf("Writing block %d to file\n",i);
									//fputs(&data_buffer[((i*(infoRecvd->block_size))%RWS)],stdout);
									fwrite(&data_buffer[((i*(infoRecvd->block_size))%RWS)],1,blocksize_recvd[i%RWS],output_file);
								}
								
							}
							
							written=lfr;
							
							if(written==end_packet_num)
							{
								fclose(output_file);
								//exit(0);
							}
						}
					}
					
					//printf("lfr=%d\n",lfr );
				}
			}
			else
			{
				printf("Received something other than data block\n");
				exit(0);
			}
			
		}
		//		fclose(output_file);
		exit(0);
	}
}

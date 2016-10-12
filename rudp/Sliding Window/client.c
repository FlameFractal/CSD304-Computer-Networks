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
#include <arpa/inet.h>
#include "structures.h"

#define SERVER_PORT 5432
#define BUF_SIZE 256
#define RWS 10

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
	printf("%d\n",var->type);
	return var->type;
}


int main(int argc, char * argv[]){

	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	int s;
	int len;
	char blank[1000];
	void * temp;
	temp=(void *)(&blank);
	int length;
	int index=-1;
	int type_recvd;
	int num_recvd;
	int written=0;
	int content_write=0;
	int * ack_counter;
	char * file_name_input="in.txt";
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
	fileReq=init_f_req(fileReq,strlen(file_name_input),file_name_input);
 	//Send GET 
	sendto(s,buf,strlen(buf)+1, 0, (struct sockaddr *)&sin, sizeof(sin)); 
 	//send File Request
	sendto(s,&fileReq,sizeof(f_req), 0, (struct sockaddr *)&sin, sizeof(sin)); 
	recvfrom(s,temp,sizeof(blank), 0,(struct sockaddr *)&sin, &client_addr_len);
	type_recvd=type_find(temp);
	if(type_recvd==4)
	{
		printf("No such file found\n");
	}
	else
	{
		output_file=fopen("out.txt","wb");
		infoRecvd=temp;
		if((infoRecvd->file_size%infoRecvd->block_size)==0)
		{
			incoming_size=((infoRecvd->file_size)/(infoRecvd->block_size))-1;
		}
		else
		{
			incoming_size=((infoRecvd->file_size)/(infoRecvd->block_size));
		}
		fwrite(&infoRecvd->data,1,infoRecvd->block_size,output_file);
 		//set up ack_counter
		ack_counter=malloc(sizeof(int)*(incoming_size+1));
		for(i=0;i<=incoming_size;i++)
		{
			ack_counter[i]=0;
		}
 		//initialize data buffer
//		printf("malloced data buffer and memseted with sizeof data_buffer as %d while it should be %d\n",sizeof(*data_buffer),(sizeof(char)*infoRecvd->block_size)*RWS);
		char data_buffer[infoRecvd->block_size*RWS];
		memset(data_buffer,'\0',sizeof(data_buffer));
		laf=RWS;
		while(lfr<incoming_size)
		{
			length = recvfrom(s,temp,sizeof(blank), 0,(struct sockaddr *)&sin, &client_addr_len);
			type_recvd=type_find(temp);
			if (type_recvd==3)
			{
				dataRecvd=temp;
				num_recvd=dataRecvd->sequence_number;

				printf("Received %d and laf is %d \n",num_recvd,laf);
				if((length>0)&&(num_recvd<=laf))
				{
					{
						ackSending=init_ack(ackSending,num_recvd);
						length = sendto(s,&ackSending, sizeof(ack), 0, (struct sockaddr *)&sin, sizeof(sin)); 
						memcpy(&data_buffer[(num_recvd*infoRecvd->block_size)%RWS],&dataRecvd->data,infoRecvd->block_size);
						printf("Sending ack for %d\n",num_recvd );
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
								if(fwrite(&data_buffer[(i*(infoRecvd->block_size)%RWS)],1,dataRecvd->block_size,output_file)<0){
									printf("fuckeeeeeeeeeeeeeeed\n");
								}
							}
						}
						written=lfr; 
					}  
				}
				printf("lfr=%d\n",lfr );
			}

			else
			{
				printf("Received something other than data block\n");
				exit(0);
			}

		}
		fclose(output_file);
		exit(0);
}
}
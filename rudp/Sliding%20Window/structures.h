#include<stdio.h>
#include<stdlib.h>

#define BUF_SIZE 1024
//from client to server
;

typedef struct file_request_message
{
	uint8_t type;
	uint8_t filename_size;
	char filename[BUF_SIZE];
} f_req;
  //type=0

typedef struct acknowledgement
{
 uint8_t type;
 uint8_t num_sequences;
 uint16_t sequence_no[BUF_SIZE];
} ack;
  //type=1

  //from server to client
typedef struct File_info_and_data
{
  uint8_t type  ;
  uint16_t sequence_number;
  uint8_t filename_size;
  char filename[BUF_SIZE];
  uint32_t file_size;
  uint16_t block_size;
  char data[BUF_SIZE];
} f_info;
//type=2

typedef struct data{
  uint8_t type  ;
  uint16_t sequence_number;
  uint16_t block_size;
  char data[BUF_SIZE];
} f_data;
//type=3

typedef struct file_not_found{
  uint8_t type  ;
  uint8_t filename_size;
  char filename[BUF_SIZE];
}f_nf;
//type=4
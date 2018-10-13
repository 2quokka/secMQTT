#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lwmqtt/unix.h>

#define COMMAND_TIMEOUT 5000
#define MESSAGE_TIMEOUT 1000

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

lwmqtt_unix_network_t network = {0};

lwmqtt_unix_timer_t timer1, timer2, timer3;

lwmqtt_client_t client;

static void message_arrived(lwmqtt_client_t *client, void *ref, lwmqtt_string_t topic, lwmqtt_message_t msg, char **id_list, int idx) {
	if( idx < 0 )
		printf(RED " [ INVALID MESSAGE ]\n" RESET);
	else{
		printf(GRN " [ ");
		for (int i = 0 ; i < topic.len ; i++){
			printf("%c", topic.data[i]);
		}
		printf(" ]\n" RESET);
	}

	printf(" FUNCTION : ");
	for (size_t i = 0; i < msg.payload_len; i++)
		printf("%c", *(msg.payload+i));
	printf("\n");

	printf(" HASH VAL : ");
	for (size_t i = 0; i < msg.digest_len; i++)
		printf("%02X ", *(msg.digest+i));

	printf("\n\n");
}

int init_idlist(char **id_list, int num, char *filename){

  char s_buf[256];
  char buf;
  int rd, i, fd, id_num = 0;

  if ( (fd = open(filename, O_RDONLY)) < 0 ){
	  perror("file open error\n");
	  return  -1;
  }
 
  memset(s_buf, 0, 256);

  i = 0;
  while((rd = read(fd, &buf, 1)) > 0){	  
	  if( buf == '\n' ){
		 s_buf[i] = '\0';
		 id_list[id_num] = (char *)malloc( strlen(s_buf)+1 ); 
		 strcpy(id_list[id_num++], s_buf);
		 //printf("%s \n", s_buf);

		  if( id_num > num-1 ){
			  printf("id_list is full\n");
			  break;
		  }
		  
		  i = 0;
		  continue;
	  }
	  s_buf[i++] = buf;
  }
  close(fd);
  return id_num;
}

int main(int argc, char *argv[]) {
	char host[30];
	char *id_list[50];
	lwmqtt_string_t *topics;
	lwmqtt_qos_t *qoss;

  if ( argc <3 ) {
	  fprintf(stdout, "Usage : host topics \n");
	  exit(-1);
  }

  memset(id_list, 0, sizeof(char *)*50);

  if ( init_idlist(id_list, 50, "list.txt") < 0 ) { 
	  printf("fail to initialize idlist \n ");
	  exit(-1);
  }
  
  strcpy(host, argv[1]);

  topics = (lwmqtt_string_t *)malloc(sizeof(lwmqtt_string_t)*(argc-2));
  qoss = (lwmqtt_qos_t *)malloc(sizeof(lwmqtt_qos_t)*(argc-2));

  printf("argc : %d \n", argc);
  for(int i = 2; i < argc; i++){
	 topics[i-2] = lwmqtt_string(argv[i]); 
	 qoss[i-2] = LWMQTT_QOS0;
	 
	 printf("topics[%d] : %s \n", i-2, topics[i-2].data);
  }


  // initialize client
  lwmqtt_init(&client, malloc(512), 512, malloc(512), 512);

  // configure client
  lwmqtt_set_network(&client, &network, lwmqtt_unix_network_read, lwmqtt_unix_network_write);
  lwmqtt_set_timers(&client, &timer1, &timer2, lwmqtt_unix_timer_set, lwmqtt_unix_timer_get);
  lwmqtt_set_callback(&client, NULL, message_arrived);

  // configure message time
  lwmqtt_unix_timer_set(&timer3, MESSAGE_TIMEOUT);

  lwmqtt_err_t err = lwmqtt_unix_network_connect(&network, host, 1883);
  if (err != LWMQTT_SUCCESS) {
    printf("failed lwmqtt_unix_network_connect: %d\n", err);
    exit(1);
  }

  // prepare options
  lwmqtt_options_t options = lwmqtt_default_options;
  options.client_id = lwmqtt_string("lwmqtt_print");
  options.keep_alive = 5;

  // send connect packet
  lwmqtt_return_code_t return_code;
  err = lwmqtt_connect(&client, options, NULL, &return_code, COMMAND_TIMEOUT); if (err != LWMQTT_SUCCESS) {
    printf("failed lwmqtt_connect: %d (%d)\n", err, return_code);
    exit(1);
  }

  // log
  printf("connected!\n");


  // subscribe to topic
  err = lwmqtt_subscribe(&client, argc-2, topics, qoss, COMMAND_TIMEOUT);
  //err = lwmqtt_subscribe_one(&client, lwmqtt_string(topic), LWMQTT_QOS0, COMMAND_TIMEOUT);
  if (err != LWMQTT_SUCCESS) {
    printf("failed lwmqtt_subscribe: %d\n", err);
    exit(1);
  }

  // loop forever
  for (;;) {
    // check if data is available
    size_t available = 0;
    err = lwmqtt_unix_network_peek(&network, &available);
    if (err != LWMQTT_SUCCESS) {
      printf("failed lwmqtt_unix_network_peek: %d\n", err);
      exit(1);
    }

    // process data if available
    if (available > 0) {
      err = lwmqtt_yield(&client, available, COMMAND_TIMEOUT, id_list);
      if (err != LWMQTT_SUCCESS) {
		  if(err == LWMQTT_INTEGRITY_INVALID){
			  continue;
		  }
        printf("failed lwmqtt_yield: %d\n", err);
        exit(1);
      }
    }

    // keep connection alive
    err = lwmqtt_keep_alive(&client, COMMAND_TIMEOUT);
    if (err != LWMQTT_SUCCESS) {
      printf("failed lwmqtt_keep_alive: %d\n", err);
      exit(1);
    }
  }
}

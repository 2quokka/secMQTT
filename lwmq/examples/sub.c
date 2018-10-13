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

lwmqtt_unix_network_t network = {0};

lwmqtt_unix_timer_t timer1, timer2, timer3;

lwmqtt_client_t client;

static void message_arrived(lwmqtt_client_t *client, void *ref, lwmqtt_string_t topic, lwmqtt_message_t msg, char **id_list, int idx) {
  printf("message_arrived: %.*s => %.*s (%d)\n", (int)topic.len, topic.data, (int)msg.payload_len-32, (char *)msg.payload,
         (int)msg.payload_len);
}

int init_command(char *argv[], char *host, char *topic){
	int opt = 0;
	argv++;
	while(*argv != NULL){
		if ( !strcmp(*argv, "-h") ){
			strcpy(host, *(++argv));
			opt |= 1; 
		}
		else if ( !strcmp(*argv, "-t") ){
			strcpy(topic, *(++argv));
			opt |= 2;
		}
		else
		{
			if ( opt % 2 == 0) {
				strcpy(host, *argv);
				opt |= 1;
			}
			else if ( opt % 4 == 1) {
				strcpy(topic, *argv);
				opt |= 2;
			}
			else
				return -1;
		}
//		printf("argv : %s \n ", *argv);
//		printf("opt  : %d \n ", opt );
//		printf("host : %s \n", host );
//		printf("topic : %s \n", topic);
		argv++;
	}

	if ( opt >= 3 )
		return 0;
	else
		return -1;
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
		 printf("%s \n", s_buf);

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
	char topic[256];
	char *id_list[50];

  if ( argc <2 ) {
	  fprintf(stdout, "Usage : host topic \n");
	  exit(-1);
  }

  if (  init_command(argv, host, topic) < 0 ){
	  printf("command error \n");
	  exit(-1);
  }

  if ( init_idlist(id_list, 50, "list.txt") < 0 ) { 
	  printf("fail to initialize idlist \n ");
	  exit(-1);
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
  options.client_id = lwmqtt_string("lwmqtt_sub");
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
  err = lwmqtt_subscribe_one(&client, lwmqtt_string(topic), LWMQTT_QOS0, COMMAND_TIMEOUT);
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
			  printf("intergrity error!!\n");
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

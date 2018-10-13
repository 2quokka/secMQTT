#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <lwmqtt/unix.h>

#define COMMAND_TIMEOUT 5000
#define MESSAGE_TIMEOUT 1000

#define MESSAGE_SIZE 10000

lwmqtt_unix_network_t network = {0};

lwmqtt_unix_timer_t timer1, timer2, timer3;

lwmqtt_client_t client;

static void message_arrived(lwmqtt_client_t *client, void *ref, lwmqtt_string_t topic, lwmqtt_message_t msg, char **id_list, int idx) {
  printf("message_arrived: %.*s => %.*s (%d)\n", (int)topic.len, topic.data, (int)msg.payload_len-32, (char *)msg.payload,
         (int)msg.payload_len);
}

int init_command(char *argv[], char *host, char *topic, char *contents, char *key){
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
		else if ( !strcmp(*argv, "-m") ){
			strcpy(contents, *(++argv));
			opt |= 4;
		}
		else if ( !strcmp(*argv, "-k") ){
			strcpy(key, *(++argv));
			opt |= 8;
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
			else if ( opt % 8 == 3) {
				strcpy(contents, *argv);
				opt |= 4;
			}
			else if ( opt % 16 == 7) {
				strcpy(key, *argv);
				opt |= 8;
			}
			else
				return -1;
		}
//		printf("argv : %s \n ", *argv);
//		printf("opt  : %d \n ", opt );
//		printf("host : %s \n", host );
//		printf("topic : %s \n", topic);
//		printf("contents : %s \n", contents );
//		printf("key : %s \n", key );
		argv++;
	}

	if ( opt >= 7 )
		return 0;
	else
		return -1;
}

int main(int argc, char *argv[]) {
	char host[30];
	char topic[256];
	char contents[MESSAGE_SIZE];
	char key[30];

  if ( argc < 3 ) {
	  fprintf(stdout, "Usage : host topic message\n");
	  exit(-1);
  }

  if ( init_command(argv, host, topic, contents, key) < 0 ){
	  printf("command error\n" );
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

  // connect to broker
  lwmqtt_err_t err = lwmqtt_unix_network_connect(&network, host, 1883);
  if (err != LWMQTT_SUCCESS) {
    printf("failed lwmqtt_unix_network_connect: %d\n", err);
    exit(1);
  }

  // prepare options
  lwmqtt_options_t options = lwmqtt_default_options;
  options.client_id = lwmqtt_string("lwmqtt_pub");
  options.keep_alive = 5;

  // send connect packet
  lwmqtt_return_code_t return_code;
  err = lwmqtt_connect(&client, options, NULL, &return_code, COMMAND_TIMEOUT); 
  if (err != LWMQTT_SUCCESS) {
    printf("failed lwmqtt_connect: %d (%d)\n", err, return_code);
    exit(1);
  }

  // log
  printf("connected!\n");
  
  // loop forever

	lwmqtt_message_t *msg = lwmqtt_create_msg(LWMQTT_QOS0, false, (uint8_t *)contents, true, key); 

//	printf("create msg\n");
//	printf("payload : %s \n", msg->payload);
//	printf("len : %d \n", (int)msg->payload_len);
//	printf("hash : %s \n", msg->digest);
//	printf("h_len : %d \n", (int)msg->digest_len);

  // publish message
  err = lwmqtt_publish(&client, lwmqtt_string(topic), *msg, COMMAND_TIMEOUT);

  if (err != LWMQTT_SUCCESS) {
	printf("failed lwmqtt_keep_alive: %d\n", err);
	exit(1);
  }
  return 0;
}

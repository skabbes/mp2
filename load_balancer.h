#ifndef LOAD_BALANCER 
#define LOAD_BALANCER 

#include "server_class.h"
#define BLOCKSIZE 4096

extern pthread_mutex_t server_mutex;
extern int numServers;
extern Server * servers;


void loadBalancer_main(char * port);
void * thread_conn_handler(void * s);


int get_filesize( const char * filename );
int read_bytes( FILE * file, char * b, int len );

void handle_get_file(int socket);

void handle_node_down(int crashed_id);

void handle_register_server(int socket, int registered_id);
/*
void handle_put_file(int socket);
void handle_file_not_found(int socket);
*/

#endif

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <cstdio>
#include <cstdlib>

#include <arpa/inet.h>

#include "socket.h"


using namespace std;

// function definitions
int main(int argc, char ** argv);
void * thread_conn_handler(void * arg);

// global variables definitions for this node
int m;
int id;
int port;

// vector of files
// vector finger table

void * thread_conn_handler(void * arg){
    int socket = *((int *)arg);
    free(arg);

    // do stuff with the socket

    return NULL;
}

int main(int argc, char ** argv){
    if( argc != 5){
       cerr << "Usage: " << argv[0] << " <m> <id> <introducer-host> <introducer-port>" << endl;
       return EXIT_FAILURE;
    }

    m = atoi(argv[1]);
    id = atoi(argv[2]);
    char * introducer_host = argv[3];
    char * introducer_port = argv[4];

    // we don't care what port we bind on, just pick an open one
    int server = setup_server("0", &port);

    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];

    // detached threads
    pthread_attr_t DetachedAttr;
    pthread_attr_init(&DetachedAttr);
    pthread_attr_setdetachstate(&DetachedAttr, PTHREAD_CREATE_DETACHED);

    pthread_t handler;

    while(1) {
        sin_size = sizeof their_addr;

        int new_fd = accept(server, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        cout << "Node got connection from " << s << endl;

        // prepare argument for thread
        int * arg = (int *) malloc( sizeof(int) );
        *arg = new_fd;
        if( pthread_create(&handler, &DetachedAttr, thread_conn_handler, arg) ){
            free(arg);
            perror("pthread_create");
            continue;
        }
        pthread_detach(handler);
    }

    // free resources for detached attribute
    pthread_attr_destroy(&DetachedAttr);
    return 0;
}

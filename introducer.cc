// c++ libs
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// c libs
#include <cstdio>
#include <cstdlib>

#include <arpa/inet.h>

// for sleep function
#include "socket.h"
#include "sha1.h"

using namespace std;

// function definitions
int main(int argc, char ** argv);

// global variables definitions
int m;
char * port;

void * thread_conn_handler(void * arg){
    free(arg);
    return NULL;
}

int main(int argc, char ** argv){

    if( argc != 3){
       cerr << "Usage: " << argv[0] << " <m> <port>" << endl;
       return EXIT_FAILURE;
    }

    // just some samples of SHA1 stuff
    cout << SHA1("hello world", 1)  << endl;
    cout << SHA1("hello world", 2)  << endl;
    cout << SHA1("hello world", 3)  << endl;
    cout << SHA1("hello world", 4)  << endl;
    cout << SHA1("hello world", 5)  << endl;

    m = atoi(argv[1]);
    port = argv[2];

    int server = setup_server(port, NULL);

    // run code for introducer here
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
        cout << "Introducer got connection from " << s << endl;

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

    return EXIT_SUCCESS;
}

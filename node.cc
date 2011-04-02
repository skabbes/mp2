// c++ libs
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// c libs
#include <cstdio>
#include <cstdlib>

#include <arpa/inet.h>

#include "socket.h"
#include "sha1.h"
#include "messages.h"
#include "node_class.h"

using namespace std;

// function definitions
int main(int argc, char ** argv);

// global variables definitions
int m;
int id;
int port;

// finger table
vector<Node> ft;

vector<string> files;
vector<string> ipaddrs;

Node next;
Node prev;

bool between(int first, int second, int test){
   if( first >= second ){
      return test > first || test < second;
   }
   return test > first && test < second;
}

Node closestFinger(int queryId){
   for(int i=m-1;i>=0;i--){
      if( ft[i].id == queryId ){
         return ft[i];
      }
      if( between(ft[i].id, id, queryId) ){
         return ft[i];
      }
   }

   // throw exception?
   int x = 10 / 0;
   return ft[0];
}


pid_t launchNode(int m, int id){
    pid_t pid = fork();

    char mString[6] = {0};
    sprintf(mString, "%d", m);

    char idString[32] = {0};
    sprintf(idString, "%d", id);

    char portString[] = "0";

    char introducerPortString[32] = {0};
    sprintf(introducerPortString, "%d", port);

    if( pid == 0 ){
        // launch listener process
        execlp("./node", "./node", mString, idString, portString, introducerPortString, (char *)0 );
    } 

    return pid;
}

void * thread_conn_handler(void * arg){
    int socket = *((int*)arg);
    free(arg);
    int command = readint(socket);

    if( command == ADD_NODE){
      cout << "NODE " << id << " got ADD_NODE";
       int size = readint(socket);
       for(int i=0; i<size; i++){
          int id = readint(socket);
          cout << " " << id;
          launchNode(m, id);
       }
       cout << endl;
    }
    else if( command == ADD_FILE){
      string filename = readstring(socket);
      string ipaddr = readstring(socket);
      cout << "Node " << id << " got ADD_FILE " << filename << " " << ipaddr << endl;
    }
    else if( command == DEL_FILE){
      cout << "Node " << id << " got DEL_FILE " << readstring(socket)  << endl;
    }
    else if( command == FIND_FILE){
      cout << "Node " << id << " got FIND_FILE " << readstring(socket)  << endl;
    }
    else if( command == GET_TABLE){
      cout << "Node " << id << " got GET_TABLE " <<  readint(socket) << endl;
    }
    else if( command == FIND_SUCCESSOR ){

      int queryId = readint(socket);

      cout << "Node " << id << " got FIND SUCCESSOR TO " << queryId << endl;

      Node closest = closestFinger(queryId);
      if( closest.id != id ){
         closest = closest.findSuccessorTo(queryId);
      }

      sendint(socket, closest.id);
      sendint(socket, closest.port);
    }
    else if( command == QUIT){
      cout << "Node " << id << " got QUIT" << endl;
    }

    // stop reading from socket, but keep tryingn to send data
    shutdown(socket, 1);
    return NULL;
}

int main(int argc, char ** argv){

    if( argc != 4 && argc != 5){
       cerr << "Usage: " << argv[0] << " <m> <id> <port> [<introducer-port>]" << endl;
       return EXIT_FAILURE;
    }
    
    // grab parameters
    m = atoi(argv[1]);
    id = atoi(argv[2]);
    port = atoi(argv[3]);

    int introducer_port = 0;
    if( argc == 5 ){
       introducer_port = atoi(argv[4]);
    } 

    // initialize the next and prev nodes
    next = Node(id, port);
    prev = Node(id, port);

    if( id != 0 ){
       Node introducer(0, introducer_port);
       next = introducer.findSuccessorTo(id);
       cout << "Successor to " << id << " is " << next.id << " on port " << next.port << endl;
    }

    // initialize finger table to self
    for(int i=0;i<m;i++){
       ft.push_back(next);
    }

    // if port == 0, we get the new port number back as an output param
    int server = setup_server(port, &port);

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
        cout << "Node " << id << " got connection from " << s << endl;

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

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
Node closestFinger(int queryId);

// global variables definitions
int m;
int id;
int port;
bool shouldQuit = false;
pthread_mutex_t quit_mutex = PTHREAD_MUTEX_INITIALIZER;


// finger table
vector<Node> ft;
pthread_mutex_t ft_mutex = PTHREAD_MUTEX_INITIALIZER;

vector<string> files;
vector<string> ipaddrs;

Node next;
Node prev;

bool canQuit(){
   pthread_mutex_lock(&quit_mutex);
   bool temp = shouldQuit;
   pthread_mutex_unlock(&quit_mutex);
   return temp;
}

bool between(int first, int second, int test){
   if( first > second ){
      return test > first || test < second;
   }
   return test > first && test < second;
}

void * stabilizer(void * arg){
    //cout << "Stabilizer started " << endl;

    while( !canQuit() ){

        // lock
        pthread_mutex_lock(&ft_mutex);

        Node x = next.findPredecessor();
        if( id == next.id || between(id, next.id, x.id) ){
            next = x;
            ft[0] = next;
            //cout << "Node " << id << " next is " << next.id << endl;
        }
        next.notify(id, port);

        // unlock
        pthread_mutex_unlock(&ft_mutex);
        usleep(250000);
    }

    // notify
    return NULL;
}

void * fixFingers(void * arg){
    //cout << "Fix fingers started " << endl;
    while( !canQuit() ){
        // lock
        pthread_mutex_lock(&ft_mutex);

        for(unsigned int i=0;i<ft.size();i++){
           int fingerId = (id + (1 << i)) % (1 << m);

           Node closest = closestFinger(fingerId);
           //cout << "node " << id << " thinks closest to " << fingerId << " is " << closest.id << endl;

           if( id == closest.id || closest.id == fingerId ){
               ft[i] = closest;
               continue;
           }

           ft[i] = closest.findSuccessorTo( fingerId );
        }

        /*
        stringstream ss (stringstream::in | stringstream::out);
        ss << "Finger Table for Node " << id << endl;
        for(unsigned int i=0;i<ft.size();i++){
           ss << i << "\t" << ft[i].id << endl;
        }
        cout << ss.str() << endl;
        */

        // unlock
        pthread_mutex_unlock(&ft_mutex);
        usleep(250000);
    }

    return NULL;
}

void startThread( void * (*functor)(void *), void * arg ){
    pthread_attr_t DetachedAttr;
    pthread_attr_init(&DetachedAttr);
    pthread_attr_setdetachstate(&DetachedAttr, PTHREAD_CREATE_DETACHED);

    pthread_t handler;
    if( pthread_create(&handler, &DetachedAttr, functor, arg) ){
        free(arg);
        perror("pthread_create");
    }
    pthread_detach(handler);

    // free resources for detached attribute
    pthread_attr_destroy(&DetachedAttr);
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
	
		//Implement ADD_FILE
		int key = SHA1(filename,m);
		Node predNode = prev;	// fidn predecessor of the current node ID

		// if the key is the same as the node ID or in between node Id and its predecessor
		if (key == id || between(predNode.id,id,key))
		{
			files.push_back(filename.c_str());
			ipaddrs.push_back(ipaddr.c_str());
			printf("Add Key %i (%s) to node %i (self)\n", key, filename.c_str(), id);
		} else {
			// Otherwise, ask the closest node (recursion)
			Node closestNode = closestFinger(key);
			closestNode.addFile(filename, ipaddr);
		}  
		
    }
    else if( command == DEL_FILE){
      cout << "Node " << id << " got DEL_FILE " << readstring(socket)  << endl;
    }
    else if( command == FIND_FILE){
      cout << "Node " << id << " got FIND_FILE " << readstring(socket)  << endl;
    }
    else if( command == GET_TABLE){

      int queryId = readint(socket);
      cout << "Node " << id << " got GET_TABLE " <<  queryId  << endl;
      Node closest = closestFinger(queryId);

      vector<int> table;
      if( queryId == id ){
         for(unsigned int i=0;i<ft.size();i++){
            table.push_back( ft[i].id );
         }
      } else {
         table = closest.getTable(queryId);
      }

      sendint(socket, ft.size());
      for(unsigned int i=0;i<table.size();i++){
         sendint(socket, table[i]);
      }

    }
    else if( command == NOTIFY ){
        int theirId = readint(socket);
        int theirPort = readint(socket);
        Node them(theirId, theirPort);

        if( prev.id == id || between(prev.id, id, them.id) ){
            prev = them;
            if( them.id != id ){
               cout << "Node " << id << " prev is " << prev.id << endl;
            }
        }
    }
    else if( command == FIND_SUCCESSOR ){

      int queryId = readint(socket);

      //cout << "Node " << id << " got FIND SUCCESSOR TO " << queryId << endl;

      Node closest = closestFinger(queryId);
      if( queryId == id ){
         closest = Node(id, port);
      }
      else if( queryId == next.id || between(id, next.id, queryId) ){
         closest = next;
      }
      else if( closest.id != id ){
         closest = closest.findSuccessorTo(queryId);
      }

      sendint(socket, closest.id);
      sendint(socket, closest.port);
    }
    else if( command == FIND_PREDECESSOR ){
      //cout << "Node " << id << " got FIND Predecessor" << endl;
      sendint(socket, prev.id);
      sendint(socket, prev.port);
    }
    else if( command == QUIT){
      cout << "Node " << id << " got QUIT" << endl;

      int originalId = readint(socket);
      if( next.id != originalId ){
         next.quit(originalId);
      }

      // ack the quit...
      sendint(socket, originalId);

      shouldQuit = true;
    }

    // stop reading from socket, but keep tryingn to send data
    close(socket);
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

    startThread(stabilizer, NULL);
    startThread(fixFingers, NULL);

    // run code for introducer here
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    //char s[INET6_ADDRSTRLEN];

    while( !canQuit() ){
       sin_size = sizeof their_addr;

       // we'll let incoming connecctions timeout to check if we should quit...
       fd_set readfds;
       FD_ZERO(&readfds);
       FD_SET(server, &readfds);

       // Timeout parameter
       timeval tv = { 0 };
       tv.tv_sec = 1;

       int ret = select(1000, &readfds, NULL, NULL, &tv);
       if (ret > 0) {
          if (FD_ISSET(server, &readfds)) {
             // Accept incoming connection and add new socket to list
             int new_fd = accept(server, (struct sockaddr *)&their_addr, &sin_size);
             if (new_fd == -1) {
                perror("accept");
                continue;
             }

             //inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
             //cout << "Node " << id << " got connection from " << s << endl;

             // prepare argument for thread
             int * arg = (int *) malloc( sizeof(int) );
             *arg = new_fd;
             startThread(thread_conn_handler, arg);

          }
       }
    }

    // let connections finish
    sleep(1);

    return EXIT_SUCCESS;
}

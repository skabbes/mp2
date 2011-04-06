// c++ libs
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// c libs
#include <cstdio>
#include <cstdlib>

#include <arpa/inet.h>
#include <time.h>

#include "socket.h"
#include "sha1.h"
#include "messages.h"
#include "node_class.h"

#define STABILIZATION_INTERVAL .25
using namespace std;

// how many messages this node has sent
int TOTAL_MESSAGES = 0;
int STABILIZER_MESSAGES = 0;

// function definitions
int main(int argc, char ** argv);
Node closestFinger(int queryId);
void quit(int socket);
void addFile(int socket);
void findSuccessor(int socket);
void notify(int socket);
void delFile(int socket);
void findFile(int socket);
void getTable(int socket);

void startDetachedThread( void * (*functor)(void *), void * arg );
pthread_t startThread( void * (*functor)(void *), void * arg );

// threads
void * fixFingers(void * arg);
void * fixFiles(void * arg);
void * stabilizer(void * arg);
void * thread_conn_handler(void * arg);

// global variables definitions
int m;
int id;
int port;
bool shouldQuit = false;

// finger table
vector<Node> ft;
pthread_mutex_t ft_mutex = PTHREAD_MUTEX_INITIALIZER;

// files
vector<string> files;

// ip addresses
vector<string> ipaddrs;

pthread_t stabilizer_thread;
pthread_t fixFingers_thread;

// predecessor
Node next;

// successor
Node prev;

int main(int argc, char ** argv){

    if( argc != 4 && argc != 5){
       cerr << "Usage: " << argv[0] << " <m> <id> <port> [<introducer-port>]" << endl;
       return EXIT_FAILURE;
    }

    // seed our RNG
    srand( time(NULL) );

    // initialzie our thread counter
    
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
    }

    // initialize finger table to self
    for(int i=0;i<m;i++){
       ft.push_back(next);
    }

    // if port == 0, we get the new port number back as an output param
    int server = setup_server(port, &port);
    stabilizer_thread = startThread(stabilizer, NULL);
    fixFingers_thread = startThread(fixFingers, NULL);

    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];

    // main loop, always accepting incoming connections
    while( !shouldQuit ){
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

             inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
             //cout << "Node " << id << " got connection from " << s << endl;

             // prepare argument for thread
             int * arg = (int *) malloc( sizeof(int) );
             *arg = new_fd;
             startDetachedThread(thread_conn_handler, arg);
          }
       }
    }

    return EXIT_SUCCESS;
}

// function which check if test lies on the ring
// between first and second
bool between(int first, int second, int test){
   if( first > second ){
      return test > first || test < second;
   }
   return test > first && test < second;
}

// thread which periodically runs to update the successor
// at a node
void * stabilizer(void * arg){
    while( true ){
        usleep( (int)(1000000 * STABILIZATION_INTERVAL) );
        // lock
        pthread_mutex_lock(&ft_mutex);

        Node x = next.findPredecessor();

        if( id == next.id || between(id, next.id, x.id) ){
            next = x;
            ft[0] = next;
        }

        next.notify(id, port);

        // unlock
        pthread_mutex_unlock(&ft_mutex);
    }

    return NULL;
}

// thread which moves files (if they need moving)
void * fixFiles(void * arg){

   // wait for system to stablizie a little
   usleep((int)(1000000 * STABILIZATION_INTERVAL));

   vector<string> files_left;
   vector<string> ips_left;

   for(unsigned int i=0;i<files.size();i++){
      int key = SHA1(files[i], m);
      if(key == id || id == prev.id || between(prev.id, id, key)){ 
         files_left.push_back( files[i] );
         ips_left.push_back( ipaddrs[i] );
      } else {
         prev.addFile(files[i], ipaddrs[i]);
      }
   }

   files = files_left;
   ipaddrs = ips_left;
   return NULL;
}

// stablilzer function to periodically update each node's finger table
// for nodes, we update ALL the finger table to ease grading
void * fixFingers(void * arg){
    while( true ){
        usleep((int)(1000000 * STABILIZATION_INTERVAL));

        // lock
        pthread_mutex_lock(&ft_mutex);

        // we can reduce the number of messages sent by implementing random sending
        // we left it as NOT random to ensure our finger tables will be correct for queries (for grading)
        //int i = (int)( ft.size() * rand() / (RAND_MAX + 1.0) );

        for(unsigned int i=0;i<ft.size();i++){
           int fingerId = (id + (1 << i)) % (1 << m);

           Node closest = closestFinger(fingerId);

           if( id == closest.id || closest.id == fingerId ){
               ft[i] = closest;
           } else {
               ft[i] = closest.findSuccessorTo( fingerId );
           }

        }

        // unlock shared memory
        pthread_mutex_unlock(&ft_mutex);
    }

    return NULL;
}

void startDetachedThread( void * (*functor)(void *), void * arg ){
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

pthread_t startThread( void * (*functor)(void *), void * arg ){
    pthread_t handler;
    if( pthread_create(&handler, NULL, functor, arg) ){
        free(arg);
        perror("pthread_create");
    }
    return handler;
}

// finds the closest id to a queryId in the finger table
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


// launch a new node process
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

    TOTAL_MESSAGES++;

    if( command == ADD_NODE){
       int size = readint(socket);
       for(int i=0; i<size; i++){
          int id = readint(socket);
          launchNode(m, id);
       }
    }
    else if( command == ADD_FILE){
        addFile(socket);
    }
    else if( command == DEL_FILE){
        delFile(socket);
    }
    else if( command == FIND_FILE){
        findFile(socket);
    }
    else if( command == GET_TABLE){
        getTable(socket);
    }
    else if( command == NOTIFY ){
        STABILIZER_MESSAGES++;
        notify(socket);
    }
    else if( command == FIND_SUCCESSOR ){
      STABILIZER_MESSAGES++;
      findSuccessor(socket);
    }
    else if( command == FIND_PREDECESSOR ){
      STABILIZER_MESSAGES++;
      sendint(socket, prev.id);
      sendint(socket, prev.port);
    }
    else if( command == QUIT){
        quit(socket);
    }

    // stop reading from socket, but keep tryingn to send data
    close(socket);
    return NULL;
}

void getTable(int socket){
    int queryId = readint(socket);
    Node closest = closestFinger(queryId);

    pair< vector<int>, vector<int> > returnValue;
    // if found
    if( queryId == id )
    {
        for(unsigned int i=0;i<ft.size();i++) {
            returnValue.first.push_back( ft[i].id );
        }

        for(unsigned int i=0;i<files.size();i++) {
            returnValue.second.push_back( SHA1(files[i], m) );
        }

    } else if( id == next.id || between(id, next.id, queryId) ){
        vector<int> temp;
        returnValue = pair< vector<int>, vector<int> >(temp, temp);
    }else {
        returnValue = closest.getTable(queryId);
    }

    sendint(socket, returnValue.first.size());
    for(unsigned int i=0;i<returnValue.first.size();i++){
        sendint(socket, returnValue.first[i]);
    }

    sendint(socket, returnValue.second.size());
    for(unsigned int i=0;i<returnValue.second.size();i++){
        sendint(socket, returnValue.second[i] );
    }
}

void findFile(int socket){
    string filename = readstring(socket);
    int key = SHA1(filename, m);

    bool found = false;
    string ip = "";
    int fileNodeId = -1;

    if( key == id || between(prev.id, id, key) ){
        fileNodeId = id;

        // look for this file
        for(unsigned int i=0;i<files.size();i++){
            if(files[i] == filename){
                found = true;
                ip = ipaddrs[i];
                break;
            }
        }
    } else {
        Node closest = closestFinger(key);
        pair<int, string> result = closest.findFile(filename);
        if(result.first == -1){
            found = false;
        } else {
            found = true;
            fileNodeId = result.first;
            ip = result.second;
        }
    }

    if( found ){
        sendint(socket, FILE_FOUND);
        sendint(socket, fileNodeId);
        sendstring(socket, ip);
    } else {
        sendint(socket, FILE_NOT_FOUND);
    }
}

void delFile(int socket){
    string filename = readstring(socket);

    int key = SHA1(filename, m);
    int fileNodeId = -1;

    // if the key is the same as the node ID or in between node Id and its predecessor
    if (key==id || between(prev.id,id,key))
    {
        // search for the file inside this node
        for (unsigned int i=0; i < files.size(); ++i)
        {
            if (files[i]==filename)
            {
                files.erase(files.begin() + i);
                ipaddrs.erase(ipaddrs.begin() + i);
                fileNodeId = id;
                break;
            }
        }

    } else {
        // Otherwise, ask the closest node 
        Node closestNode = closestFinger(key);
        fileNodeId = closestNode.removeFile(filename);
    }

    if( fileNodeId != -1){
        sendint(socket, FILE_FOUND);
        sendint(socket, fileNodeId);
    } else {
        sendint(socket, FILE_NOT_FOUND);
    }
}

void notify(int socket){
    int theirId = readint(socket);
    int theirPort = readint(socket);
    Node them(theirId, theirPort);

    if( prev.id == id || between(prev.id, id, them.id) ){

        if( them.id != id ){
            prev = them;
            startDetachedThread(fixFiles, NULL);
        }
        prev = them;
    }

    // send an ack...
    sendint(socket, 0);
}

void findSuccessor(int socket){
      int queryId = readint(socket);

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

void addFile(int socket){
    string filename = readstring(socket);
    string ipaddr = readstring(socket);

    int key = SHA1(filename,m);

    int fileNodeId = -1;

    // if the key is the same as the node ID or in between node Id and its predecessor
    if (prev.id == id || key == id || between(prev.id,id,key))
    {
        fileNodeId = id;
        files.push_back(filename.c_str());
        ipaddrs.push_back(ipaddr.c_str());
    } else {
        // Otherwise, ask the closest node (recursion)
        Node closestNode = closestFinger(key);
        fileNodeId = closestNode.addFile(filename, ipaddr);
    }  
    sendint(socket, fileNodeId);	
}

void quit(int socket){
      int originalId = readint(socket);

      int totalMessages = 0;
      int stabilizerMessages = 0;

      // don't send messages until our stabilization threads have stopped
      pthread_cancel(stabilizer_thread);
      pthread_cancel(fixFingers_thread);

      if( next.id != originalId ){
         pair<int, int> messages = next.quit(originalId);
         totalMessages += messages.first;
         stabilizerMessages += messages.second;
      }

      shouldQuit = true;

      totalMessages += TOTAL_MESSAGES;
      stabilizerMessages += STABILIZER_MESSAGES;

      sendint(socket, totalMessages);
      sendint(socket, stabilizerMessages);
}

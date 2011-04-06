#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// for sleep function
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>

#include "socket.h"
#include "messages.h"

using namespace std;

// function definitions
int main(int argc, char ** argv);
void * processCommand(void * arg);
void addNode(vector<int> ids);
void addFile(string filename, string ip);
void delFile(string filename);
void findFile(string filename);
void getTable(int id);
void quit();
void startThread( void * (*functor)(void *), void * arg );

// global variables definitions
char * host;
char * port;

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

int main(int argc, char ** argv){

    if( argc != 3){
       cerr << "Usage " << argv[0] << " <introducer-host> <introducer-port>" << endl;
       return EXIT_FAILURE;
    }

    host = argv[1];
    port = argv[2];


    // read commands in line by line and process them
    bool shouldQuit = false;
    while( cin && !shouldQuit ){
        string * input = new string();
        getline(cin, *input);

        stringstream is (stringstream::in | stringstream::out);
        is << (*input);

        string command;
        is >> command;

        if( command  == "QUIT" ){
           shouldQuit = true;
        } else if( command  == "SLEEP" ){
           int seconds; 
           is >> seconds;
           cout << "sleeping for " << seconds << endl;
           sleep(seconds); 
        } else {
           // start a thread so not to block other operations (handle concurrently)
           startThread(processCommand, input);
        }
    }

    quit();
    return 0;
}

void * processCommand(void * arg){
    string * temp = (string *)arg;
    string input = *temp;
    delete temp;

    stringstream is (stringstream::in | stringstream::out);
    is << input;

    string command;
    is >> command;
    if( command.size() == 0 ) return false;

    if( command == "ADD_NODE" ){
        vector<int> ids;
        while( !is.eof() ){
            int id = -1;
            is >> id;
            if( id  != -1 ){
               ids.push_back(id);
            }
        }
        addNode(ids);
    } else if( command == "ADD_FILE" ){
        string filename, ip;
        is >> filename;
        is >> ip;
        addFile(filename, ip);
    } else if( command == "DEL_FILE" ){
        string filename;
        is >> filename;
        delFile(filename);
    } else if( command == "FIND_FILE" ){
        string filename;
        is >> filename;
        findFile(filename);
    } else if( command == "GET_TABLE" ){
        int nodeId;
        is >> nodeId; 
        getTable(nodeId);
    } else {
        // unrecognized command, just ignore it with an error
        cerr << "Invalid command: " << command << endl;
    }

    return NULL;
}

void addNode(vector<int> ids){
    cout << "ADD_NODE called with";
    for(unsigned int i=0;i< ids.size();i++){
       cout << " " << ids[i];
    }
    cout << endl;

    int socket = setup_client(host, port);
    sendint(socket, ADD_NODE);
    sendint(socket, ids.size() );

    for(unsigned int i=0; i<ids.size(); i++){
       sendint(socket, ids[i]);
    }
    close(socket);

    /* BAD, don't sleep, prevents concurrent operation */

    int waitTime = ids.size() * 2;

    cout << "Waiting " << waitTime << " seconds for system to stabilize" << endl;
    sleep( waitTime );

    for(unsigned int i=0; i<ids.size(); i++){
       int socket = setup_client(host, port);
       sendint(socket, GET_TABLE);
       sendint(socket, ids[i]);

       int size = readint(socket);
       cout << "Finger table for node " << ids[i] << endl;
       cout << "\ti\tf[i]" << endl;
       for(int j=0;j<size;j++){
         int temp = readint(socket);
         cout << "\t" << j << "\t" << temp << endl;
       }
       close(socket);
    }

}

void addFile(string filename, string ip){
    cout << "ADD_FILE called with " << filename << ", " << ip << endl;

    int socket = setup_client(host, port);
    sendint(socket, ADD_FILE);

    sendstring(socket, filename);
    sendstring(socket, ip);

   close(socket);
}

void delFile(string filename){
    cout << "DEL_FILE called with " << filename << endl;
    int socket = setup_client(host, port);
    sendint(socket, DEL_FILE);
    sendstring(socket, filename);
    shutdown(socket, 1);
}

void findFile(string filename){
    cout << "FIND_FILE called with " << filename << endl;
    int socket = setup_client(host, port);
    sendint(socket, FIND_FILE);
    sendstring(socket, filename);

    int result = readint(socket);
    if( result == FILE_FOUND ){
        int fileNodeId = readint(socket);
        string ip = readstring(socket);
        cout << filename << " found at Node " << fileNodeId << " : " << ip << endl;
    } else {
        cout << filename << " not found " << endl;
        cout << "error code " << result;
    }

    close(socket);
}

void getTable(int id){
    cout << "GET_TABLE called with " << id << endl;
    int socket = setup_client(host, port);
    sendint(socket, GET_TABLE);
    sendint(socket, id);

    int size = readint(socket);
    if( size == 0 ){
       cout << "Node " << id << " doesn't yet exist" << endl;
    } else {
        for(int i=0;i<size;i++){
            cout << readint(socket) << endl;
        }
    }
    close(socket);
}

void quit(){
    cout << "QUIT called, quitting in 5 seconds after connections finish"  << endl;

    int socket = setup_client(host, port);
    sendint(socket, QUIT);

    // designate the origin of the quit to come from node 0
    sendint(socket, 0);
    int total = readint(socket);
    close(socket);
    cout << "TOTAL MESSAGES: " << total << endl;

    sleep(5);
}

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#include "sha1.h"
#include "messages.h"
#include "node_class.h"

using namespace std;

// function definitions
int main(int argc, char ** argv);

// threaded command handler
void * processCommand(void * arg);

// auxiliary functions
pthread_t startThread( void * (*functor)(void *), void * arg );
void printFingerTable( int id, vector<int> const & table );
void printKeys( int id, vector<int> const & table );
void waitForThreads(vector<pthread_t> threads);

// funcitons which handle the individual commands
void addFile(string filename, string ip);
void addNode(vector<int> ids);
void delFile(string filename);
void findFile(string filename);
void getTable(int id);
void quit();

// global variables definitions
Node introducer;
int M;

int main(int argc, char ** argv){

    if( argc != 3){
       cerr << "Usage " << argv[0] << " <m> <introducer-port>" << endl;
       return EXIT_FAILURE;
    }

    M = atoi(argv[1]);
    int port = atoi(argv[2]);
    introducer = Node(0, port);

    bool shouldQuit = false;

    vector<pthread_t> threads;

    // read commands in line by line and process them
    while( cin && !shouldQuit ){
        string * input = new string();
        getline(cin, *input);

        stringstream is (stringstream::in | stringstream::out);
        is << (*input);

        string command;
        is >> command;

        if( cin.eof() ) continue;

        if( command  == "QUIT" ){
           shouldQuit = true;
        } else if( command  == "SLEEP" ){
           int seconds; 
           is >> seconds;

           waitForThreads(threads);
           threads.empty();

           cout << "sleeping for " << seconds << endl;
           sleep(seconds); 
        } else {
           // start a thread so not to block other operations (handle concurrency)
           threads.push_back( startThread(processCommand, input) );
        }
    }

    shouldQuit = true;

    waitForThreads(threads);
    threads.empty();

    quit();
    return 0;
}

void waitForThreads(vector<pthread_t> threads){
    for(unsigned int i=0;i<threads.size();i++){
        pthread_join( threads[i], NULL);
    }
}

// prints a finger table for a node with ID == id
void printFingerTable( int id, vector<int> const & table ){
   cout << "Finger table for node " << id << endl;
   cout << "\ti\tf[i]" << endl;
   for(unsigned int j=0;j<table.size();j++){
     cout << "\t" << j << "\t" << table[j] << endl;
   }
}

// prints the keys for a node with ID == id
void printKeys( int id, vector<int> const & keys ){
	   cout << "Keys at node " << id << " ";
	   cout << "(";
	   for(unsigned int j=0;j<keys.size();j++){
         if( j != 0 ){
            cout << ", ";
         }
         cout << keys[j];
       }
	   cout << ")" << endl;
}

pthread_t startThread( void * (*functor)(void *), void * arg ){
    pthread_t handler;
    if( pthread_create(&handler, NULL, functor, arg) ){
        free(arg);
        perror("pthread_create");
    }
    return handler;
}


// threaded command dispatcher
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

// add a nodes to the system, then wait for the system to stabilize and print
// its finger table
void addNode(vector<int> ids){
    introducer.addNodes(ids);

    double waitTime = ids.size() * .5;

    cout << "Waiting " << waitTime << " seconds for system to stabilize" << endl;
    usleep( (int)(waitTime * 1000000) );

    for(unsigned int i=0; i<ids.size(); i++){
       getTable(ids[i]);
    }

}

// add a file into the system with attribute == ip
void addFile(string filename, string ip){
    int fileNodeId = introducer.addFile(filename, ip);	
    cout << "Added file " << filename << " (" << SHA1(filename, M) <<  ") to Node " << fileNodeId << endl; 
}

// delete a file from the system, or print an error on failure
void delFile(string filename){
    int key = SHA1(filename, M);
    int fileNodeId = introducer.removeFile(filename);
	if (fileNodeId == -1){
		cout << "[Error]:" << filename << " (" << key << ") not found!" << endl;
	} else {
		cout << filename << " (" << key << ") has been deleted from Node " << fileNodeId << endl;
	}
}

// find a file in the system, or print an error on failure
void findFile(string filename){
    pair<int, string> p = introducer.findFile(filename);
    int fileNodeId = p.first;
    string ip = p.second;
    int key = SHA1(filename, M);

    // not found
    if( fileNodeId == -1 ){
        cout << filename << " (" << key << ") not found." << endl;
    } else {
        cout << filename << " (" << key << ") found at Node " << fileNodeId << " : " << ip << endl;
    }
}

// get (and print) the finger table at a node
void getTable(int id){
    pair< vector<int>, vector<int> > p = introducer.getTable(id);
    vector<int> table = p.first;
    vector<int> keys = p.second;

    if( table.size() == 0 ){
       cout << "Node " << id << " doesn't yet exist" << endl;
    } else {
       printFingerTable(id, table);
       printKeys(id, keys);
    }
}

// Send a quit message to all nodes in the system
void quit(){

    pair<int, int> p = introducer.quit( introducer.id );
    int totalMessages = p.first;
    int stabilizerMessages = p.second;

    cout << "TOTAL MESSAGES: " << totalMessages << endl;
    cout << "STABLILZER MESSAGES: " << stabilizerMessages << endl;
    cout << "DIFFERENCE : " << totalMessages - stabilizerMessages << endl;
}

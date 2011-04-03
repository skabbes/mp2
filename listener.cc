#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// for sleep function
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>

#include "socket.h"
#include "messages.h"

using namespace std;

// function definitions
int main(int argc, char ** argv);
bool processCommand(stringstream & is);
void addNode(vector<int> ids);
void addFile(string filename, string ip);
void delFile(string filename);
void findFile(string filename);
void getTable(int id);
void quit();

// global variables definitions
char * host;
char * port;

int main(int argc, char ** argv){

    if( argc != 3){
       cerr << "Usage " << argv[0] << " <introducer-host> <introducer-port>" << endl;
       return EXIT_FAILURE;
    }

    host = argv[1];
    port = argv[2];

    string input;

    // read commands in line by line and process them
    bool shouldQuit = false;
    while( cin && !shouldQuit ){
        stringstream is (stringstream::in | stringstream::out);
        getline(cin, input);
        is << input;
        shouldQuit = processCommand(is);
    }

    quit();
    return 0;
}

bool processCommand(stringstream & is){
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
    } else if( command == "SLEEP" ){
        int seconds; 
        is >> seconds;
        sleep(seconds); 
    } else if( command == "QUIT" ){
        // don't actually call quit here, we will call it in main to handle
        // premature quits as well
    } else {
        // unrecognized command, just ignore it with an error
        cerr << "Invalid command: " << command << endl;
    }
    return command == "QUIT";
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
    shutdown(socket, 1);
}

void getTable(int id){
    cout << "GET_TABLE called with " << id << endl;
    int socket = setup_client(host, port);
    sendint(socket, GET_TABLE);
    sendint(socket, id);
    shutdown(socket, 1);
}

void quit(){
    cout << "QUIT called" << endl;
    int socket = setup_client(host, port);
    sendint(socket, QUIT);

    // designate the origin of the quit to come from node 0
    sendint(socket, 0);
    readint(socket);
    close(socket);
}

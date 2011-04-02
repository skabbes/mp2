#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// for sleep function
#include <cstdlib>
#include <unistd.h>

#include "socket.h"

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
        while( is ){
            int id;
            is >> id;
            if( !is.eof() ){
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
    cout << "ADD_NODE called" << endl;
}

void addFile(string filename, string ip){
    cout << "ADD_FILE called" << endl;
}

void delFile(string filename){
    cout << "DEL_FILE called" << endl;
}

void findFile(string filename){
    cout << "FIND_FILE called" << endl;
}

void getTable(int id){
    cout << "GET_TABLE called" << endl;
}

void quit(){
    cout << "QUIT called" << endl;
}

#include <iostream>
#include <vector>
#include <string>
#include <utility>

#include <arpa/inet.h>

#include "node_class.h"
#include "socket.h"
#include "messages.h"

using namespace std;

Node::Node(){}

Node::Node(int _id, int _port){
   id = _id;
   port = _port;
}

Node Node::findSuccessorTo(int queryId){
   //open up a connection to this node, and return its successor
   int socket = setup_client("localhost", port);

   sendint(socket, FIND_SUCCESSOR);
   sendint(socket, queryId);

   int successorId = readint(socket);
   int successorPort = readint(socket);

   close(socket);
   return Node(successorId, successorPort);
}

pair< vector<int>, vector<int> > Node::getTable(int queryId){
   int socket = setup_client("localhost", port);

   sendint(socket, GET_TABLE);
   sendint(socket, queryId);

   int tableSize = readint(socket);
   vector<int> table;
   for(int i=0;i<tableSize;i++){
      int temp = readint(socket);
      table.push_back(temp);
   }
   
   int keySize = readint(socket);
   vector<int> keys;
   for(int i=0;i<keySize;i++){
      int temp = readint(socket);
      keys.push_back(temp);
   }

   close(socket);
   return pair< vector<int>, vector<int> >(table, keys);
}

pair<int, int> Node::quit(int originalId){
   int socket = setup_client("localhost", port);
   sendint(socket, QUIT);
   sendint(socket, originalId);

   // wait for the ack
   int total_messages = readint(socket);
   int stabilizer_messages = readint(socket);
   close(socket);
   return pair<int, int>(total_messages, stabilizer_messages);
}

void Node::notify(int myId, int myPort){
   int socket = setup_client("localhost", port);
   sendint(socket, NOTIFY);
   sendint(socket, myId);
   sendint(socket, myPort);

   int ack = readint(socket);
   ack++;
   close(socket);
}

Node Node::findPredecessor(){
   //open up a connection to this node, and return its successor
   int socket = setup_client("localhost", port);

   sendint(socket, FIND_PREDECESSOR);

   int predecessorId = readint(socket);
   int predecessorPort = readint(socket);

   close(socket);
   return Node(predecessorId, predecessorPort);
}

/**
* Add file to the node (asking if there is any node can store the specific file)
*/
void Node::addFile(string filename, string ipaddr)
{
   //open up a connection to this node, and return its successor
   int socket = setup_client("localhost", port);

   sendint(socket, ADD_FILE);
   sendstring(socket, filename);
   sendstring(socket, ipaddr);

   close(socket);
}
/**
* Remove file from the node
* @param filename - filename to be deleted
*/
void Node::removeFile(string filename)
{
	int socket = setup_client("localhost",port);
	
	sendint(socket, DEL_FILE);
	sendstring(socket, filename);
	
	close(socket);
}

// if the id == -1, then let that denote an error (no file found)
pair<int, string> Node::findFile(string filename){
   //open up a connection to this node, and return its successor
   int socket = setup_client("localhost", port);

   sendint(socket, FIND_FILE);
   sendstring(socket, filename);

   int id = -1;
   string ip = "";

   int result = readint(socket);
   if( result == FILE_FOUND ){
       id = readint(socket);
       ip = readstring(socket);
   } 

   close(socket);
   return pair<int, string>(id, ip);
}

void Node::setId(int _id){
   id = _id;
}

void Node::setPort(int _port){
   port = _port;
}

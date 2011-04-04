#include <iostream>
#include <vector>
#include <string>
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

vector<int> Node::getTable(int queryId){
   int socket = setup_client("localhost", port);

   sendint(socket, GET_TABLE);
   sendint(socket, queryId);

   int size = readint(socket);
   vector<int> table;
   for(int i=0;i<size;i++){
      int temp = readint(socket);
      table.push_back(temp);
   }

   close(socket);
   return table;
}

int Node::quit(int originalId){
   int socket = setup_client("localhost", port);
   sendint(socket, QUIT);
   sendint(socket, originalId);

   // wait for the ack
   int total = readint(socket);
   close(socket);
   return total;
}

void Node::notify(int myId, int myPort){
   int socket = setup_client("localhost", port);
   sendint(socket, NOTIFY);
   sendint(socket, myId);
   sendint(socket, myPort);
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

void Node::setId(int _id){
   id = _id;
}

void Node::setPort(int _port){
   port = _port;
}

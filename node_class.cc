#include <iostream>
#include <vector>
#include <string>

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

   Node temp(successorId, successorPort);
   return temp;
}

void Node::notify(int myId, int myPort){
   int socket = setup_client("localhost", port);
   sendint(socket, NOTIFY);
   sendint(socket, myId);
   sendint(socket, myPort);
}

Node Node::findPredecessor(){
   //open up a connection to this node, and return its successor
   int socket = setup_client("localhost", port);

   sendint(socket, FIND_PREDECESSOR);

   int predecessorId = readint(socket);
   int predecessorPort = readint(socket);

   return Node(predecessorId, predecessorPort);
}

void Node::setId(int _id){
   id = _id;
}

void Node::setPort(int _port){
   port = _port;
}

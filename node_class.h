#ifndef SERVER_CLASS
#define SERVER_CLASS

#include <vector>
#include <iostream>
using namespace std;

class Node{

public:
    int id;
    int port;

    Node();
    Node(int id, int port);
    Node findSuccessorTo(int queryId);
    void setId(int id);
    void setPort(int port);
};

#endif

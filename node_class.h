#ifndef SERVER_CLASS
#define SERVER_CLASS

#include <string>
#include <vector>
#include <iostream>
#include <utility>
using namespace std;

class Node{

public:
    int id;
    int port;

    Node();
    Node(int id, int port);
    Node findSuccessorTo(int queryId);
    vector<int> getTable(int queryId);
    Node findPredecessor();
    void addFile(string _filename, string _ipaddr);
	void removeFile(string filename);
    pair<int, string> findFile(string filename);
    int quit(int originalId);

    void notify(int myId, int myPort);
    void setId(int id);
    void setPort(int port);
};

#endif

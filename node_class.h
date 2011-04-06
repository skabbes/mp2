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
    pair<vector<int>, vector<int> > getTable(int queryId);
    Node findPredecessor();
    void addFile(string _filename, string _ipaddr);
	int removeFile(string filename);
    pair<int, string> findFile(string filename);
    pair<int, int> quit(int originalId);

    void notify(int myId, int myPort);
    void setId(int id);
    void setPort(int port);
};

#endif

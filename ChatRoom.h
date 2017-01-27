#include <sys/socket.h>
#include <iostream>
#include <string>
#include <sstream>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>

using namespace std;

#define BUFFER_LENGTH 2500
    
class ChatRoom {
public:
	string roomName;
	int portNumber;
	int numberOfMembers;
	int chatSocket;
    int maxFD;
	pthread_t receiveConnectionsThread, receiveMessageThread;
	fd_set* slaveSocketList;
    fd_set* readFDs;
	
	//NOTE: left out slaveSocket, not sure how this thing works properly
	ChatRoom(void){
		roomName = "";
		portNumber = 0;
		numberOfMembers = 0;
		chatSocket = 0;
		
        fd_set tempFDSet;
        FD_ZERO(&tempFDSet);
		slaveSocketList = &tempFDSet;
	}
    
    ChatRoom(string rn, int pn, int nom, int cs){
        roomName = rn;
        portNumber = pn;
        numberOfMembers = nom;
        chatSocket = cs;
        maxFD = chatSocket;
        
        fd_set tempFDSet;
        FD_ZERO(&tempFDSet);
		slaveSocketList = &tempFDSet;
        
        fd_set tempFDSet2;
        FD_ZERO(&tempFDSet2);
		readFDs = &tempFDSet2;
    }
};

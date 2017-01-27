// crsd.cpp
// Chat Room Server
// Homework 1
// Colin Banigan and Katherine Drake
// Due January 27, 2017

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <utility>
#include "ChatRoom.h"

#define MAX_MEMBER 32
#define MAX_ROOM 32
#define BUFFER_LENGTH 2500

vector<ChatRoom*> chatRoomDB;
char* service = "16230"; //service name or port number
char buffer[BUFFER_LENGTH];

int passiveTCPsock(const char * service, int backlog);

void* receiveClientConnections(void* argument) {
    int indexCRDB = (intptr_t) argument;
    
    for (;;) {
        struct sockaddr_in clientAddr;
        socklen_t length;

        // wait for new client to connect
        int slaveSocket = accept(chatRoomDB[indexCRDB]->chatSocket, (struct sockaddr *)&clientAddr, &length);
        if(slaveSocket >= 0){
            cout << "New Client Has Connected to Chat Room " << chatRoomDB[indexCRDB]->roomName << endl;
            // add new slave socket to list of sockets
            FD_SET(slaveSocket, chatRoomDB[indexCRDB]->slaveSocketList);
            
            cout << "This New Client's FD is: " << slaveSocket << endl;
            //FD_SET(slaveSocket, chatRoomDB[indexCRDB]->readFDs);
            
            if (slaveSocket > chatRoomDB[indexCRDB]->maxFD) {
                cout << "Updating MaxFD. It was: " << chatRoomDB[indexCRDB]->maxFD << endl;
                chatRoomDB[indexCRDB]->maxFD = slaveSocket;
            }
            else {
                cout << "Not Updating MaxFD. It is: " << chatRoomDB[indexCRDB]->maxFD << endl;
            }
        }
    }
}

void* receiveMessages(void* argument) {
    int indexCRDB = (intptr_t) argument;
    
    // create buffer for messages
    char buffer[BUFFER_LENGTH];
    memset(buffer, 0, sizeof(buffer));

    // set timeout for select to 10,000 seconds
    struct timeval timeval;
    timeval.tv_sec = 10;
    timeval.tv_usec = 0;

    // wait for messages from client
    for (;;) {
        usleep(500);
        int maxFD = chatRoomDB[indexCRDB]->maxFD;
        //cout << "I think maxFD is: " << maxFD << endl;
        chatRoomDB[indexCRDB]->readFDs = chatRoomDB[indexCRDB]->slaveSocketList;
        
        // wait until a slave socket is ready to send datas
        //cout << "Stuck1 \n";
        if (select(maxFD+1, chatRoomDB[indexCRDB]->readFDs, NULL, NULL, NULL) > 0) {
            // run through the existing connections looking for data to read
          //  cout << "Stuck2 \n";
            for(int tempFD = 0; tempFD <= maxFD; tempFD++) {
            //    cout << "Stuck3 \n";
                if (FD_ISSET(tempFD, chatRoomDB[indexCRDB]->readFDs)) {
                    // handle data from a client
              //      cout << "Stuck4 \n";
                    if (recv(tempFD, buffer, sizeof(buffer), 0) > 0) {
                        // send message to other clients
                //        cout << "Stuck5 \n";
                        for(int otherTempFD = 0; otherTempFD <= maxFD; otherTempFD++) {
                  //          cout << "Stuck6 \n";
                            if (FD_ISSET(otherTempFD, chatRoomDB[indexCRDB]->slaveSocketList)) {
                                // except the listener and ourselves
                    //            cout << "Stuck7 \n";
                                if (otherTempFD != chatRoomDB[indexCRDB]->chatSocket 
                                    && otherTempFD != tempFD) {
                                    send(otherTempFD, buffer, sizeof(buffer), 0);

                                    cout << "Chat Room: " << chatRoomDB[indexCRDB]->roomName << ",      Client Message: " << buffer << endl;
                                }
                            }
                        }
                        cout << "GOT A MESSAGE! " << buffer << endl;
                        memset(buffer, 0, sizeof(buffer));
                    }
                }
            } 
        }
    } 
        /*
        else {
            //cout << "SELECTED SOCKET = " << selectedSocket << endl;
        
            // receive data from select file descriptor
            int receiveMessage = recv(selectedSocket, buffer, sizeof(buffer), 0);
            if (receiveMessage < 0) {
                //cout << "ERROR: Message Failed to Receive from Server\n"; 
                continue;
            }

            cout << "Chat Room: " << chatRoomDB[indexCRDB]->roomName << ", Client Message: " << buffer << endl;
            memset(buffer, 0, sizeof(buffer));
        }*/
}

void startThreads(int indexCRDB) {
    int createConnectionThread = pthread_create (&(chatRoomDB[indexCRDB]->receiveConnectionsThread), NULL, receiveClientConnections, (void *) (intptr_t) indexCRDB);
    if (createConnectionThread < 0) {
        cout << "ERROR: Couldn't Create Connection Thread\n";
    }
    
    int createReceiveThread = pthread_create (&(chatRoomDB[indexCRDB]->receiveMessageThread), NULL, receiveMessages, (void *) (intptr_t) indexCRDB);
    if (createReceiveThread < 0) {
        cout << "ERROR: Couldn't Create Receive Thread\n";
    }
    
}

void createChatRoom(string chatName, int s_sock, sockaddr_in clientAddr, socklen_t length){
	bool alreadyExists = false;
	for(int i = 0; i < chatRoomDB.size(); i++){
		if(chatRoomDB[i]->roomName == chatName){
			alreadyExists = true;
		}
	}
	if(!alreadyExists){
		cout << "chat room " << chatName << " does not yet exist, creating now." << endl;
        
        //create new socket for the created chat room
		char* newPort = (char*) to_string(stoi(service) + chatRoomDB.size() + 1).c_str();
        
        cout << "New port number is " << newPort << endl;
        
        int newSocket = passiveTCPsock(newPort, 32);
        if (newSocket < 0) {
            cout << "ERROR: Socket Less Than 0\n";
            return;
        }
        
        ChatRoom* newChat = new ChatRoom(chatName, (stoi(service) + chatRoomDB.size() + 1), 0, newSocket);
		
		chatRoomDB.push_back(newChat);
        
        //passing index of the chatRoomDB of the new chat room, write a search function later if need to create more threads later
        startThreads(chatRoomDB.size() - 1);
		
		//Inform client that the server was created
		string messageToSend = "SUCCESS Created chat room \"" + chatName + "\".";
		strcpy(buffer, messageToSend.c_str());
		
		int sendMessage = sendto(s_sock, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&clientAddr, length);
		memset(buffer, 0, sizeof(buffer));
		if (sendMessage < 0) {
			cout << "ERROR: Message Failed to Send to Clients"; 
		}
		
	} else {
		cout << "chat room " << chatName << " already exists! Cannot create." << endl;
	}
}

void deleteChatRoom(string chatName, int s_sock, sockaddr_in clientAddr, socklen_t length){
	for(int i = 0; i < chatRoomDB.size(); i++){
		if(chatRoomDB[i]->roomName == chatName){
			cout << "chat room " << chatName << " already exists, deleting now." << endl;
			chatRoomDB.erase(chatRoomDB.begin()+i);
			//PROBS GONNA HAVE TO CLOSE THE SOCKET IN THE CHATROOMDB AS WELL, I DOUBT THIS IS ENOUGH
			
			//Inform client that the server was deleted
			string messageToSend = "Deleted chat room \"" + chatName + "\".";
			strcpy(buffer, messageToSend.c_str());
			
			int sendMessage = sendto(s_sock, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&clientAddr, length);
			memset(buffer, 0, sizeof(buffer));
			if (sendMessage < 0) {
				cout << "ERROR: Message Failed to Send to Clients"; 
			}
			return;
		}
	}
	cout << "ERROR: Chat room " << chatName << " does not exist! Cannot delete." << endl;
	
	string messageToSend = "ERROR: Chat room " + chatName + " does not exist! Cannot delete.";
	strcpy(buffer, messageToSend.c_str());
	
	int sendMessage = sendto(s_sock, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&clientAddr, length);
	memset(buffer, 0, sizeof(buffer));
	if (sendMessage < 0) {
		cout << "ERROR: Message Failed to Send to Clients"; 
	}
	
}

void joinChatRoom(string chatName, int s_sock, sockaddr_in clientAddr, socklen_t length){
	for(int i = 0; i < chatRoomDB.size(); i++){
		if(chatRoomDB[i]->roomName == chatName){
			cout << "\nchat room " << chatName << " already exists, attempting to join." << endl;
			chatRoomDB[i]->numberOfMembers++;
			//chatRoomDB[i].slaveSocketList.push_back(s_sock);
			cout << "chat room " << chatName << " now has " << chatRoomDB[i]->numberOfMembers << " members!" << endl;
			
			//Inform client that the server was joined
			string messageToSend = "JOINROOM " + to_string(chatRoomDB[i]->portNumber) + " " + to_string(chatRoomDB[i]->numberOfMembers);
			strcpy(buffer, messageToSend.c_str());
			
			int sendMessage = sendto(s_sock, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&clientAddr, length);
			memset(buffer, 0, sizeof(buffer));
			if (sendMessage < 0) {
				cout << "ERROR: Message Failed to Send to Clients"; 
			}
			return;
		}
	}
	cout << "ERROR: Chat room " << chatName << " does not exist! Cannot join." << endl;
	
	string messageToSend = "ERROR: Chat room " + chatName + " does not exist! Cannot join.";
	strcpy(buffer, messageToSend.c_str());
	
	int sendMessage = sendto(s_sock, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)&clientAddr, length);
	memset(buffer, 0, sizeof(buffer));
	if (sendMessage < 0) {
		cout << "ERROR: Message Failed to Send to Clients"; 
	}
	return;
}

//string split functions below
void split(const string &s, char delim, vector<string> &elems) {
	stringstream ss;
	ss.str(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

vector<string> split(const string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

//taken from slide 41 of Lecture 2
int passiveTCPsock(const char * serviceArg, int backlog) {
	
	struct sockaddr_in socketAddr;
	memset(&socketAddr, 0, sizeof(sockaddr));
	socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(stoi(serviceArg));
	socketAddr.sin_addr.s_addr = INADDR_ANY;
	
	// Map Server Name to Port Number
	/*if(struct servent *pse = getservbyname(serviceArg, "tcp")) {
		socketAddr.sin_port = pse->s_port;
	}
	else if((socketAddr.sin_port = htons((unsigned short)atoi(serviceArg))) == 0){
		cout << "Error: can't get service" << endl;
	}
    else {
        cout << "SOMETHING WEIRD HAPPENED\n\n";
    }*/
	
	// Allocate socket
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0) {
		cout << "Error: can't create socket" << endl;
	}
	
	//Bind the socket
	if(bind(s, (struct sockaddr *)&socketAddr, sizeof(socketAddr)) < 0){
		cout << "Error: can't bind" << endl;
	}
	
	//Listen on socket
	if(listen(s, backlog) < 0){
		cout << "Error: can't listen" << endl;
	}
	
	return s;
}

int main(int argc, char** argv){
	int m_sock, s_sock; //master and slave socket
	memset(buffer, 0, sizeof(buffer));
	struct sockaddr_in clientAddr;
	socklen_t length;
	
	m_sock = passiveTCPsock(service, 32);
    if (m_sock < 0) {
        cout << "ERROR: Socket Less Than 0\n";
        return 0;
    }
	
	for (;;) {
		s_sock = accept(m_sock, (struct sockaddr *)&clientAddr, &length);
		if(s_sock < 0){
			//errexit("accept failed: %s\n", strerror(errno));
			//cout << "Error: Accept failed" << endl;
		} else {
			cout << "\n\nWe're connected to the client!" << endl;
		}
		
		int receiveMessage = recvfrom(s_sock, buffer, BUFFER_LENGTH, 0, NULL, NULL);
		if (receiveMessage < 0) {
			//cout << "ERROR: Message Failed to Receive from Server" << endl;
		} else {
			cout << "Client Says: " << buffer << "\n";
		}
		
		//deal with message here, contained within buffer
		vector<string> splitBuffer = split(buffer, ' ');
		
		if (splitBuffer[0] == "CREATE"){
			createChatRoom(splitBuffer[1], s_sock, clientAddr, length);
		} else if (splitBuffer[0] == "DELETE"){
			deleteChatRoom(splitBuffer[1], s_sock, clientAddr, length);
		} else if (splitBuffer[0] == "JOIN"){
			joinChatRoom(splitBuffer[1], s_sock, clientAddr, length);
		}
		
		memset(buffer, 0, sizeof(buffer));
	}
	//close(m_sock);
	
	return 0;
}

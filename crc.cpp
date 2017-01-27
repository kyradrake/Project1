// crc.cpp
// Chat Room Client
// Homework 1
// Colin Banigan and Katherine Drake
// Due January 27, 2017

#include <sys/socket.h>
#include <iostream>
#include <string>
#include <sstream>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <unistd.h>

using namespace std;

int BUFFER_LENGTH = 2500;
struct sockaddr_in serverSocketAddr;
string username = "";

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

void* receiveMessages(void* argument) {
    // convert argument to socket
    int socket = *((int*) &argument);
    
    // create buffer for messages
    char buffer[BUFFER_LENGTH];
    memset(buffer, 0, sizeof(buffer));
    
    // wait for messages from server
    for (;;) {
        int receiveMessage = recv(socket, buffer, sizeof(buffer), 0);
        if (receiveMessage > 0) {
            vector<string> splitBuffer = split(buffer, ' ');
            if(splitBuffer[0] != (username + ":")){
                cout << buffer << endl;
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }
}

void* sendMessages(void* argument) {
    // convert argument to socket
    int socket = *((int*) &argument);
    
    // create buffer for messages
    char buffer[BUFFER_LENGTH];
    memset(buffer, 0, sizeof(buffer));
    
    // wait for client input
    for (;;) {
        string message;
        getline(cin, message);
        message = username + ": " + message;
        strcpy(buffer, message.c_str());
        
        int sendMessage = sendto(socket, buffer, BUFFER_LENGTH, 0, (struct sockaddr *)
                                 &serverSocketAddr, sizeof(serverSocketAddr));
        if (sendMessage < 0) {
            cout << "ERROR: Message Failed to Send to Server"; 
        } 
        memset(buffer, 0, sizeof(buffer));
    }
}

int createTCPSocket(const char* name, const char* port){
    // Create Client Socket - UNIX Socket, TCP Socket
    int nSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    struct hostent *hostentPtr;
    hostentPtr = gethostbyname(name);
    if(hostentPtr == NULL) {
        cout << "ERROR: Server Host Name Could Not Be Resolved... Program is Terminating...\n";
        return 0;
    }
    
    memcpy(&serverSocketAddr.sin_addr, hostentPtr->h_addr, sizeof(serverSocketAddr.sin_addr));
    serverSocketAddr.sin_family = AF_INET;
    serverSocketAddr.sin_port = htons(stoi(port));
    serverSocketAddr.sin_addr.s_addr = INADDR_ANY;     //inet_addr(hostName);
    
    int connection = connect(nSocket, (struct sockaddr*) &serverSocketAddr, 
                             sizeof(serverSocketAddr));
    if (connection < 0) {
        cout << "ERROR: Connection to Server Failed... Program is Terminating... \n";
        return 0;
    }
    else { 
        cout << "Connected to Server " << name << ":" << port << endl;
    }
    
    return nSocket;
}

int main(int argc, char* argv[]) {
    pthread_t sendThread, receiveThread;
    int serverSocket, chatRoomSocket;
    
    if(argc != 3) {
        cout << "ERROR: Invalid Arguments... Program is Terminating...\n";
        return 0;
    }
    
    const char* hostName = argv[1];
    const char* portNumber = argv[2];
    
    char buffer[BUFFER_LENGTH];
    memset(buffer, 0, sizeof(buffer));
    
    // Create Client Socket - UNIX Socket, TCP Socket
    serverSocket = createTCPSocket(hostName, portNumber);
    
    // Wait for CREATE/DELETE/JOIN Command
    cout << "Please CREATE/DELETE/JOIN a Chat Room\n";
    string command = "";
    getline(cin,command);
    strcpy(buffer, command.c_str());
    
    cout << "My Buffer: " << buffer << endl;
    
    // Send CREATE/DELETE/JOIN Command to Server
    int sendMessage = sendto(serverSocket, buffer, BUFFER_LENGTH, 0, (struct sockaddr *) &serverSocketAddr, sizeof(serverSocketAddr));
    
    memset(buffer, 0, sizeof(buffer));
    if (sendMessage < 0) {
        cout << "ERROR: Message Failed to Send to Server\n"; 
    
        //SHOULD WE TERMINATE HERE?!?!?!
    } 
    
    // Wait for Message Back From Server
    int receiveMessage = recv(serverSocket, buffer, sizeof(buffer), 0);
    if (receiveMessage < 0) {
        cout << "ERROR: Message Failed to Receive from Server\n"; 
    }
    cout << "Server Says: " << buffer << endl;
    
    string str(buffer);
    vector<string> splitServerMessage = split(buffer, ' ');
    
    memset(buffer, 0, sizeof(buffer));
    
    close(serverSocket);
    
    if(splitServerMessage[0] == "SUCCESS"){
        //state chat information to client before connecting to chat room
        cout << "Received Success from Server, Terminating Client\n";
        
        return 0;
    } else if (splitServerMessage[0] == "ERROR:"){
        //state error received from the server
        cout << buffer;
        
        return -1;
    } else if ((splitServerMessage[0] == "JOINROOM") && splitServerMessage.size() > 2){
        const char* chatPortNumber = splitServerMessage[1].c_str();
        const char* chatNumberMembers = splitServerMessage[2].c_str();
        
        //state chat information to client before connecting to chat room
        cout << "Joining chat room " << chatPortNumber << " containing " << chatNumberMembers << " members\n";
        
        chatRoomSocket = createTCPSocket(hostName, chatPortNumber);
    } else {
        cout << "ERROR: Split Message not in Correct Format\n";
    }
    
    // Start thread for sending messages to server
    int createSendThread = pthread_create (&sendThread, NULL, sendMessages, (void*) chatRoomSocket);
    if (createSendThread < 0) {
        cout << "ERROR: Couldn't Create Send Thread\n";
    }
    
    // Start thread for receiving messages from server
    int createReceiveThread = pthread_create (&receiveThread, NULL, receiveMessages, (void*) chatRoomSocket);
    if (createReceiveThread < 0) {
        cout << "ERROR: Couldn't Create Receive Thread\n";
    }
    
    cout << "Enter username for this chat room (no whitespace): ";
    cin >> username;
    
    while(true) {
        continue;
    }
    
    return 0; 
}

//
// Created by Shankar Ganesh on 2019-10-26.
//

#include <iostream>
#include <sys/socket.h> // for socket(), connect(), send(), and recv()
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi() and exit()
#include <stdio.h>
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <dirent.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include "player.h"
#include <unordered_set>
#include <unordered_map>
#include "Game.h"
#include "Server.h"
#include <cstdio>
#include <ctime>
//#include <time.h>


const int BUFFERSIZE = 512;   // Size the message buffers
const int MAXPENDING = 10;    // Maximum pending connections

fd_set recvSockSet;   // The set of descriptors for incoming connections
int maxDesc = 0;      // The max descriptor
bool terminated = false;
size_t length = 0;
int bytesRecv;
int nyes = -1;
int timeout = 60;
std::clock_t timeout_start;

extern std::unordered_map <int, struct player> players;
extern std::unordered_map <int, struct player> queue;
extern bool inGame;
std::unordered_set <int> assigned_sock;

void initServer (int&, int port);
void processSockets (fd_set);
void askName(int sock, char * buffer);
void print_wait(bool enough, float time);
void send_new_name(std::string name);
void sendAll(std::string toall);

int main(int argc, char *argv[])
{
    int serverSock;                  // server socket descriptor
    int clientSock;                  // client socket descriptor
    struct sockaddr_in clientAddr;   // address of the client

    struct timeval timeout = {0, 10};  // The timeout value for select()
    struct timeval selectTime;
    fd_set tempRecvSockSet;            // Temp. receive socket set for select()

    // Initilize the server
    initServer(serverSock, atoi(argv[1]));

    // Clear the socket setsck.count(sock) == 0){
      |                                 ^~~~
make: *** [Makefile:7: Server] Error 1

    FD_ZERO(&recvSockSet);

    // Add the listening socket to the set
    FD_SET(serverSock, &recvSockSet);

    maxDesc = std::max(maxDesc, serverSock);

    // Run the server until a "terminate" command is received)
    while (!terminated)ck.count(sock) == 0){
      |                                 ^~~~
make: *** [Makefile:7: Server] Error 1

    {

        // copy the receive descriptors to the working set
        memcpy(&tempRecvSockSet, &recvSockSet, sizeof(recvSockSet));

        // Select timeout has to be reset every time before select() is
        // called, since select() may update the timeout parameter to
        // indicate how much time was left.
        selectTime = timeout;
        int ready = select(maxDesc + 1, &tempRecvSockSet, NULL, NULL, &selectTime);
        if (ready < 0)
        {
            std::cout << "select() failed" << std::endl;
            break;
        }

        // First, process new connection request, if any.
        if (FD_ISSET(serverSock, &tempRecvSockSet))
        {
            // set the size of the client address structure
            unsigned int size = sizeof(clientAddr);

            // Establish a connection
            if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &size)) < 0)
                break;
            std::cout << "Accepted a connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << clientAddr.sin_port << std::endl;

            // Add the new connection to the receive socket set
            FD_SET(clientSock, &recvSockSet);
            maxDesc = std::max(maxDesc, clientSock);
            char * buffer = new char[BUFFERSIZE];
            askName(clientSock, buffer);
            assigned_sock.insert(clientSock);
            if (get_num_players() == 1){
                timeout_start = std::clock();
            }
            std::clock_t time = timeout_start-std::clock();
            float sec = ((float) time)/CLOCKS_PER_SEC;
            print_wait(1, sec);
        }else{
            processSockets(tempRecvSockSet);
        }
    }

    for (int sock = 0; sock <= maxDesc; sock++)
    {
        if (FD_ISSET(sock, &recvSockSet))
            close(sock);
    }

    // Close the server sockets
    close(serverSock);
}

void initServer(int& serverSock, int port)
{
    struct sockaddr_in serverAddr;

    if ((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        std::cout << "socket() failed" << std::endl;
        exit(1);
    }

    int yes = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        std::cout << "setsockopt() failed" << std::endl;
        exit(1);
    }
//    std::cout << "B1" << std::endl;
    // Initialize the server information
    // Note that we can't choose a port less than 1023 if we are not privileged users (root)
    memset(&serverAddr, 0, sizeof(serverAddr));         // Zero out the structure
    serverAddr.sin_family = AF_INET;                    // Use Internet address family
    serverAddr.sin_port = htons(port);                  // Server port number
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);     // Any incoming interface

    // Bind to the local address
    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cout << "bind() failed" << std::endl;
        exit(1);
    }

    // Listen for connection requests
    if (listen(serverSock, MAXPENDING) < 0)
    {
        std::cout << "listen() failed" << std::endl;
        exit(1);
    }
}

void processSockets (fd_set readySocks)
{
    char* buffer = new char[BUFFERSIZE];       // Buffer for the message from the server
    int size;                                    // Actual size of the message
    // Loop through the descriptors and process
    for (int sock = 0; sock <= maxDesc; sock++)
    {
        if (players.count(sock) != 1){
            continue;
        }
//        if (!inGame){
//            timeout_start = timeout_start-std::clock();
//            float sec = ((float) timeout_start)/CLOCKS_PER_SEC;
//            if ( sec >= (float)timeout){
//                timeout_start = clock();
//                if (get_num_players() > 1){
//                    for (int i = 0; i < maxDesc; i++){
//                        if (players.find(i) == find.end()){
//                            continue;
//                        }
//                        std::string send_msg = "Starting game in 5 seconds";
//                        length = send_msg.length();
//                        sendData(sock, (char *) &length, sizeof(length));
//                        sendData(sock, (char *) send_msg.c_str(), length);
//                        sleep(3);
//                        send_msg = "Starting game in 2 seconds";
//                        length = send_msg.length();
//                        sendData(sock, (char *) &length, sizeof(length));
//                        sendData(sock, (char *) send_msg.c_str(), length);
//                        sleep(2);
//                    }

//                    for (int i = sock; i < maxDesc; i++) {
//                        if (socks.count(sock) != 1) {
//                            continue;
//                        }
//                        std::string send_msg;
//                        send_msg = "Starting game...";
//                        length = send_msg.length();
//                        sendData(sock, (char *) &length, sizeof(length));
//                        sendData(sock, (char *) send_msg.c_str(), length);
//                    }
//                }else{
//                        print_wait(0, sec);
//            }
//        }

        if (FD_ISSET(sock, &readySocks) == 0){
            continue;
        }

        length = recv(sock, (unsigned char *) &length, sizeof(length), 0);
        receiveData(sock, buffer, size);
        length = 0;
        memset(buffer, 0, BUFFERSIZE);

    }

    delete[] buffer;
}

void receiveData (int sock, char* inBuffer, int& size)
{
    size = recv(sock, (char *) inBuffer, BUFFERSIZE, 0);

    if (size <= 0)
    {
        std::cout << "recv() failed, or the connection is closed. " << std::endl;
        FD_CLR(sock, &recvSockSet);

        while (FD_ISSET(maxDesc, &recvSockSet) == false){
            maxDesc -= 1;
//            players.erase(sock);
        }

        return;
    }
}

void sendData (int sock, char* buffer, int size)
{
    int bytesSent = 0;

    bytesSent = send(sock, (char *) buffer, size, 0);

    if (bytesSent <= 0 || bytesSent != size)
    {
        std::cout << "error in sending" << std::endl;
        return;
    }
}

void askName(int sock, char * buffer){
    length = 0;
    int size;
    std::string msg_send = "Please enter your name: ";
    length = msg_send.length();
    sendData(sock, (char *) &length, sizeof(length));
    sendData(sock, (char *) msg_send.c_str(), length);
    length = 0;
    while(length > 12 || length <= 0){
        bytesRecv = 0;
        recv(sock, (unsigned char *) &length, sizeof(length), 0);
        receiveData(sock, buffer, size);
        if (length > 12 || length <= 0) {
            msg_send = "Please enter a valid and unique name (at most 12 characters): ";
            length = msg_send.length();
            sendData(sock, (char *) &length, sizeof(length));
            sendData(sock, (char *) msg_send.c_str(), length);
            length = 0;
            continue;
        }
        if (get_num_players() == 0){
            add_player(sock, std::string(buffer));
            break;
        }
        int  i = 0;
        for (i = 0; i < maxDesc; i++){
            if (assigned_sock.count(sock) == 0){
                continue;
            }
            if (std::string(buffer).compare(players[i].player_name) == 0){
                break;
            }
        }
        if (i == get_num_players()){
            add_player(sock, std::string(buffer));
            break;
        }
        length = 0;
    }
    return;
}

void print_wait(bool enough, float time){
    std::string msg;
    if (enough){
       msg = "Approximate wait time before game starts:" + std::to_string(time) + "seconds\nWaiting for additional players\n\n"
              "Current players in queue: ";
    }else{
        msg = "Not enough players to start a game. At least 2 players required.\n 1 minute timer will reset.\n"
              "Approximate wait time before game starts: 60 seconds\nWaiting for additional players\n\n"
              "Current players in queue: ";
    }
    for (int i = 0; i < maxDesc; i++){
        if (assigned_sock.count(i) == 0){
            continue;
        }
        msg = msg + players[i].player_name + " ";
    }
    sendAll(msg);
    return;
}

void send_new_name(std::string name){
    std::string msg = name + "has joined the the game.\n";
    sendAll(msg);
    std::clock_t time = timeout_start-std::clock();
    float sec = ((float) time)/CLOCKS_PER_SEC;
    print_wait(1, sec);
}


void sendAll(std::string toall){
    length = toall.length();
    for (int sock = 0; sock < maxDesc; sock++){
        if (assigned_sock.count(sock) == 0){
            continue;
        }
        sendData(sock, (char *) &length, sizeof(length));
        sendData(sock, (char *) toall.c_str(), length);
    }
    return;
}

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
#include <unordered_map>
#include "Game.h"
#include "Server.h"
#include <cstdio>
#include <ctime>


const int BUFFERSIZE = 512;   // Size the message buffers
const int MAXPENDING = 10;    // Maximum pending connections

fd_set recvSockSet;   // The set of descriptors for incoming connections
int maxDesc = 0;      // The max descriptor
bool terminated = false;
size_t length = 0;
int bytesRecv;
int nyes = -1;

std::unordered_map <int, bool> status;
extern std::unordered_map <int, struct player> players;
extern std::unordered_map <int, struct player> queue;
extern bool inGame;

void initServer (int&, int port);
void processSockets (fd_set);
void askName(int sock, char * buffer);
void current_status(int);

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

    // Clear the socket sets
    FD_ZERO(&recvSockSet);

    // Add the listening socket to the set
    FD_SET(serverSock, &recvSockSet);

    maxDesc = std::max(maxDesc, serverSock);

    // Run the server until a "terminate" command is received)
    while (!terminated)
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
//            std::cout << "Client Sock = " << clientSock << std::endl;
            status[clientSock] = 0;
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
        current_status(sock);
        if (FD_ISSET(sock, &readySocks) == 0){
            continue;
        }
//        std::cout << "Sock = " << std::endl;

//        std::string msg_send = "Please enter a valid name (at most 12 characters)";
//        length = msg_send.length();
//        sendData(sock, (char *) &length, sizeof(length));
//        sendData(sock, (char *) msg_send.c_str(), length);
//        length = 0;

//        if (status[sock] == 0 ){
//            continue;
//        }
        std::cout << "NUwdwdwdM ++++++ ==== " << get_num_players() << std::endl;
        length = recv(sock, (unsigned char *) &length, sizeof(length), 0);
        receiveData(sock, buffer, size);
        std::cout << buffer << std::endl;
        std::cout << "NUM +++ccc+++ ==== " << get_num_players() << std::endl;
        if (std::string(buffer).compare("Y") == 0) {
            status[sock] = 1;
            if (nyes == -1) {
                nyes = 0;
            }
            nyes += 1;
            if (nyes == get_num_players()) {
                std::cout << "NUM ++++++ ==== " << get_num_players() << std::endl;
                std::cout << "NYES +++++vdkv== " << nyes << std::endl;
                start_game(100);
                inGame = true;
            }
        }

        length = 0;
        memset(buffer, 0, BUFFERSIZE);
//        std::string msg_send;


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
            players.erase(sock);
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
    std::cout << "B2" << std::endl;
    while(length > 12 || length <= 0){
        bytesRecv = 0;
        recv(sock, (unsigned char *) &length, sizeof(length), 0);
        receiveData(sock, buffer, size);
        if (length > 12 || length <= 0) {
            msg_send = "Please enter a valid name (at most 12 characters): ";
            length = msg_send.length();
            sendData(sock, (char *) &length, sizeof(length));
            sendData(sock, (char *) msg_send.c_str(), length);
            length = 0;
        }
        if (length < 12 && length > 0) {
            add_player(sock, std::string(buffer));
            break;
        }
    }
    return;
}


void current_status(int sock){

    if (status.count(sock) <= 0){
        return;
    }
    if (status[sock] == 1){
        return;
    }
    std::string msg_send;
    std::cout << "E2" << std::endl;
    if (get_num_players() >= 2 && !inGame) {
        msg_send = "Start game with " + std::to_string(get_num_players()) + " players? [y/N]: ";
        length = msg_send.length();
        sendData(sock, (char *) &length, sizeof(length));
        sendData(sock, (char *) msg_send.c_str(), length);
        length = 0;
        std::unordered_map<int, bool>::iterator it = status.begin();
        while (it != status.end()){
            it->second = 0;
            it++;
        }
        status[sock] = 1;
    } else if (get_num_players() < 2 && !inGame) {
        msg_send = "Please wait for more players to start the game";
        length = msg_send.length();
        sendData(sock, (char *) &length, sizeof(length));
        sendData(sock, (char *) msg_send.c_str(), length);
        length = 0;
        status[sock] = 1;
    } else if (inGame) {
        msg_send = "Please wait for the current game to complete. You've been added to the queue";
        length = msg_send.length();
        sendData(sock, (char *) &length, sizeof(length));
        sendData(sock, (char *) msg_send.c_str(), length);
        length = 0;
        status[sock] = 1;

    }
}

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
#include <pthread.h>

const int BUFFERSIZE = 512;   // Size the message buffers
const int MAXPENDING = 10;    // Maximum pending connections

fd_set recvSockSet;   // The set of descriptors for incoming connections
//fd_set tempset;
int maxDesc = 0;      // The max descriptor
bool terminated = false;
size_t length = 0;
int bytesRecv;
float timeout_game = 10.0;
std::clock_t timeout_start;

extern std::unordered_map <int, struct player> players;
extern std::unordered_map <int, struct player> queue;
extern std::unordered_map <int, struct player> quit_players;
extern volatile bool inGame;

std::unordered_set <int> assigned_sock;
std::unordered_set<int>::iterator it;
std::unordered_set<int>::iterator iter;
std::unordered_map <int, struct player>::iterator qit;

void initServer (int&, int port);
void processSockets (fd_set);
void askName(int sock, char * buffer);
void print_wait(bool enough, float time);
void send_new_name(std::string name);
void sendData (int sock, char* buffer, int size);
int handle_error(std::string error_message);
bool name_length_invalid(size_t name_length);

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

    FD_ZERO(&recvSockSet);

    // Add the listening socket to the set
    FD_SET(serverSock, &recvSockSet);

    maxDesc = std::max(maxDesc, serverSock);

    // Run the server until a "terminate" command is received)
    while (!terminated){

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
            if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &size)) < 0){
                break;
            }
            std::cout << "Accepted a connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << clientAddr.sin_port << std::endl;

            // Add the new connection to the receive socket set
            FD_SET(clientSock, &recvSockSet);
            maxDesc = std::max(maxDesc, clientSock);

            char * buffer = new char[BUFFERSIZE];
            assigned_sock.insert(clientSock);
            askName(clientSock, buffer);
            if (get_num_players() == 1){
                timeout_start = std::clock();
                print_wait(1, timeout_game);
            }

        }else{
            processSockets(tempRecvSockSet);
        }
    }

    for (int sock = 0; sock <= maxDesc; sock++)
    {
        if (FD_ISSET(sock, &recvSockSet))
            close(sock);
    }

    close(serverSock);
}

void initServer(int& serverSock, int port)
{
    struct sockaddr_in serverAddr;

    if ((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        handle_error("socket() failed");

    int yes = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        handle_error("setsockopt() failed");

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind to the local address
    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        handle_error("bind() failed");

    // Listen for connection requests
    if (listen(serverSock, MAXPENDING) < 0)
        handle_error("listen() failed");

    std::cout << "Server has started" << std::endl;
}

void processSockets (fd_set readySocks)
{
    char* buffer = new char[BUFFERSIZE];       // Buffer for the message from the server
    int size;
    int br;// Actual size of the message
    // Loop through the descriptors and process
    for (it = assigned_sock.begin(); it != assigned_sock.end(); it++)
    {

        if (!inGame){
            std::clock_t temp = std::clock() - timeout_start;
            float sec = ((float) temp)/CLOCKS_PER_SEC;
            if ( sec >= timeout_game){
                if (get_num_players() > 1){
                    inGame = 1;
                	sendAll("Starting game...");
                    pthread_t thread;
                    pthread_create(&thread, NULL, start_thread, (void*)&readySocks);
                }else {
                    print_wait(0, sec);
                    timeout_start = std::clock();
                }
            }
        }

    }
    delete[] buffer;
}

void receiveData (int sock, char* inBuffer, int& size){
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

bool name_length_invalid(size_t name_length){
    return length > 12 || length < 2;
}

void askName(int sock, char * buffer){
    length = 0;
    int size;
    std::string msg_send = "Please enter your name: ";
    length = msg_send.length();
    sendData(sock, (char *) &length, sizeof(length));
    sendData(sock, (char *) msg_send.c_str(), length);
    length = 0;
    while(name_length_invalid(length)){
        memset(buffer, 0, BUFFERSIZE);
        bytesRecv = 0;
        bytesRecv = recv(sock, (unsigned char *) &length, sizeof(length), 0);
        receiveData(sock, buffer, size);
        std::string name = std::string(buffer);
        name.erase(std::remove(name.begin(), name.end(), '\n'),name.end());
        if (name_length_invalid(length)) {
            msg_send = "Please enter a valid and unique name (at least 1 and at most 12 characters): ";
            length = msg_send.length();
            sendData(sock, (char *) &length, sizeof(length));
            sendData(sock, (char *) msg_send.c_str(), length);
            length = 0;
            continue;
        }
        if (get_num_players() == 0){
            assigned_sock.insert(sock);
            add_player(sock, name);
            break;
        }

        for (qit = players.begin(); qit != players.end(); qit++){
            if (name.compare(qit->second.player_name) == 0){
                break;
            }
        }

        if (qit == players.end()){
            assigned_sock.insert(sock);
            add_player(sock, name);
            send_new_name(name);
            break;
        }
        length = 0;
        msg_send = "Please enter a valid and unique name (at most 12 characters): ";
        send(msg_send, sock);
        continue;
    }
    return;
}

void print_wait(bool enough, float time){
    std::unordered_map <int, struct player>::iterator names;
    std::string msg = "";
    if (!inGame) {
        if (enough) {
            msg = "Approximate wait time before game starts =" + std::to_string(time) +
                  " seconds\nWaiting for additional players\n\n"
                  "Current players in queue = ";
        } else {
            msg = "Not enough players to start a game. At least 2 players required.\n 1 minute timer will reset.\n"
                  "Approximate wait time before game starts: 60 seconds\nWaiting for additional players\n\n"
                  "Current players in queue = ";
        }

        for (names = players.begin(); names != players.end(); names++) {
            msg = msg + names->second.player_name + " ";
        }
    }else{
        msg = "Game in progress. Please wait for the current game to complete.\nApproximate wait time for the current game to complete = "
                + std::to_string(get_time_remaining()) + "\nPlayers in the queue for the next game = ";
        for (names = queue.begin(); names != queue.end(); names++){
            msg = msg + names->second.player_name + " ";
        }
    }
    msg += "\n\n";
    sendAll(msg);
}

void send_new_name(std::string name){
    std::string msg;
    if (!inGame) {
        msg = name + " has joined the the game.\n";
    }else{
        msg = name + " has joined the the queue.\n";
    }
    sendAll(msg);
    std::clock_t time = std::clock() - timeout_start;
    float sec = ((float) time)/CLOCKS_PER_SEC;
    print_wait(1, timeout_game-sec);
}

void sendAll(std::string toall){

    length = toall.length();

    for (iter = assigned_sock.begin(); iter != assigned_sock.end(); iter++)
	{
        int sock = *iter;
		sendData(sock, (char *) &length, sizeof(length));
		sendData(sock, (char *) toall.c_str(), length);
    }

}

void send(std::string msg, int sock)
{
    length = msg.length();
    sendData(sock, (char *) &length, sizeof(length));
    sendData(sock, (char *) msg.c_str(), length);
}

void recv_length(int sock, size_t len_string, char * buffer)
{
    recv(sock, (unsigned char *) &len_string, sizeof(len_string), 0);
    int size;
    receiveData(sock, buffer, size);
}

int handle_error(std::string error_message){
    std::cout << error_message << std::endl;
    exit(1);
}

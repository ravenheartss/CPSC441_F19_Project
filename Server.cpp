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
#include <signal.h>
#include <ctime>
#include <pthread.h>

const int BUFFERSIZE = 512;   // Size the message buffers
const int MAXPENDING = 10;    // Maximum pending connections

fd_set recvSockSet;   // The set of descriptors for incoming connections
fd_set tempset;
extern fd_set backupSet;
int maxDesc = 0;      // The max descriptor
bool terminated = false;
size_t length = 0;
int bytesRecv;
int sentBytes;
float timeout_game = 10.0;
extern float total_time;
std::time_t timeout_start;

extern std::unordered_map <int, struct player> players;
extern std::unordered_map <int, struct player> queue;
extern std::unordered_map <int, struct player> quit_players;
extern volatile bool inGame;
volatile sig_atomic_t flag = 0;

std::unordered_set <int> assigned_sock;
std::unordered_set <int> queue_socket;
std::unordered_set<int>::iterator it;
std::unordered_set<int>::iterator iter;
std::unordered_map <int, struct player>::iterator qit;

void initServer (int&, int port);
void processSockets (fd_set);
void askName(int sock, char * buffer);
void print_wait(bool enough, float time);
void send_new_name(std::string name);
int sendData (int sock, char* buffer, int size);
int handle_error(std::string error_message);
bool name_length_invalid(size_t name_length);
void askTime(int,char *);


void handler(int interr){
    flag = 1;
    return;
}

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
            askName(clientSock, buffer);
            if (get_num_players() == 1){
                memset(buffer,0,BUFFERSIZE);
                askTime(clientSock, buffer);
                time(&timeout_start);
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
    for (int sock = 0; sock < maxDesc; sock++)
    {
        if (!inGame){
            std::time_t now;
            std::time(&now);
            float sec = std::difftime(now, timeout_start);
            if ( sec >= timeout_game){
                if (players.size() > 1){
                	sendAll(1, "Starting game...");
                	if (flag){
                	    flag = 0;
                	    time(&timeout_start);
                        return;
                	}
                    inGame = 1;
                    pthread_t thread;
                    pthread_create(&thread, NULL, start_thread, (void*)&readySocks);
                }else {
                    print_wait(0, sec);
                    time(&timeout_start);
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
        player_quitting(sock);
        return;
    }
}

int sendData (int sock, char* buffer, int size)
{
    signal(SIGPIPE, handler);

    int bytesSent = 0;

    bytesSent = send(sock, (char *) buffer, size, 0);
    if (flag){
        player_quitting(sock);
        return bytesSent;
    }
    if (bytesSent <= 0 || bytesSent != size)
    {
        std::cout << "error in sending" << std::endl;
        player_quitting(sock);
        return bytesSent;
    }

    return bytesSent;
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
            sentBytes = sendData(sock, (char *) &length, sizeof(length));
            if (sentBytes <= 0){
                return;
            }
            sentBytes = sendData(sock, (char *) msg_send.c_str(), length);
            if (sentBytes <= 0){
                return;
            }
            length = 0;
            continue;
        }
        if (get_num_players() == 0){
            if(!inGame) {
                assigned_sock.insert(sock);
            }else{
                queue_socket.insert(sock);
            }
            add_player(sock, name);
            break;
        }

        for (qit = players.begin(); qit != players.end(); qit++){
            if (name.compare(qit->second.player_name) == 0){
                break;
            }
        }

        if (qit == players.end()){
            if(!inGame) {
                assigned_sock.insert(sock);
            }else{
                queue_socket.insert(sock);
            }
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
                  "Approximate wait time before game starts = 60 seconds\nWaiting for additional players\n\n"
                  "Current players in queue = ";
        }

        for (names = players.begin(); names != players.end(); names++) {
            msg = msg + names->second.player_name + " ";
        }
        msg += "\n\n";
        sendAll(1, msg);
    }else{
        msg = "Game in progress. Please wait for the current game to complete.\nApproximate wait time for the current game to complete = "
                + std::to_string(get_time_remaining()) + "\nPlayers in the queue for the next game = ";
        for (names = queue.begin(); names != queue.end(); names++){
            msg = msg + names->second.player_name + " ";
        }
        msg += "\n\n";
        sendAll(0, msg);
    }

}

void send_new_name(std::string name){
    std::string msg;
    if (!inGame) {
        msg = name + " has joined the the game.\n";
        sendAll(1, msg);
    }else{
        msg = name + " has joined the the queue.\n";
        sendAll(0, msg);
    }

    std::time_t now;
    std::time(&now);
    float sec = std::difftime(now, timeout_start);
    print_wait(1, timeout_game-sec);
}

void sendAll(bool isPlayer, std::string toall){

    length = toall.length();
    if (isPlayer) {
        for (iter = assigned_sock.begin(); iter != assigned_sock.end(); iter++) {
            int sock = *iter;
            sentBytes = sendData(sock, (char *) &length, sizeof(length));
            if (sentBytes <= 0){
                return;
            }
            sentBytes = sendData(sock, (char *) toall.c_str(), length);
            if (sentBytes <= 0){
                return;
            }
        }
    }else{
        for (iter = queue_socket.begin(); iter != queue_socket.end(); iter++) {
            int sock = *iter;
            sentBytes = sendData(sock, (char *) &length, sizeof(length));
            if (sentBytes <= 0){
                return;
            }
            sentBytes = sendData(sock, (char *) toall.c_str(), length);
            if (sentBytes <= 0){
                return;
            }
        }
    }

}

void send(std::string msg, int sock)
{
    length = msg.length();
    sentBytes = sendData(sock, (char *) &length, sizeof(length));
    if (sentBytes <= 0){
        return;
    }
    sentBytes = sendData(sock, (char *) msg.c_str(), length);
    if (sentBytes <= 0){
        return;
    }
}

void recv_length(int sock, size_t len_string, char * buffer)
{
    int size;
    size = recv(sock, (unsigned char *) &len_string, sizeof(len_string), 0);
    if (size <= 0)
    {
        player_quitting(sock);
        length = -1;
        return;
    }
    size = 0;
    receiveData(sock, buffer, size);
    return;
}


int handle_error(std::string error_message){
    std::cout << error_message << std::endl;
    exit(1);
}

void game_clear(){

    assigned_sock.clear();
    if (!queue.empty()){
        for (qit = queue.begin(); qit != queue.end(); qit++){
            players[qit->first] = qit->second;
            assigned_sock.insert(qit->first);
        }
        queue.clear();
        queue_socket.clear();
    }
    return;

}

void askTime(int sock, char * buffer){
    do{
        memset(buffer, 0, BUFFERSIZE);
        send("Please enter the time (in seconds) (must be greater than or equal to 30s) (default 180s (3min)): ", sock);
        recv_length(sock, length, buffer); //length has a value of 97 here (equal to the length of the above message, why do we want to receive 97 bytes, the same what we send????)
        if (std::string(buffer).compare("\n") == 0){
            total_time = 180;
            break;
        }
        if (length == -1){
            break; // we should probably retry here instead???? disconnect user????
        }
    }while(atoi(buffer) < 30 );
    if (atoi(buffer) >= 30){
        total_time = atoi(buffer);
    }
    memset(buffer, 0, BUFFERSIZE);
}

void player_quitting(int socket){

    FD_CLR(socket, &recvSockSet);

    while (FD_ISSET(maxDesc, &recvSockSet) == false){
        maxDesc -= 1;
    }

    if (inGame) {
        memcpy(&backupSet, &recvSockSet, sizeof(recvSockSet));
    }

    players.erase(socket);
    if (players.size() == 0 && inGame){
        finish_game();
    }

    maxDesc = 0;
    for (qit = players.begin(); qit != players.end(); qit++){
        maxDesc = std::max(maxDesc, qit->first);
    }
    assigned_sock.erase(socket);
    queue_socket.erase(socket);

    time(&timeout_start);

    return;
}
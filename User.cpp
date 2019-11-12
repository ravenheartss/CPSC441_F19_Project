/*
 * CPSC 441 Fall 2019
 * Group 3 Project
 */

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fstream>
#include <algorithm>
#include <signal.h>
#include <cstdio>
#include <ctime>
#include <pthread.h>

#define BUFFERSIZE 512

int handle_error(std::string error_message, int exit_with);


int main(int argc, char * argv[]) {

    int socket_desc;
    struct sockaddr_in serverAddr;
    size_t total_size;

    char inBuffer[BUFFERSIZE];
    char outBuffer[BUFFERSIZE];
    int bytesRecv;
    int bytesSent;

    bool force_quit = false;
    bool quit = false;

    // To be changed
    const char * server_IP = "127.0.0.1";
    int port = atoi(argv[1]);


//    signal(SIGINT, handle);

    if ((socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        handle_error("socket() failed", 1);

    int yes = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        handle_error("setsockopt() failed", 1);


    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(server_IP);

    if (connect(socket_desc, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
        handle_error( "connect() failed", 1);
//    pthread_t timekeeper;
//    pthread_create(&timekeeper, NULL, term_time, (void*)&socket_desc);
    while (!quit) {
        // Clear the buffers
        total_size = 0;
        memset(&outBuffer, 0, BUFFERSIZE);
        memset(&inBuffer, 0, BUFFERSIZE);
//        recieve_data(socket_desc, inBuffer);
        total_size = 0;
        bytesRecv = 0;
        bytesRecv = recv(socket_desc, (unsigned char*) &total_size, sizeof(size_t), 0);

        if (bytesRecv <= 0)
            handle_error("Error in receiving message from server. Try restarting the game", -1);

        bytesRecv = 0;
        while (bytesRecv < total_size) {
            bytesRecv += recv(socket_desc, (char *) inBuffer, total_size, 0);
            std::cout << inBuffer;
        }

        if (bytesRecv <= 0)
            handle_error("Error in receiving message from server. Try restarting the game", -1);

        if (std::string(inBuffer).find("Thank you for playing") != std::string::npos){
            quit = true;
            continue;
        }

        if (std::string(inBuffer).find_last_of(":") == std::string::npos){
            continue;
        }

        memset(&outBuffer, 0, BUFFERSIZE);
        fgets(outBuffer, BUFFERSIZE, stdin);
        total_size = strlen(outBuffer);

        bytesSent = send(socket_desc, (char *) &total_size, sizeof(total_size), 0);

        if (bytesSent <= 0) {
            handle_error("Error in receiving message from server. Try restarting the game", -1);
        }

        bytesSent = send(socket_desc, (char *) &outBuffer, total_size, 0);

        if (bytesSent < 0 || bytesSent != total_size) {
            handle_error("Error in receiving message from server. Try restarting the game", -1);
        }
    }

    close(socket_desc);
    std::cout << "Thank you for playing!" << std::endl;
    exit(0);
}

int handle_error(std::string error_message, int exit_with){
    std::cout << error_message << std::endl;
    exit(exit_with);
}



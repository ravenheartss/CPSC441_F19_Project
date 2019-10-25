/*
 * CPSC 441 Fall 2019
 * Group 3 Project
 */

// Author: Shankar Ganesh

// Include necessary libraries
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

#define BUFFERSIZE 512

volatile sig_atomic_t force_quit = 0;

void handle(int signum) {
    force_quit = 1;
}

int main() {

    int socket_desc;
    struct sockaddr_in serverAddr;

    char inBuffer[BUFFERSIZE];
    char outBuffer[BUFFERSIZE];
    int bytesRecv;
    int bytesSent;
    size_t total_size;

    bool force_quit;
    bool quit;

    // To be changed
    const char * server_IP = "192.168.1.1";
    int port = 2055;


    signal(SIGINT, handle);

    if ((socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        std::cout << "socket() failed" << std::endl;
        exit(1);
    }

    int yes = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        std::cout << "setsockopt() failed" << std::endl;
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(server_IP);

    if (connect(socket_desc, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cout << "connect() failed" << std::endl;
        exit(1);
    }

    while (!force_quit && !quit) {
        // Clear the buffers
        memset(&outBuffer, 0, BUFFERSIZE);
        memset(&inBuffer, 0, BUFFERSIZE);

        total_size = 0;
        bytesRecv = recv(socket_desc, (unsigned char *) &total_size, sizeof(size_t), 0);

        if (bytesRecv <= 0) {
            std::cout << "Error in receiving message from server. Try restarting the game" << std::endl;
        }

        while (bytesRecv < total_size && bytesRecv > 0) {
            bytesRecv += recv(socket_desc, (char *) inBuffer, BUFFERSIZE, 0);
            std::cout << inBuffer;
        }

        if (bytesRecv <= 0) {
            std::cout << "Error in receiving message from server. Try restarting the game" << std::endl;
        }

        fgets(outBuffer, BUFFERSIZE, stdin);
        total_size = strlen(outBuffer);

        bytesSent = send(socket_desc, (char *) &total_size, sizeof(total_size), 0);

        if (bytesSent < 0 || bytesSent != total_size) {
            std::cout << "Error in sending. Try restarting the game." << std::endl;
            exit(-1);
        }

        bytesSent = send(socket_desc, (char *) &outBuffer, total_size, 0);

        if (bytesSent < 0 || bytesSent != total_size) {
            std::cout << "Error in sending. Try restarting the game." << std::endl;
            exit(-1);
        }

        std::string check = std::string(outBuffer);
        if (check.compare("quit")) {
            quit = true;
        }
    }

    close(socket_desc);
    std::cout << "Thank you for playing!" << std::endl;
    exit(0);
}

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

volatile sig_atomic_t flag = 0;
int socket_desc;
size_t total_size;

void handle(int interr){
    std::string quitting = "mequitting";
    total_size = quitting.length();
    send(socket_desc, (char *) &total_size, sizeof(total_size), 0);
    send(socket_desc, (char *) quitting.c_str(), total_size, 0);
    flag = 1;
    std::cout << "Thank you for playing!" << std::endl;
    close(socket_desc);
    exit(-1);
}

int handle_error(std::string error_message, int exit_with);

int main(int argc, char * argv[]) {


    struct sockaddr_in serverAddr;


    char inBuffer[BUFFERSIZE];
    char outBuffer[BUFFERSIZE];
    int bytesRecv;
    int bytesSent;

    bool force_quit = false;
    bool quit = false;

    // To be changed
    const char * server_IP = "127.0.0.1";
    int port = atoi(argv[1]);


    signal(SIGINT, handle);

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

//    signal(SIGINT, sig_handler);
//    signal(SIGKILL, sig_handler);
//    signal(SIGTERM, sig_handler);

    while (!quit && !flag) {
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
    exit(0);
}

int handle_error(std::string error_message, int exit_with){
    std::cout << error_message << std::endl;
    exit(exit_with);
}





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
#include <sys/select.h>
#include <sys/types.h>
#include "player.h"
#include "Game.h"
#include <unordered_map>
#include "wrdgen.h"
#include "Server.h"
#include <cstdio>
#include <ctime>
#include <utility>
#include <signal.h>
#include <algorithm>

#define BUFFERSIZE 512

std::unordered_map <int, struct player> players;
std::unordered_map <int, struct player> queue;
std::unordered_map <int, struct player> quit_players;
std::vector <struct player *> for_sorting;
std::vector <struct player *>::iterator it1;
std::unordered_map <int, struct player>::iterator it2;


bool inGame = false;
std::vector<std::string> word_list;
std::clock_t start;
float total_time = 180.0;
size_t len = 0;


bool descending (const player *struct1, const player *struct2){

    return (struct1->rate > struct2->rate);

}

void add_player(int sock_no, std::string name){
    struct player info;
    info.socket = sock_no;
    info.n_typed = 0;
    info.pos = 0;
    info.rate = 0;
    info.characters = 0;
    info.errors = 0;
    info.player_name = name;
    if (!inGame) {
        players[sock_no] = info;
        struct player *toadd = &info;
        for_sorting.push_back(toadd);
    }else{
        queue[sock_no] = info;
    }
}

void delete_player(int sock_no){
    quit_players[sock_no] = players[sock_no];
    for (it1 = for_sorting.begin(); it1 != for_sorting.end(); it1++){
        struct player *me = *it1;
        if (me->socket == sock_no){
            for_sorting.erase(it1);
            break;
        }
    }
    players.erase(sock_no);
    queue.erase(sock_no);
}

std::string get_word(int sock_no){
    if (sock_no == -1){
        return word_list[0];
    }
    if (players[sock_no].pos >= word_list.size()){
        players[sock_no].pos = 0;
        return word_list[0];
    }else{
        return word_list[players[sock_no].pos];
    }
}

std::unordered_map <int, struct player> get_players(){
    return players;
}

void clear_list(){
    word_list.clear();
}


int get_num_players(){
    return players.size();
}

void start_game(int num, fd_set readySocks)
{
    //inGame = true;
//    std::cout << "Before generate" << std::endl;
    word_list=generate_random(num);
//    std::cout << "after generate words before print list" << std::endl;
//    for (int i=0; i<word_list.size(); i++){
//        std::cout << word_list[i] << std::endl;
//    }
//    std::cout << "after generate" << std::endl;
//    std::string word = "Type: " + get_word(-1);
//    for (int i = 0; i++; )

    game_loop(readySocks);

}

void game_loop(fd_set readySocks)
{
//    while (get_time_remaining() < 180)
//    for (auto player : players){
//        int playersock = player.first;
////        std::cout << "did it make it here? lets test" << std::endl;
//        send(word_list[1],playersock);
//    }
    sendAll("Game starting");
    display_all();
    std::string first_word = "\nType: " + get_word(-1) + "\n";
    sendAll(first_word);
    start = std::clock();
    while (get_time_remaining() < 180){
        monitor_sockets(readySocks);
    }
}

void finish_game(){
    display_all();
    players.clear();
    quit_players.clear();

    kill(getpid(),SIGKILL);
}

void check(int sock_no, std::string typed){
    struct player info = players[sock_no];
    if (typed.compare(word_list[info.pos]) == 0){
        info.pos++;
    }
}


void display(int sock){
    sort_ranks();
    std::string fmt = "Time Remaining = " + std::to_string(get_time_remaining());
	fmt += "Rank  Name          Speed    Errors\n\n";
    send(fmt, sock);
    fmt = "";
    int i = 1;
    for (it1 = for_sorting.begin(); it1 != for_sorting.end(); it2++){
        fmt = fmt + std::to_string(i) + ")" + "    " + (*it1)->player_name;
        int j = 14 - (*it1)->player_name.length();
        while( j != 0){
            fmt += " ";
            j++;
        }
        fmt += std::to_string((*it1)->rate);
        j = 9 - std::to_string((*it1)->rate).length();
        while(j != 0){
            fmt += " ";
            j++;
        }
        fmt += std::to_string((*it1)->errors) + "\n";
    }
    send(fmt, sock);
}

void display_all(){
    std::unordered_map <int, struct player>::iterator temp;
    sort_ranks();
    std::string fmt = "Time Remaining = " + std::to_string(get_time_remaining());
    fmt += "\n\nRank  Name          Speed    Errors\n\n";
    sendAll(fmt);
    fmt = "";
    int i = 1;
    for (temp = players.begin(); temp != players.end(); temp++){
        fmt = fmt + std::to_string(i) + ")" + "    " + temp->second.player_name;
        int j = 14 - temp->second.player_name.length();
        while( j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string(temp->second.rate);
        j = 9 - std::to_string(temp->second.rate).length();
        while(j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string(temp->second.errors) + "\n";
        i++;
    }
    sendAll(fmt);
}

void sort_ranks(){
    sort(for_sorting.begin(), for_sorting.end(), descending);
}

float get_time_remaining(){
    std::clock_t diff = std::clock() - start;
    float elapsed = (float)diff/CLOCKS_PER_SEC;
    return (total_time - elapsed);
}

void monitor_sockets(fd_set readySocks){
    char * buff = new char[BUFFERSIZE];
    std::string input;
    int size;
    int br;
    for (it2 = players.begin(); it2 != players.end(); it2++)
    {
        /*
         * THIS PART IS NOT WORKING!!!!!!!!!!
         */
        int sock = it2->second.socket;
        if(!FD_ISSET(sock, &readySocks)){
            continue;
        }
        memset(buff, 0, BUFFERSIZE);
        br = 0;
        br = recv(sock, (unsigned char *) &len, sizeof(len), 0);
        receiveData(sock, buff, size);
        std::cout << "YAYYYYY!!!!!!" << std::endl;
        input = std::string(buff);
        input.erase(std::remove(input.begin(), input.end(), '\n'),input.end());
        check(sock, input);
        display(sock);
        input = "Type: " + get_word(sock) + "\n";
        send(input, sock);
    }
    delete[] buff;
}




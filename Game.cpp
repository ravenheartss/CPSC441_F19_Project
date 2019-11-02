

#include <iostream>
#include <sys/socket.h> // for socket(), connect(), send(), and recv()
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi() and exit()
#include <stdio.h>
#inlcude <signal.h>
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <dirent.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include "player.h"
#include "Game.h"
#include <unordered_map>
#include "wrdgen.h"
#include "Server.h"
#include <cstdio>
#include <ctime>
#include <utility>


std::unordered_map <int, struct player> players;
std::unordered_map <int, struct player> queue;

bool inGame = false;
std::vector<std::string> word_list;
std::clock_t start;
double total_time;



void add_player(int sock_no, std::string name){
    struct player info;
    info.socket = sock_no;
//    info.n_typed = 0;
//    info.pos = 0;
//    info.rate = 0;
    info.player_name = name;
    if (!inGame) {
        players.insert(std::make_pair(sock_no, info));
    }else{
        queue.insert(std::make_pair(sock_no, info));
    }
    std::cout << "Players size == " << get_num_players() << std::endl;
}

void delete_player(int sock_no){
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
        players[sock_no].pos += 1;
        return word_list[players[sock_no].pos];
    }
}

std::unordered_map <int, struct player> get_players(){
    return players;
}



void clear_list(){
    word_list.clear();
}

//struct player get_player(int sock_no){
//    return players[sock_no];
//}



int get_num_players(){
    return players.size();
}




void game_loop(){
//    sendAll("Game Starting");
//    sendAll(word_list[0]);
    for (auto player : players){
        int playersock = player.first;
        std::cout << "did it make it here? lets test" << std::endl;
        send(word_list[1],playersock);}
}

void start_game(int num){
    //inGame = true;
    std::cout << "Before generate" << std::endl;
    word_list=generate_random(num);
    std::cout << "after generate words before print list" << std::endl;
    for (int i=0; i<word_list.size(); i++){
        std::cout << word_list[i] << std::endl;
    }
    std::cout << "after generate" << std::endl;
    std::string word = "Type: " + get_word(-1);
//    for (int i = 0; i++; )
    start = std::clock();
    game_loop();

}

void finish_game(){
    kill(0,SIGKILL);
}


void check(int sock_no, std::string typed){
    struct player info = players[sock_no];
    if (typed.compare(word_list[info.pos]) == 0){
        info.pos++;
    }
}




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
#include "Game.h"
#include <unordered_map>
#include "wrdgen.h"
#include "Server.h"
#include <cstdio>
#include <ctime>
#include <utility>

std::unordered_map <int, struct player> players;
std::unordered_map <int, struct player> queue;
std::vector <struct player *> for_sorting;

bool inGame = false;
std::vector<std::string> word_list;
std::clock_t start;
double total_time;
size_t len = 0;

void add_player(int sock_no, std::string name){
    struct player info;
    info.socket = sock_no;
    info.n_typed = 0;
    info.pos = 0;
    info.rate = 0;
    info.player_name = name;
    if (!inGame) {
        players.insert(std::make_pair(sock_no, info));
        for_sorting.push_back(&players[sock_no]);
    }else{
        queue.insert(std::make_pair(sock_no, info));
    }
    std::cout << "Players smake ize == " << get_num_players() << std::endl;
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

void generate_words(int num){
    generate_random(num, word_list);
}

void clear_list(){
    word_list.clear();
}


int get_num_players(){
    return players.size();
}

void start_game(int num){

    generate_words(num);

    std::string word = "Type: " + get_word(-1);
//    for (int i = 0; i++; )
    start = std::clock();
//    game_loop();
}

void game_loop(){
    sendAll("Game Starting");
}

void check(int sock_no, std::string typed){
    struct player info = players[sock_no];
    if (typed.compare(word_list[info.pos]) == 0){
        info.pos++;
    }
}

void display(int sock){
	std::string fmt = "Rank  Name          Speed    Errors\n\n";
    len = fmt.length();
    sendData(sock, (char *) &len, sizeof(len));
    sendData(sock, (char *) fmt.c_str(), len);
    
}



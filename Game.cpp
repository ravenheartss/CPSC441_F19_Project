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
#include <pthread.h>
#include <cmath>

#define BUFFERSIZE 512

std::unordered_map <int, struct player> players;
std::unordered_map <int, struct player> queue;
std::unordered_map <int, struct player> quit_players;
std::vector <struct player *> for_sorting;
std::vector <struct player *>::iterator it1;
std::unordered_map <int, struct player>::iterator it2;

extern fd_set recvSockSet;
extern int maxDesc;
extern fd_set tempset;
fd_set backupSet;
volatile bool inGame = false;
std::vector<std::string> word_list;
std::time_t start;
float total_time = 300.0;
struct timeval timeout = {0, 10};
size_t len = 0;

void observe();

bool descending (const player *struct1, const player *struct2){

    return (struct1->rate > struct2->rate);

}

void add_player(int sock_no, std::string name){
    struct player info;
    info.socket = sock_no;
    info.n_typed = 0;
    info.pos = 0;
    info.rate = 0;
    info.errors = 0;
    info.player_name = name;
    if (!inGame) {
        players[sock_no] = info;
        for_sorting.push_back(&players[sock_no]);
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

void *start_thread(void* fd)
{
    while(!inGame){};
//    memcpy(&tempset, &recvSockSet, sizeof(recvSockSet));
    start_game(200);
}

void start_game(int num)
{
    word_list=generate_random(num);
    game_loop();
}

void game_loop()
{
    time(&start);
    display_all();
    std::string first_word = "\nType: " + get_word(-1) + "\n\n";
    sendAll(1, first_word);
    monitor_sockets();
}

void finish_game(){
    if (get_num_players() != 0){
        display_all();
        sendAll(1, "Thank you for playing!");
        players.clear();
    }
    quit_players.clear();
    game_clear();
    inGame = false;
    pthread_exit(NULL);
}

void check(int sock_no, std::string typed){
    struct player info = players[sock_no];
    if (typed.compare(word_list[info.pos]) == 0){
        players[sock_no].pos++;
        players[sock_no].n_typed++;
        update_rate(&players[sock_no]); 
    }else{
        players[sock_no].errors++;
    }
}


void display(int sock){
    sort_ranks();
    float time_remaining = get_time_remaining();
    std::string fmt = time_remaining > 0 ? "Time Remaining = " + std::to_string(get_time_remaining()) : "Game is over!";
	fmt += "\n\nRank  Name          Speed    Errors\n\n";
    send(fmt, sock);
    fmt = "";
    int i = 1;

    for (it1 = for_sorting.begin(); it1 != for_sorting.end(); it1++){
        fmt = fmt + std::to_string(i) + ")" + "    " + (*it1)->player_name;
        int j = 14 - (*it1)->player_name.length();
        while( j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string((*it1)->rate);
        j = 9 - std::to_string((*it1)->rate).length();
        while(j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string((*it1)->errors) + "\n";
        i++;
    }
    send(fmt, sock);
}

void display_all(){
    std::unordered_map <int, struct player>::iterator temp;
    sort_ranks();
    float time_remaining = get_time_remaining();
    std::string fmt = time_remaining > 0 ? "Time Remaining = " + std::to_string(get_time_remaining()) : "Game is over!";
    fmt += "\n\nRank  Name          Speed    Errors\n\n";
    sendAll(1, fmt);
    fmt = "";
    int i = 1;
    for (it1 = for_sorting.begin(); it1 != for_sorting.end(); it1++){
        fmt = fmt + std::to_string(i) + ")" + "    " + (*it1)->player_name;
        int j = 14 - (*it1)->player_name.length();
        while( j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string((*it1)->rate);
        j = 9 - std::to_string((*it1)->rate).length();
        while(j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string((*it1)->errors) + "\n";
        i++;
    }
    sendAll(1, fmt);
}

void sort_ranks(){
    sort(for_sorting.begin(), for_sorting.end(), descending);
}

float get_time_remaining(){
    std::time_t now;
    std::time(&now);
    float elapsed = std::difftime(now, start);
    return (total_time - elapsed);
}

float time_elapsed(){
    std::time_t now;
    std::time(&now);
    float elapsed = std::difftime(now, start);
    return elapsed;
}

void update_rate(player *player){
    float mins_elapsed = time_elapsed()/60.0;
    int n_typed = player -> n_typed;
    int rate = round(n_typed/mins_elapsed);
    player -> rate = rate;
}

void observe(){
    std::unordered_map <int, struct player>::iterator temp;
    sort_ranks();
    float time_remaining = get_time_remaining();
    std::string fmt = time_remaining > 0 ? "Time Remaining = " + std::to_string(get_time_remaining()) : "Game is over!";
    fmt += "\n\nRank  Name          Speed    Errors\n\n";
    sendAll(1, fmt);
    fmt = "";
    int i = 1;
    for (it1 = for_sorting.begin(); it1 != for_sorting.end(); it1++){
        fmt = fmt + std::to_string(i) + ")" + "    " + (*it1)->player_name;
        int j = 14 - (*it1)->player_name.length();
        while( j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string((*it1)->rate);
        j = 9 - std::to_string((*it1)->rate).length();
        while(j != 0){
            fmt += " ";
            j--;
        }
        fmt += std::to_string((*it1)->errors) + "\n";
        i++;
    }
    sendAll(0, fmt);
}

void monitor_sockets(){
    char * buff = new char[BUFFERSIZE];
    std::string input;
    int size;
    int br;
    std::unordered_map <int, struct player>::iterator temp;
    struct timeval selectTime;
    memcpy(&backupSet, &recvSockSet, sizeof(recvSockSet));

    while (time_elapsed() <= total_time) {
        selectTime = timeout;
        memcpy(&tempset, &backupSet, sizeof(backupSet));
        int ready = select(maxDesc + 1, &tempset, NULL, NULL, &selectTime);

        if (ready < 0)
        {
            std::cout << "select() failed" << std::endl;
            break;
        }

        for (temp = players.begin(); temp != players.end(); temp++) {

            int sock = temp->second.socket;
            if (!FD_ISSET(sock, &tempset)) {
                continue;
            }

            memset(buff, 0, BUFFERSIZE);
            recv_length(sock, len, buff);
            input = std::string(buff);
            input.erase(std::remove(input.begin(), input.end(), '\n'),input.end());
            if (input.compare("mequitting") == 0){
                player_quitting(sock);
                break;
            }
            check(sock, input);
            display(sock);
            observe();
            input = "Type: " + get_word(sock) + "\n\n";
            send(input, sock);
        }
    }
    delete[] buff;
    finish_game();
}

void player_quitting(int socket){

    FD_CLR(socket, &recvSockSet);

    while (FD_ISSET(maxDesc, &recvSockSet) == false){
        maxDesc -= 1;
    }
    memcpy(&backupSet, &recvSockSet, sizeof(recvSockSet));
    quit_players[socket] = players[socket];
    players.erase(socket);
    if (players.size() == 0){
        finish_game();
    }
    return;
}





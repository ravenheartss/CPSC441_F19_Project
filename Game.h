//
// Created by Shankar Ganesh on 2019-10-26.
//

void add_player(int sock_no, std::string name);
void delete_player(int sock_no);
std::string get_word(int sock_no);
void clear_list();
struct player get_player(int sock_no);
int get_num_players();
void start_game(int num, fd_set readySocks);
void check(int sock_no, std::string typed);
void finish_game();
void game_loop(fd_set readySocks);
float get_time_remaining();
void monitor_sockets(fd_set readySocks);
void sort_ranks();
void display_all();
void display(int sock);
void *start_thread(void *recvset);
//std::unordered_map<int, int> get_players();
//
// Created by Shankar Ganesh on 2019-10-26.
//

void add_player(int sock_no, std::string name);
void delete_player(int sock_no);
std::string get_word(int sock_no);
void generate_words(int num);
void clear_list();
struct player get_player(int sock_no);
int get_num_players();
void start_game(int num);
void check(int sock_no, std::string typed);
//std::unordered_map<int, int> get_players();
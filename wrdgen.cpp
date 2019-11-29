#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <fstream>
#include "wrdgen.h"
#include "Server.h"
#include <vector>
std::string out_file = "temp_output.txt";
extern int lang;

std::vector <std::string> generate_random(int n_words){

    std::vector <std::string> list;
    std::string temp = std::to_string(n_words);
//    const char * args[] = {"shuf", "-n", temp, "/usr/share/dict/words", nullptr};
    std::string command;
    if (lang == 2){
        command = "shuf -n" + temp + " /usr/share/dict/nederlands >> " + out_file;
    }else{
        command = "shuf -n" + temp + " /usr/share/dict/british-english >> " + out_file;
    }

    system(command.c_str());
    int n = 0;
//    std::string words[n_words];
    std::string wrd;
    std::ifstream infile(out_file);
    while (infile >> wrd){
        list.push_back(wrd);
        n++;
    }
    infile.close();
    command = "rm -f " + out_file;
    system(command.c_str());
    return list;
}


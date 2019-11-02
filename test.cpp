#include <iostream>
#include <stdio.h>
#include <string.h>     // for memset()
#include <unordered_map>


using namespace std;

int main()
{

    unordered_map<int, string> me;
    me[10] = "LOL";
    me[13] = "LMAO";

    cout << me.size() << endl;

    return 1;

}

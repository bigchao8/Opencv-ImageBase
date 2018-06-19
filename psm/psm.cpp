#include <memory>
#include <set>
#include <cstdio>
#include <map>
#include <utility>
#include <algorithm>
#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <deque>
#include <iomanip>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv)
{
    fstream fin(argv[1]);
    string str;
    while(getline(fin,str))
    {
        if ( strncmp(str.c_str(),"Rotate:",7) == 0)
        {
            cout<< str <<endl;
        }
    }

    return 0;
}
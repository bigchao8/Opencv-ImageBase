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
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

// String splitting
void split(string s, char splitchar, vector<string> &vec)
{
    if (vec.size() > 0)
        vec.clear();
    int length = s.length();
    int start = 0;
    string topush;
    for (int i = 0; i < length; i++)
    {
        if (s[i] == splitchar && i == 0)
        {
            start += 1;
        }
        else if (s[i] == splitchar)
        {
            topush = s.substr(start, i - start);
            if (topush.length() > 0)
                vec.push_back(topush);
            start = i + 1;
        }
        else if (i == length - 1)
        {
            topush = s.substr(start, i + 1 - start);
            if (topush.length() > 0)
                vec.push_back(topush);
        }
    }
}

int main(int argc, char **argv)
{
    Mat src = imread(argv[1]);
    fstream fin(argv[2]);
    string path = argv[1];
    char cmd[1024];
    ofstream ot;

    if (!argv[1])
    {
        cout << "Error: no source image path." << endl;
        return 0;
    }

    if (!argv[2])
    {
        cout << "Error: no source box path." << endl;
        return -1;
    }

    if (!argv[3])
    {
        cout << " Error: no screenshot output path." << endl;
        return 0;
    }

    if (!argv[4])
    {
        cout << " Error: no box output path." << endl;
        return -1;
    }
    ot.open(argv[4]);

    string readline;
    vector<string> ver;
    deque<int> qestr;
    deque<deque<int>> alls;
    deque<int> ax;
    deque<int> ay;
    deque<string> word;

    while (getline(fin, readline))
    {

        split(readline, ' ', ver);
        if (ver.size() < 5)
        {
            continue;
        }
        word.push_back(ver[0]);
        ax.push_back(atoi(ver[1].c_str()));
        ay.push_back(atoi(ver[2].c_str()));

        qestr.push_back(atoi(ver[1].c_str()));
        qestr.push_back(atoi(ver[2].c_str()));
        qestr.push_back(atoi(ver[3].c_str()));
        qestr.push_back(atoi(ver[4].c_str()));

        alls.push_back(qestr);
        qestr.clear();
    }

    // Inverted container[1][2]     [1][3]
    // ******            [3][4]  -> [2][4]
    deque<int> tmp;
    deque<deque<int>> mx;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < alls.size(); j++)
        {
            tmp.push_back(alls[j][i]);
        }
        mx.push_back(tmp);
        tmp.clear();
    }

    int maxx = alls[0][0];
    int maxxdx = 0;
    for (int i = 0; i < alls.size(); i++)
    {
        if (alls[i][0] > maxx)
        {
            maxxdx = i;
            maxx = alls[i][0];
        }
    }

    int maxy = alls[0][1];
    int maxxdy = 0;
    for (int i = 0; i < alls.size(); i++)
    {
        if (alls[i][1] > maxy)
        {
            maxxdy = i;
            maxy = alls[i][1];
        }
    }

    int miny = alls[0][1];
    int minydx = 0;
    for (int i = 0; i < alls.size(); i++)
    {
        if (alls[i][1] < miny)
        {
            minydx = i;
            miny = alls[i][1];
        }
    }

    int rx = (*min_element(ax.begin(), ax.end())) - 4;
    int ry = (*min_element(ay.begin(), ay.end())) - 4;
    int rw = (*max_element(ax.begin(), ax.end())) - (*min_element(ax.begin(), ax.end())) + alls[maxxdx][2] + 8; // w = maxx - minx + w
    int rh = *max_element(ay.begin(), ay.end()) - ry + alls[maxxdy][3] + 8;                                     // h = maxy - miny
    Mat roi = src(Rect(rx, ry, rw, rh));

    imwrite(argv[3], roi);
    int tmpt;
    for (int i = 0; i < alls.size(); i++)
    {
        if (i == 0)
        {
            ot << word[i] << " " << 4
               << " " << roi.size().height - (alls[0][3] - (miny - alls[0][1]) + 4)
               << " " << alls[0][2] + 4
               << " " << alls[0][2] + (roi.size().height - (miny - alls[i][1]) - alls[i][2] - 4)
               << " " << 0 << endl;
        }
        else
        {
            if (i == 1)
            {
                tmpt = 4;
            }
            else
            {
                tmpt = alls[i - 1][0] - alls[i - 2][0] + tmpt;
            }
            if (i < alls.size())
            {
                ot << word[i] << " " << fabs(alls[i][0] - alls[i - 1][0]) + tmpt
                   << " " << roi.size().height - (alls[i][3] + (fabs(miny - alls[i][1])) + 4)
                   << " " << alls[i][0] - alls[i - 1][0] + alls[i][2] + tmpt
                   << " " << alls[i][2] + (roi.size().height - (miny - alls[i][1]) - alls[i][2] - 6)
                   << " " << 0 << endl;
            }
        }
    }

    roi.release();
    fin.close();
    ot.close();
    return 0;
}

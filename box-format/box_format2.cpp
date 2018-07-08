#include <iostream>
#include <regex>
#include <memory>
#include <set>
#include <cstdio>
#include <map>
#include <utility>
#include <algorithm>
#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>
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

void trim(string &s)
{
    int index = 0;
    if (!s.empty())
    {
        while ((index = s.find(' ', index)) != string::npos)
        {
            s.erase(index, 1);
        }
    }
}

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

int main(int argc, char const *argv[])
{

    Mat src = imread(argv[1]);
    fstream fin(argv[2]);
    fstream fin1(argv[4]); // noun rules
    fstream fin2(argv[5]); // abbreviation rules
    fstream fin3(argv[6]); // numerical rules
    fstream fin4(argv[7]); // Replace the rules

    string readline;
    vector<string> tmpsr;
    vector<string> ver;        // characters used to store character segmentation.
    deque<int> qestr;          // temporary variables
    deque<string> word;        // single character
    deque<deque<int>> alls;    // all text positions are wide and high in information.
    deque<deque<string>> cols; // store a row of data with location information.
    deque<string> anoun;
    deque<deque<string>> allstr;
    deque<string> co;
    string readnoun;
    string readrenum;
    string readacr;
    string readstr;
    deque<string> renoun;
    deque<string> renum;
    deque<string> reacr;
    deque<string> restr;
    deque<string> resstr;
    deque<string> costr;

    double rew = src.size().width * 0.2; // Overlong rectangle
    double reh = 120;                    // High rectangle

    if (!argv[1])
    {
        cout << "Error: incorrect image input path." << endl;
        return 0;
    }

    if (!argv[2])
    {
        cout << "Error: error box file path." << endl;
        return -1;
    }
    if (!argv[3])
    {
        cout << "Error: error thesaurus file path." << endl;
        return -1;
    }
    if (!argv[4])
    {
        cout << "There is no noun rule file" << endl;
        return -1;
    }
    if (!argv[5])
    {
        cout << "There is no acronym file" << endl;
        return -1;
    }
    if (!argv[6])
    {
        cout << "There is no numerical rule file" << endl;
        return -1;
    }
    if (!argv[7])
    {
        cout << "There is no replaceable thesaurus file" << endl;
        return -1;
    }

    // Get four columns of data.
    while (getline(fin, readline))
    {
        split(readline, ' ', ver);
        if (ver.size() < 5)
        {
            continue;
        }

        if ((atoi(ver[3].c_str()) - atoi(ver[1].c_str())) < rew &&
            (atoi(ver[4].c_str()) - atoi(ver[2].c_str())) < reh)
        {

            // Storage source data
            word.push_back(ver[0]); // Store single text
            // A character that evaluates to 0.
            if (atoi(ver[2].c_str()) == 0)
            {
                if (alls.size() == 0)
                {
                    continue;
                }
                qestr = alls.back();
                qestr[0] = qestr[0] + (qestr[2] - qestr[0]);
            }
            else
            {
                qestr.push_back(atoi(ver[1].c_str()));
                qestr.push_back(atoi(ver[2].c_str()));
                qestr.push_back(atoi(ver[3].c_str()));
                qestr.push_back(atoi(ver[4].c_str()));
            }

            // Store a row of data
            alls.push_back(qestr); // Analog two-dimensional array
            qestr.clear();
        }
        // The data that records 0 will be reintroduced in the future.
    }

    // Calculate the average character width.
    double wsum = .0;
    for (int i = 0; i < alls.size(); i++)
    {
        wsum += alls[i][2] - alls[i][0];
    }
    double wd = wsum / alls.size();

    // he magnitude of the x value before and after the calculation will be positive and negative.
    deque<double> xd;
    for (int i = 0; i < alls.size() - 1; i++)
    {
        xd.push_back((alls[i][0] - alls[i + 1][0]));
    }

    // The main basis of the data is the positive and negative value of the
    // difference between upstream and downstream and the average width of the text.
    deque<string> single;
    int als = alls.size();
    for (int i = 0; i < als; i++)
    {
        // A line of characters with location.带位置
        co.push_back(word[i]);
        co.push_back(to_string(alls[0][0]));
        co.push_back(to_string(alls[0][1]));
        co.push_back(to_string(alls[0][2]));
        co.push_back(to_string(alls[0][3]));
        alls.pop_front();
        // Judge a newline
        double x = xd[i];
        if (x > 0 && x > wd)
        {
            cols.push_back(co);
            co.clear();
        }
        if (i == als - 1)
        {
            cols.push_back(co);
        }
    }

    deque<string> strcol; // Determine the text after an average split of a line.
    string s;             // One-line string
    // Determine the space column.
    for (int i = 0; i < cols.size(); i++)
    {
        double wd = .0;
        double sumwd = .0;
        if (cols[i].size() != 0)
        {

            int index = 0;
            for (int j = 0; j < cols[i].size() - 5; j += 5)
            {
                wd = fabs(atoi(cols[i][j + 6].c_str()) - atoi(cols[i][j + 3].c_str()));
                if (wd == 0)
                {
                    index++;
                }
                sumwd += wd;
            }
            double d = sumwd / (cols[i].size() / 5 - 1 - index);

            for (int j = 0; j < cols[i].size() - 5; j += 5)
            {
                s.append(cols[i][j]); // Append character array
                wd = fabs(atoi(cols[i][j + 6].c_str()) - atoi(cols[i][j + 3].c_str()));
                if (wd > d * 2)
                {
                    s.append(" ");
                }
            }
            s.append(cols[i][cols[i].size() - 5]);
            strcol.push_back(s);
            s.clear();
        }
    }

    // Merge single character
    for (int i = 0; i < strcol.size(); i++)
    {
        // Merge single characters to the left.
        for (auto st = strcol[i].begin(); st != strcol[i].end();)
        {
            if (*st == ' ' && *(st + 1) == ' ') // Continuous Spaces
            {
                st = strcol[i].erase(st);
            }
            if (*st == ' ' && *(st + 2) == ' ') // A single character
            {
                st = strcol[i].erase(st);
            }
            if (*st == ' ' && *(st + 1) == ' ') // Continuous Spaces
            {
                st = strcol[i].erase(st);
            }
            else
            {
                *st++;
            }
        }
    }

    // By keyword.
    fstream f(argv[3]);
    string phrase;
    vector<int> chpos;
    vector<int> chi;
    vector<int> maxph;

    int colsize = strcol.size();
    while (getline(f, phrase))
    {
        if (phrase.size() == 0)
        {
            continue;
        }
        // CRLF -> LF
        for (auto t = phrase.begin(); t != phrase.end();)
        {
            if (*t == '\r')
            {
                t = phrase.erase(t);
            }
            else
            {
                t++;
            }
        }
        // Go through the lines and match the keywords.
        for (int i = 0; i < colsize; i++)
        {
            int position = 0;
            bool noundex = false;
            while ((position = strcol[i].find(phrase, position)) != string::npos)
            {
                for (int j = 0; j < chpos.size(); j++)
                {
                    if (position >= chpos[j] && i == chi[j] && maxph[j] > position)
                    {
                        noundex = true;
                        break;
                    }
                }
                if (noundex == true)
                {
                    position +=2;
                    continue;
                }
                if (position > 0)
                {
                    auto sts = strcol[i].begin();
                    if (*(sts + position + phrase.size()) != ' ') // If there's a space at the end
                    {
                        if (position + phrase.size() != strcol[i].size()) // No Spaces at the end.
                        {
                            strcol[i].insert(position + phrase.size(), " "); // End of keyword
                        }
                    }
                    if (*(sts + position) != ' ')
                    {
                        strcol[i].insert(position, " "); // Key pref.
                        int srdex = strcol[i].find(phrase);
                        chpos.push_back(srdex);
                        maxph.push_back(srdex + phrase.size());
                        chi.push_back(i);
                    }
                }
                else if (position == 0)
                {
                    auto sts = strcol[i].begin();
                    if (*(sts + position + phrase.size()) != ' ')        // If there's a space at the end
                        strcol[i].insert(position + phrase.size(), " "); // End of keyword
                    int srdex = strcol[i].find(phrase);
                    chpos.push_back(srdex);
                    maxph.push_back(srdex + phrase.size());
                    chi.push_back(i);
                }
                position += 2;
            }
        }
    }

    // Whitespace removal
    for (int i = 0; i < strcol.size(); i++)
    {
        for (auto st = strcol[i].begin(); st != strcol[i].end();)
        {
            if (*st == ' ' && *(st + 1) == ' ') // Continuous Spaces
            {
                st = strcol[i].erase(st);
            }
            else
            {
                *st++;
            }
        }
    }

    // regular match
    // noun match
    while (getline(fin1, readnoun))
    {
        // CRLF -> LF
        for (auto t = readnoun.begin(); t != readnoun.end();)
        {
            if (*t == '\r')
            {
                t = readnoun.erase(t);
            }
            else
            {
                t++;
            }
        }
        renoun.push_back(readnoun);
    }
    // noun abbreviation
    while (getline(fin2, readacr))
    {
        // CRLF -> LF
        for (auto t = readacr.begin(); t != readacr.end();)
        {
            if (*t == '\r')
            {
                t = readacr.erase(t);
            }
            else
            {
                t++;
            }
        }
        reacr.push_back(readacr);
    }
    // numerical match
    while (getline(fin3, readrenum))
    {
        // CRLF -> LF
        for (auto t = readrenum.begin(); t != readrenum.end();)
        {
            if (*t == '\r')
            {
                t = readrenum.erase(t);
            }
            else
            {
                t++;
            }
        }
        readrenum = readrenum + '$';
        readrenum.insert(readrenum.begin(), '^');
        renum.push_back(readrenum);
        readrenum.clear();
    }
    int dexline = 0;
    // noun to replace
    while (getline(fin4, readstr))
    {
        dexline++;
        if (readstr.size() == 0)
        {
            continue;
        }
        // CRLF -> LF
        for (auto t = readstr.begin(); t != readstr.end();)
        {
            if (*t == '\r')
            {
                t = readstr.erase(t);
            }
            else
            {
                t++;
            }
        }
        split(readstr, '\t', tmpsr);
        if (tmpsr.size() < 2)
        {
            cout << "Incorrect substitution rules error lines:"
                 << " " << dexline << endl;
            continue;
        }
        restr.push_back(tmpsr[0]);
        resstr.push_back(tmpsr[1]);
        tmpsr.clear();
    }

    for (int i = 0; i < strcol.size(); i++)
    {
        // Merge single characters to the left.
        costr.push_back(to_string(i + 1));
        costr.push_back("\t");
        auto sstrat = strcol[i].begin();
        auto tmep = sstrat;
        for (auto st = strcol[i].begin(); st != strcol[i].end(); st++)
        {
            if (*st == ' ')
            {
                string tmps;
                tmps.append(sstrat, st);
                trim(tmps);
                sstrat = st;
                costr.push_back(tmps);
                int idx = 0;
                // Noun match
                for (int i = 0; i < renoun.size(); i++)
                {
                    trim(renoun[i]);
                    if (tmps == renoun[i])
                    {
                        costr.push_back("\t");
                        costr.push_back("GPN");
                        break;
                    }
                }
                // noun abbreviation
                for (int i = 0; i < reacr.size(); i++)
                {
                    if (tmps == reacr[i])
                    {
                        costr.push_back("\t");
                        costr.push_back("APN");
                        break;
                    }
                }
                // noun to replace
                for (int i = 0; i < renum.size(); i++)
                {
                    // numerical match
                    std::regex regex_numerical(renum[i]);
                    if (regex_match(tmps, regex_numerical))
                    {
                        costr.push_back("\t");
                        costr.push_back("VAL");
                        break;
                    }
                }
                costr.push_back("\n");
                costr.push_back(to_string(i + 1));
                costr.push_back("\t");
                tmps.clear();
            }
            tmep = st;
        }
        // no Spaces at the end
        auto tail = strcol[i].end();
        if (*tail != ' ')
        {
            string tmps;
            tmps.append(sstrat, strcol[i].end());
            trim(tmps);
            costr.push_back(tmps);
            int idx = 0;
            for (int i = 0; i < renoun.size(); i++)
            {
                trim(renoun[i]);
                if (tmps == renoun[i])
                {
                    costr.push_back("\t");
                    costr.push_back("GPN");
                    break;
                }
            }
            for (int i = 0; i < reacr.size(); i++)
            {
                if (tmps == reacr[i])
                {
                    costr.push_back("\t");
                    costr.push_back("APN");
                    break;
                }
            }
            for (int i = 0; i < renum.size(); i++)
            {
                std::regex regex_numerical(renum[i]);
                if (regex_match(tmps, regex_numerical))
                {
                    costr.push_back("\t");
                    costr.push_back("VAL");
                    break;
                }
            }
            tmps.clear();
        }
        costr.push_back("\n");
    }
    string s2 = "APN";
    string s1 = "GPN";
    string ttm;
    for (int i = 0; i < costr.size(); i++)
    {
        // replace
        for (int j = 0; j < restr.size(); j++)
        {
            if (costr[i] == restr[j] && costr[i + 2] == s1)
            {
                ttm = costr[i];
                costr[i] = resstr[j];
                costr[i + 2].append("\t");
                costr[i + 2].append(ttm.c_str());
                break;
            }
            if (costr[i] == restr[j] && costr[i + 2] == s2)
            {
                ttm = costr[i];
                costr[i] = resstr[j];
                costr[i + 2].append("\t");
                costr[i + 2].append(ttm.c_str());
                break;
            }
        }
        cout << costr[i];
    }
    fin.close();
    fin1.close();
    fin2.close();
    f.close();

    return 0;
}
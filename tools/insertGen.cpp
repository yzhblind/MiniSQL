#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>

using namespace std;

const int record_num = 1000;
const int attr_num = 4;
const vector<int> type = {9, 17, 0, 2};
const vector<bool> str_int = {true, false, false, false};
const vector<bool> unique = {true, true, false, false};
const string table_name("student");
const string form("insert into @ values ( @ );");
vector<set<string>> check_s(attr_num);
vector<set<int>> check_i(attr_num);
vector<set<float>> check_f(attr_num);

string random_string_gen(int max_len, bool is_int)
{
    string str("");
    int min_len = max_len / 2;
    int len = min_len + (rand() % (max_len - min_len));
    for (int i = 0; i < len; ++i)
        str += is_int ? '0' + rand() % 10 : 'a' + rand() % 26;
    return '\'' + str + '\'';
}
int random_int_gen(int max_int) { return rand() % max_int; }
string record_gen()
{
    string res = form;
    res.replace(res.find("@"), 1, table_name);
    string record("");
    for (int i = 0; i < attr_num; ++i)
    {
        switch (type[i])
        {
        case 0:
            int t_int;
            t_int = 10 + random_int_gen(50);
            record += to_string(t_int) + ',';
            break;
        case 1:
            break;
        default:
            string t_str("");
            if (unique[i] == true)
            {
                do
                {
                    t_str = random_string_gen(type[i], str_int[i]);
                } while (check_s[i].find(t_str) != check_s[i].end());
                check_s[i].insert(t_str);
            }
            else
            {
                t_str = random_string_gen(type[i], str_int[i]);
            }
            record += t_str + ',';
            break;
        }
    }
    record.pop_back();
    res.replace(res.find("@"), 1, record);
    return res;
}

int main()
{
    srand(0);
    ofstream out("out.txt");
    for (int i = 0; i < record_num; ++i)
        out << record_gen() << endl;
    return 0;
}
#include "BufferManager.hpp"
#include <iostream>
using namespace std;

int main()
{
    int fileAddr = bufMgr.openFile("testRecord.db", DATA, 6);
    for (int i = 5; i < 9; ++i)
    {
        string str = std::to_string(i + i * 10 + i * 100 + i * 1000);
        node dest = bufMgr.getNextFree(fileAddr);
        dest.write(str.c_str(), 0, str.size() + 1);
    }
    bufMgr.deleteRecord(fileAddr, 3, 14);
    bufMgr.deleteRecord(fileAddr, 4, 21);
    node src = bufMgr.getRecord(fileAddr, 6, 7);
    char s[256];
    src.read(s, 0, 6);
    cout << string(s) << endl;
    return 0;
}
#include "BufferManager.hpp"
#include <iostream>
using namespace std;

int main()
{
    int fileAddr = bufMgr.openFile("testRecord.db", DATA, 25);
    for (int i = 0; i < 64; ++i)
    {
        string str = "123456789!";
        node dest = bufMgr.getNextFree(fileAddr);
        dest.write(str.c_str(), 0, str.size() + 1);
    }
    // bufMgr.deleteRecord(fileAddr, 3, 14);
    // bufMgr.deleteRecord(fileAddr, 4, 21);
    // node src = bufMgr.getRecord(fileAddr, 6, 7);
    // char s[256];
    // src.read(s, 0, 6);
    // cout << string(s) << endl;
    return 0;
}
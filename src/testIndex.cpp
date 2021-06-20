#include "Type.hpp"
#include "BufferManager.hpp"
#include "RecordManager.hpp"
#include "IndexManager.hpp"
#include "iostream"

using namespace std;

void testCase1()
{
    remove("test1.db");
    remove("index.db");
    vector<attribute> col = {{"col_a", 10, 0, true}};
    hword dataAddr = bufMgr.openFile("test1.db", DATA, 10);
    hword indexAddr = bufMgr.openFile("index.db", INDEX);
    char s[10];
    int num;
    cin>>num;
    for (int i = 0; i < num; ++i)
    {
        sprintf(s, "%09d", i);
        s[9] = '\0';
        rcdMgr.insertRecord(dataAddr, s, col);
    }
    idxMgr.setIndexFileAddr(indexAddr);
    idxMgr.createEntry(dataAddr, col, 0);
    cout << col[0].indexRootAddr << endl;
}

int main()
{
    testCase1();
    return 0;
}
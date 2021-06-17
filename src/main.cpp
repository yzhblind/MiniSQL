#include "Interpreter.hpp"
#include "RecordManager.hpp"
using namespace std;

int main()
{
    int fileAddr = bufMgr.openFile("test.db", DATA, 2);
    vector<attribute> column;
    column.resize(1);
    column[0].type = 2;
    char str[2];
    str[2] = '\0';
    for (int i = 'A'; i <= 'Z'; ++i)
    {
        str[0] = i;
        rcdMgr.insertRecord(fileAddr, str);
    }
    filter flt(column);
    char a[2] = "F";
    char b[2] = "X";
    flt.addCond(condExpr(column, 0, GE, a));
    flt.addCond(condExpr(column, 0, L, b));
    rcdMgr.deleteRecord(fileAddr, flt);
    //rcdMgr.selectRecord(fileAddr, flt);
    //for (int i = 0; i < flt.res.size(); ++i)
    //    puts(static_cast<char *>(flt.res[i]));
    return 0;
}
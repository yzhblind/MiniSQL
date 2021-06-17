#pragma once

#include "Type.hpp"
#include "BufferManager.hpp"
#include <vector>
#include <iostream>

enum op
{
    E,  // =
    NE, // <>
    L,  // <
    G,  // >
    LE, // <=
    GE  // >=
};
struct condExpr
{
    const std::vector<attribute> &origin;
    const int pos;
    const op opr;
    const void *val;
    static const void *copyVal(const std::vector<attribute> &origin, const int pos, const void *val);
    condExpr(const std::vector<attribute> &origin, const int pos, const op opr, const void *val) : origin(origin), pos(pos), opr(opr), val(copyVal(origin, pos, val)) {}
    ~condExpr() { delete[] static_cast<const char *>(val); }
    condExpr(const condExpr &obj) : origin(obj.origin), pos(obj.pos), opr(obj.opr), val(copyVal(obj.origin, obj.pos, obj.val)) {}
};
class filter
{
private:
    const std::vector<attribute> &origin;
    std::vector<int> offset;
    std::vector<condExpr> cond;
    int keyPos;
    bool check(const condExpr &c, void *record);

public:
    std::vector<void *> res;
    std::vector<dword> resAddr;
    filter(const std::vector<attribute> &origin);
    ~filter();
    int addCond(const condExpr &c);
    int push(void *record, dword vAddr, bool delFlag = false);
    inline void setKeyPos(int pos) { keyPos = pos; }
};

class RecordManager
{
private:
    bool uniqueCheck(hword fileAddr, void *data, const std::vector<attribute> &origin);

public:
    //插入失败返回0，否则返回插入位置（不包括valid字节）的首地址
    dword insertRecord(hword fileAddr, void *data, const std::vector<attribute> &origin);
    //flt中将会有已删除记录的地址与删除的信息（TODO：可指定保留信息的部分）
    int deleteRecord(hword fileAddr, filter &flt);
    int selectRecord(hword fileAddr, filter &flt);
};

extern RecordManager rcdMgr;
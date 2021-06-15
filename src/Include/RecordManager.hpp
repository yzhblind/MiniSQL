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
    std::vector<void *> res;
    bool check(const condExpr &c, void *record);

public:
    filter(const std::vector<attribute> &origin);
    ~filter();
    int addCond(const condExpr &c);
    int push(void *record);
    int output(std::ostream &out, int alignment) const;
};

class RecordManager
{
private:
    /* data */
public:
    RecordManager(/* args */);
    ~RecordManager();
};

extern RecordManager rcdMgr;
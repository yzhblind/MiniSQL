#include "RecordManager.hpp"
#include <cstring>

static inline hword extractFileAddr(dword virtAddr) { return virtAddr >> 48; }
static inline hword extractOffset(dword virtAddr) { return virtAddr & 0xFFFF; }
static inline word extractBlockAddr(dword virtAddr) { return (virtAddr >> 16) & 0xFFFFFFFF; }

const void *condExpr::copyVal(const std::vector<attribute> &origin, const int pos, const void *val)
{
    void *res = origin[pos].type <= 1 ? new char[4] : new char[origin[pos].type];
    return memcpy(res, val, origin[pos].type <= 1 ? 4 : origin[pos].type);
}

filter::filter(const std::vector<attribute> &origin) : origin(origin)
{
    offset.push_back(0);
    for (int i = 1; i <= origin.size(); ++i)
        offset.push_back(offset[i - 1] + (origin[i].type <= 1 ? 4 : origin[i].type));
}
filter::~filter()
{
    for (int i = 0; i < res.size(); ++i)
        delete[] static_cast<char *>(res[i]);
}
int filter::addCond(const condExpr &c)
{
    if (&c.origin == &origin)
    {
        cond.push_back(c);
        return SUCCESS;
    }
    return FAILURE;
}
template <class T>
bool cmp(const op opr, T a, T b)
{
    switch (opr)
    {
    case E:
        return a == b;
        break;
    case NE:
        return a != b;
        break;
    case L:
        return a < b;
        break;
    case G:
        return a > b;
        break;
    case LE:
        return a <= b;
        break;
    case GE:
        return a >= b;
        break;
    default:
        return false;
        break;
    }
}
bool filter::check(const condExpr &c, void *record)
{
    switch (origin[c.pos].type)
    {
    case 0:
    {
        const int *a = reinterpret_cast<const int *>(static_cast<char *>(record) + offset[c.pos]);
        const int *b = reinterpret_cast<const int *>(c.val);
        return cmp(c.opr, *a, *b);
    }
    break;

    case 1:
    {
        const float *a = reinterpret_cast<const float *>(static_cast<char *>(record) + offset[c.pos]);
        const float *b = reinterpret_cast<const float *>(c.val);
        return cmp(c.opr, *a, *b);
    }
    break;

    default:
    {
        const char *a = static_cast<char *>(record) + offset[c.pos];
        const char *b = reinterpret_cast<const char *>(c.val);
        int t = strcmp(a, b);
        return cmp(c.opr, t, 0);
    }
    break;
    }
}
int filter::push(void *record, dword vAddr, bool delFlag)
{
    for (int i = 0; i < cond.size(); ++i)
        if (check(cond[i], record) == false)
            return FAILURE;
    if (!delFlag)
    {
        char *p = new char[*offset.rbegin()];
        memcpy(p, record, *offset.rbegin());
        res.push_back(p);
    }
    if (delFlag)
        resAddr.push_back(vAddr);
    return SUCCESS;
}

int RecordManager::insertRecord(hword fileAddr, void *data)
{
    node wrt = bufMgr.getNextFree(fileAddr);
    return wrt.write(data, 0, wrt.getSize());
}
int RecordManager::deleteRecord(hword fileAddr, filter &flt)
{
    int blockNum = bufMgr.getBlockNumber(fileAddr);
    int rSize = bufMgr.getRecordSize(fileAddr);
    if (blockNum == -1 || rSize == -1)
        return FAILURE;
    char *record = new char[rSize + 1];
    for (int i = 1; i < blockNum; ++i)
    {
        node t = bufMgr.getBlock(fileAddr, i);
        int tSize = t.getSize();
        for (int offset = 0; offset < tSize - rSize; offset += rSize + 1)
        {
            t.read(record, offset, rSize + 1);
            if (*record == true)
                flt.push(record + 1, t.getVirtAddr() + offset);
        }
    }
    for (int i = 0; i < flt.resAddr.size(); ++i)
        bufMgr.deleteRecord(extractFileAddr(flt.resAddr[i]), extractBlockAddr(flt.resAddr[i]), extractOffset(flt.resAddr[i]));
    return SUCCESS;
}
int RecordManager::selectRecord(hword fileAddr, filter &flt)
{
    int blockNum = bufMgr.getBlockNumber(fileAddr);
    int rSize = bufMgr.getRecordSize(fileAddr);
    if (blockNum == -1 || rSize == -1)
        return FAILURE;
    char *record = new char[rSize + 1];
    for (int i = 1; i < blockNum; ++i)
    {
        node t = bufMgr.getBlock(fileAddr, i);
        int tSize = t.getSize();
        for (int offset = 0; offset < tSize - rSize; offset += rSize + 1)
        {
            t.read(record, offset, rSize + 1);
            if (*record == true)
                flt.push(record + 1, t.getVirtAddr() + offset);
        }
    }
    return SUCCESS;
}

RecordManager rcdMgr;
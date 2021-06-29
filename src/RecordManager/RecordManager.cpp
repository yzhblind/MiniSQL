#include "RecordManager.hpp"
#include "Type.hpp"
#include <cstring>

const void *condExpr::copyVal(const std::vector<attribute> &origin, const int pos, const void *val)
{
    void *res = new char[type2size(origin[pos].type)];
    return memcpy(res, val, type2size(origin[pos].type));
}

filter::filter(const std::vector<attribute> &origin) : origin(origin)
{
    // keyPos = -1;
    offset.push_back(0);
    for (int i = 1; i <= origin.size(); ++i)
        offset.push_back(offset[i - 1] + type2size(origin[i - 1].type));
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
    char *p = new char[getSize()];
    memcpy(p, record, getSize());
    res.push_back(p);
    if (delFlag == true)
        resAddr.push_back(vAddr);
    // if (keyPos >= 0 && keyPos < origin.size())
    // {
    //     char *p = new char[type2size(origin[keyPos].type)];
    //     memcpy(p, static_cast<char *>(record) + offset[keyPos], type2size(origin[keyPos].type));
    //     res.push_back(p);
    // }
    return SUCCESS;
}
int filter::push(const node &in)
{
    char *p = new char[getSize()];
    if (in.read(p, 0, getSize()) == FAILURE)
        return FAILURE;
    for (int i = 0; i < cond.size(); ++i)
        if (check(cond[i], p) == false)
            return FAILURE;
    res.push_back(p);
    return SUCCESS;
}
bool RecordManager::uniqueCheck(hword fileAddr, void *data, const std::vector<attribute> &origin)
{
    std::vector<int> index2check;
    index2check.clear();
    for (int i = 0; i < origin.size(); ++i)
        if (origin[i].isUnique == true && origin[i].indexRootAddr == 0)
            index2check.push_back(i);
    if (index2check.empty())
        return true;
    std::vector<int> offset;
    offset.clear();
    offset.push_back(0);
    for (int i = 1; i < origin.size(); ++i)
        offset.push_back(offset[i - 1] + type2size(origin[i - 1].type));
    int blockNum = bufMgr.getBlockNumber(fileAddr);
    int rSize = bufMgr.getRecordSize(fileAddr);
    char *record = new char[rSize + 1];
    for (int i = 1; i < blockNum; ++i)
    {
        node t = bufMgr.getBlock(fileAddr, i);
        int tSize = t.getSize();
        for (int i = 0; i < tSize - rSize; i += rSize + 1)
        {
            t.read(record, i, rSize + 1);
            if (*record == true)
                for (int j = 0; j < index2check.size(); ++j)
                    if (strncmp(static_cast<char *>(data) + offset[index2check[j]], record + 1 + offset[index2check[j]], type2size(origin[index2check[j]].type)) == 0)
                    {
                        delete[] record;
                        return false;
                    }
        }
    }
    delete[] record;
    return true;
}
dword RecordManager::insertRecord(hword fileAddr, void *data, const std::vector<attribute> &origin)
{
    int blockAddr = bufMgr.getNextFreeAddr(fileAddr);
    if (blockAddr > 0)
        bufMgr.pinBlock(fileAddr, blockAddr, PIN);
    //int ret = FAILURE;
    dword ret = 0;
    if (uniqueCheck(fileAddr, data, origin))
    {
        node wrt = bufMgr.getNextFree(fileAddr);
        wrt.write(data, 0, wrt.getSize());
        ret = wrt.getVirtAddr();
    }
    if (blockAddr > 0)
        bufMgr.pinBlock(fileAddr, blockAddr, UNPIN);
    return ret;
}
int RecordManager::deleteRecord(hword fileAddr, filter &flt)
{
    int blockNum = bufMgr.getBlockNumber(fileAddr);
    int rSize = bufMgr.getRecordSize(fileAddr);
    if (blockNum == FAILURE || rSize == FAILURE)
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
                flt.push(record + 1, t.getVirtAddr() + offset, true);
        }
    }
    delete[] record;
    for (int i = 0; i < flt.resAddr.size(); ++i)
        bufMgr.deleteRecord(extractFileAddr(flt.resAddr[i]), extractBlockAddr(flt.resAddr[i]), extractOffset(flt.resAddr[i]));
    return SUCCESS;
}
int RecordManager::deleteRecord(dword virtAddr) { return bufMgr.deleteRecord(extractFileAddr(virtAddr), extractBlockAddr(virtAddr), extractOffset(virtAddr)); }
int RecordManager::selectRecord(hword fileAddr, filter &flt)
{
    int blockNum = bufMgr.getBlockNumber(fileAddr);
    int rSize = bufMgr.getRecordSize(fileAddr);
    if (blockNum == FAILURE || rSize == FAILURE)
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
                flt.push(record + 1, 0); //选择时不关心virtAddr
        }
    }
    return SUCCESS;
}
int RecordManager::getRecord(dword virtAddr, filter &flt)
{
    node t = bufMgr.getRecord(extractFileAddr(virtAddr), extractBlockAddr(virtAddr), extractOffset(virtAddr));
    return flt.push(t);
}

RecordManager rcdMgr;
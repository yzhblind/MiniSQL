#include "RecordManager.hpp"

const void *condExpr::copyVal(const std::vector<attribute> &origin, const int pos, const void *val)
{
    void *res = origin[pos].type <= 1 ? new char[4] : new char[origin[pos].type];
    return memcpy(res, val, origin[pos].type <= 1 ? 4 : origin[pos].type);
}

RecordManager::RecordManager(/* args */)
{
}

RecordManager::~RecordManager()
{
}

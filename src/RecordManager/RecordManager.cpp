#include "RecordManager.hpp"
#include <cstring>

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
int filter::push(void *record)
{
    for (int i = 0; i < cond.size(); ++i)
        if (check(cond[i], record) == false)
        {
            static_cast<const char *>(record);
            return FAILURE;
        }
    res.push_back(record);
    return SUCCESS;
}

RecordManager::RecordManager(/* args */)
{
}

RecordManager::~RecordManager()
{
}

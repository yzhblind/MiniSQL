#pragma once

#include "Type.hpp"
#include "BufferManager.hpp"
#include <vector>

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
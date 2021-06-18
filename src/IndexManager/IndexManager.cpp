#include "IndexManager.hpp"
#include "Type.hpp"
#include <cstring>

bnode::bnode(const node &src) : node(src)
{
    origin.pinBlock(getFileAddr(), getBlockAddr(), PIN);
    base = reinterpret_cast<char *>(phyAddr);
    parent = reinterpret_cast<word *>(base + 2);
    cnt = reinterpret_cast<int *>(base + 6);
}
bnode::~bnode() { origin.pinBlock(getFileAddr(), getBlockAddr(), UNPIN); }
IndexManager::IndexManager()
{
    for (int i = 0; i <= 256; ++i)
    {
        int size = type2size(i) + 6;
        bcnt[i] = (BufferManager::pageSize - 2 - 4 - 4 - 6) / size + 1;
    }
}
int bnode::binarySearch(const element &e)
{
    if (e < element(e.type, base + index2offset(*cnt - 1, e.type) + 6))
    {
        int l = 0, r = *cnt - 1;
        int mid;
        while (l < r)
        {
            mid = (l + r) >> 1;
            if (e <= element(e.type, base + index2offset(mid, e.type) + 6))
                r = mid;
            else
                l = mid + 1;
        }
        return r;
    }
    else
        return *cnt;
}
void *bnode::move(int start, int dir, int type)
{
    int dest = index2offset(start + dir, type);
    int src = index2offset(start, type);
    int end = index2offset(*cnt, type) + 6;
    memmove(base + dest, base + src, end - src);
    *cnt += dir;
    return base + src;
}
int bnode::insertKey(const element &key, const dword virtAddr)
{
    if (*cnt == idxMgr.bcnt[key.type] - 1)
        return FAILURE;
    int idx = binarySearch(key);
    void *p = move(idx, 1, key.type);
    *static_cast<word *>(p) = extractBlockAddr(virtAddr);
    *static_cast<hword *>(p + 4) = extractOffset(virtAddr);
    element(key.type, p + 6).cpy(key);
    return idx;
}
int bnode::deleteKey(const element &key)
{
    int idx = binarySearch(key);
    if (idx != *cnt)
    {
        int offset = index2offset(key.type, idx) + 6;
        if (key == element(key.type, base + offset))
        {
            move(idx + 1, -1, key.type);
            return idx;
        }
    }
    return FAILURE;
}
int bnode::replaceKey(const element &key, const dword virtAddr)
{
    
}
bnode bnode::split(const element &key, const dword virtAddr)
{

}
element bnode::splice(bnode &src)
{

}
element bnode::merge(bnode &src)
{

}

dword IndexManager::findAddrEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue)
{
    //TODO
}
node IndexManager::findRecordEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue)
{
    dword virtAddr = findAddrEntry(dataFileAddr, rootAddr, keyValue);
    return bufMgr.getRecord(dataFileAddr, extractBlockAddr(virtAddr), extractOffset(virtAddr));
}
int IndexManager::insertEntry(const hword dataFileAddr, attribute &attr, const element &keyValue, dword virtAddr)
{
    //TODO
}
int IndexManager::deleteEntry(const hword dataFileAddr, std::vector<attribute> &col, const filter &flt)
{
    //TODO
}
int IndexManager::createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos)
{
    //TODO
}
int IndexManager::dropEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos)
{
    //TODO
}

IndexManager idxMgr;
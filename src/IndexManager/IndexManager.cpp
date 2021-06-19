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
        bInMincnt[i] = (bcnt[i] + 1) / 2 - 1;
        bLeafMincnt[i] = bcnt[i] / 2;
    }
    bnode::tmpMemory = new char[BufferManager::pageSize + 256];
}
IndexManager::~IndexManager() { delete[] static_cast<char *>(bnode::tmpMemory); }
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
void *bnode::move(int start, int dir, int type, const packing packingType)
{
    int dest = index2offset(start + dir, type) + (packingType == PTR_DATA ? 0 : 6);
    int src = index2offset(start, type) + (packingType == PTR_DATA ? 0 : 6);
    int end = index2offset(*cnt, type) + 6;
    memmove(base + dest, base + src, end - src);
    *cnt += dir;
    return base + src;
}
// void *bnode::rmove(int start, int dir, int type)
// {
//     int dest = index2offset(start + dir, type) + 6;
//     int src = index2offset(start, type) + 6;
//     int end = index2offset(*cnt, type) + 6;
//     memmove(base + dest, base + src, end - src);
//     *cnt += dir;
//     return base + src;
// }
dword bnode::find(const element &key, const packing packingType)
{
    int idx = binarySearch(key);
    if (idx != *cnt)
    {
        int offset = index2offset(key.type, idx);
        if (key == element(key.type, base + offset + 6))
        {
            word blockAddr = *reinterpret_cast<word *>(base + offset + (packingType == PTR_DATA ? 0 : 6 + type2size(key.type)));
            hword offsetAddr = *reinterpret_cast<hword *>(base + offset + 4 + (packingType == PTR_DATA ? 0 : 6 + type2size(key.type)));
            return combileVirtAddr(getFileAddr(), blockAddr, offsetAddr);
        }
    }
    return 0;
}
int bnode::insertKey(const element &key, const dword virtAddr, const packing packingType)
{
    //if (*cnt == idxMgr.bcnt[key.type] - 1)
    //    return FAILURE;
    int idx = (*cnt == 0) ? 0 : binarySearch(key);
    origin.setDirty(getFileAddr(), getBlockAddr());
    char *p = static_cast<char *>(move(idx, 1, key.type, packingType));
    *reinterpret_cast<word *>(p + (packingType == PTR_DATA ? 0 : type2size(key.type))) = extractBlockAddr(virtAddr);
    *reinterpret_cast<hword *>(p + (packingType == PTR_DATA ? 0 : type2size(key.type)) + 4) = extractOffset(virtAddr);
    element(key.type, p + (packingType == PTR_DATA ? 6 : 0)).cpy(key);
    return idx;
}
int bnode::deleteKey(const element &key, const packing packingType)
{
    int idx = binarySearch(key);
    if (idx != *cnt)
    {
        int offset = index2offset(key.type, idx) + 6;
        // if (key == element(key.type, base + offset))
        // {
        origin.setDirty(getFileAddr(), getBlockAddr());
        move(idx + 1, -1, key.type, packingType);
        return idx;
        // }
    }
    return FAILURE;
}
int bnode::replaceKey(const element &key, const element &newKey)
{
    int idx = binarySearch(key);
    if (idx != *cnt)
    {
        int offset = index2offset(key.type, idx) + 6;
        element &&t = element(key.type, base + offset);
        if (key == t)
        {
            origin.setDirty(getFileAddr(), getBlockAddr());
            t.cpy(newKey);
            return idx;
        }
    }
    return FAILURE;
}
bnode bnode::split(const element &key, const dword virtAddr)
{
    memcpy(tmpMemory, phyAddr, BufferManager::pageSize);
    void *phyAddrSave = phyAddr;
    phyAddr = tmpMemory;
    insertKey(key, virtAddr, *base == 0 ? PTR_DATA : DATA_PTR);
    int lcnt = (*cnt + 1) / 2;
    int rcnt = *cnt - lcnt;
    bnode t(origin.getNextFree(getFileAddr()));
    *t.base = *base, *(t.base + 1) = 0, *t.cnt = rcnt;
    int dest = index2offset(0, key.type);
    int src = index2offset(lcnt, key.type);
    int end = index2offset(*cnt, key.type) + 6;
    *cnt = lcnt;
    memcpy(t.base + dest, base + src, end - src);
    t.origin.setDirty(t.getFileAddr(), t.getBlockAddr());
    if (*base == 0)
        *reinterpret_cast<word *>(base + src) = t.getBlockAddr();
    else
        --*cnt;
    memcpy(phyAddrSave, phyAddr, src);
    phyAddr = phyAddrSave;
    return t;
}
int bnode::splice(bnode &par, bnode &src, int type)
{
    // bnode par(origin.getBlock(getFileAddr(), *parent));
    origin.setDirty(getFileAddr(), getBlockAddr());
    src.origin.setDirty(src.getFileAddr(), src.getBlockAddr());
    par.origin.setDirty(par.getFileAddr(), par.getBlockAddr());
    if (element(type, src.base + index2offset(0, type) + 6) < element(type, base + index2offset(0, type) + 6))
    {
        int firstKey = index2offset(0, type);
        int lastKey = index2offset(*src.cnt - 1, type);
        element key = element(type, src.base + lastKey + 6);
        dword virtAddr = combileVirtAddr(0, *reinterpret_cast<word *>(src.base + lastKey), *reinterpret_cast<hword *>(src.base + lastKey + 4));
        if (*base == 0)
        {
            element oldKey(type, base + firstKey + 6);
            par.replaceKey(oldKey, key);
            insertKey(key, virtAddr);
            src.move(*src.cnt, -1, type);
        }
        else
        {
            int idx = par.binarySearch(key);
            element parentKey(type, par.base + index2offset(idx, type) + 6);
            insertKey(parentKey, virtAddr);
            parentKey.cpy(key);
            --*src.cnt;
        }
    }
    else
    {
        int firstKey = index2offset(0, type);
        dword virtAddr = combileVirtAddr(0, *reinterpret_cast<word *>(src.base + firstKey), *reinterpret_cast<hword *>(src.base + firstKey + 4));
        if (*base == 0)
        {
            int secondKey = index2offset(1, type);
            element oldKey = element(type, src.base + firstKey + 6);
            element key = element(type, src.base + secondKey + 6);
            par.replaceKey(oldKey, key);
            insertKey(oldKey, virtAddr);
            src.move(1, -1, type);
        }
        else
        {
            int lastKey = index2offset(*cnt - 1, type);
            element pKey = element(type, base + lastKey + 6);
            int idx = par.binarySearch(pKey);
            element parKey = element(type, par.base + index2offset(idx, type) + 6);
            element key = element(type, src.base + firstKey + 6);
            insertKey(parKey, virtAddr);
            parKey.cpy(key);
            src.move(1, -1, type);
        }
    }
    return 0;
}
element bnode::merge(bnode &par, bnode &src, int type)
{
    //bnode par(origin.getBlock(getFileAddr(), *parent));
    origin.setDirty(getFileAddr(), getBlockAddr());
    //int firstKey = index2offset(0, type);
    int lastKey = index2offset(*cnt - 1, type);
    element key = element(type, base + lastKey + 6);
    if (*base == 0)
    {
        int dest = index2offset(*cnt, type);
        int end = index2offset(*src.cnt, type) + 6;
        memcpy(base + dest, src.base, end);
        *cnt += *src.cnt;
    }
    else
    {
        int idx = par.binarySearch(key);
        element parKey(type, par.base + index2offset(idx, type) + 6);
        element endKey(type, base + index2offset(*cnt, type) + 6);
        endKey.cpy(parKey);
        int dest = index2offset(*cnt + 1, type);
        int end = index2offset(*src.cnt, type) + 6;
        memcpy(base + dest, src.base, end);
        *cnt += *src.cnt + 1;
    }
    src.deleteBlock();
    return key;
}
word IndexManager::find(const word curAddr, const word parentAddr, const element &keyValue)
{
    bnode cur(bufMgr.getBlock(indexFileAddr, curAddr));
    cur.setParent(parentAddr);
    if (cur.getBase() == 0)
        return curAddr;
    else
        return find(extractBlockAddr(cur.find(keyValue)), curAddr, keyValue);
}
dword IndexManager::findAddrEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue)
{
    word leafAddr = find(rootAddr, 0, keyValue);
    bnode leaf(bufMgr.getBlock(indexFileAddr, leafAddr));
    dword res = leaf.find(keyValue);
    return res > 0 ? combileVirtAddr(dataFileAddr, extractBlockAddr(res), extractOffset(res)) : 0;
}
node IndexManager::findRecordEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue)
{
    dword virtAddr = findAddrEntry(dataFileAddr, rootAddr, keyValue);
    return bufMgr.getRecord(dataFileAddr, extractBlockAddr(virtAddr), extractOffset(virtAddr));
}
word IndexManager::insertInParent(bnode &cur, const element &key, const word addr)
{
    if (*(cur.base + 1) == 1)
    {
        bnode newRoot(bufMgr.getNextFree(indexFileAddr));
        *(cur.base + 1) = 0;
        newRoot.origin.setDirty(indexFileAddr, newRoot.getBlockAddr());
        *newRoot.base = 1, *(newRoot.base + 1) = 1, *newRoot.cnt = 0;
        *reinterpret_cast<word *>(newRoot.base + newRoot.index2offset(0, key.type)) = cur.getBlockAddr();
        newRoot.insertKey(key, combileVirtAddr(0, addr, 0), DATA_PTR);
        return newRoot.getBlockAddr();
    }
    bnode p(bufMgr.getBlock(indexFileAddr, cur.getParent()));
    if (p.getCnt() < bcnt[key.type] - 1)
    {
        p.insertKey(key, combileVirtAddr(0, addr, 0), DATA_PTR);
        return 0;
    }
    else
    {
        bnode t = p.split(key, combileVirtAddr(0, addr, 0));
        element tKey(key.type, t.base + t.index2offset(0, key.type) + 6);
        return insertInParent(p, tKey, t.getBlockAddr());
    }
}
int IndexManager::insertEntry(const hword dataFileAddr, attribute &attr, const element &keyValue, dword virtAddr)
{
    int &rootAddr = attr.indexRootAddr;
    const int &type = attr.type;
    word leafAddr = find(rootAddr, 0, keyValue);
    bnode leaf(bufMgr.getBlock(indexFileAddr, leafAddr));
    if (leaf.getCnt() < bcnt[type] - 1)
        leaf.insertKey(keyValue, virtAddr);
    else
    {
        bnode t = leaf.split(keyValue, virtAddr);
        element key(type, t.base + t.index2offset(0, type) + 6);
        word tAddr = insertInParent(leaf, key, t.getBlockAddr());
        if (tAddr > 0)
            rootAddr = tAddr;
    }
    return SUCCESS;
}
word IndexManager::dlt()
{

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
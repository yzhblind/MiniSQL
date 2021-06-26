#include "IndexManager.hpp"
#include "Type.hpp"
#include <cstring>
//判断元素a所指向的元素是否小于b所指向的元素
//不进行类型检查，类型解释依赖元素a
bool operator<(const element &a, const element &b)
{
    switch (a.type)
    {
    case 0:
        return *static_cast<int *>(a.ptr) < *static_cast<int *>(b.ptr);
        break;
    case 1:
        return *static_cast<float *>(a.ptr) < *static_cast<float *>(b.ptr);
        break;
    default:
        return strncmp(static_cast<char *>(a.ptr), static_cast<char *>(b.ptr), a.type) < 0;
        break;
    }
}
//判断元素a所指向的元素是否小于等于b所指向的元素
//不进行类型检查，类型解释依赖元素a
bool operator<=(const element &a, const element &b)
{
    switch (a.type)
    {
    case 0:
        return *static_cast<int *>(a.ptr) <= *static_cast<int *>(b.ptr);
        break;
    case 1:
        return *static_cast<float *>(a.ptr) <= *static_cast<float *>(b.ptr);
        break;
    default:
        return strncmp(static_cast<char *>(a.ptr), static_cast<char *>(b.ptr), a.type) <= 0;
        break;
    }
}
//判断元素a所指向的元素是否等于b所指向的元素
//不进行类型检查，类型解释依赖元素a
bool operator==(const element &a, const element &b)
{
    switch (a.type)
    {
    case 0:
        return *static_cast<int *>(a.ptr) == *static_cast<int *>(b.ptr);
        break;
    case 1:
        return *static_cast<float *>(a.ptr) == *static_cast<float *>(b.ptr);
        break;
    default:
        return strncmp(static_cast<char *>(a.ptr), static_cast<char *>(b.ptr), a.type) == 0;
        break;
    }
}

void *bnode::tmpMemory = NULL;

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
int bnode::binarySearch(const element &e, const bool lFlag)
{
    if (lFlag ? e < getElement(getCnt() - 1, e.type) : e <= getElement(getCnt() - 1, e.type))
    {
        int l = 0, r = getCnt() - 1;
        int mid;
        while (l < r)
        {
            mid = (l + r) >> 1;
            if (lFlag ? e < getElement(mid, e.type) : e <= getElement(mid, e.type))
                r = mid;
            else
                l = mid + 1;
        }
        return r;
    }
    else
        return getCnt();
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
dword bnode::find(const element &key, const packing packingType, const bool equalFlag, const bool lFlag)
{
    int idx = binarySearch(key, lFlag);
    if (idx != getCnt() || (idx == getCnt() && packingType == PTR_DATA && equalFlag == false))
    {
        // if (key == element(key.type, base + offset + 6))
        // {
        if (equalFlag == false || (equalFlag == true && key == getElement(idx, key.type)))
        {
            int offset = index2offset(idx, key.type);
            word blockAddr = *reinterpret_cast<word *>(base + offset + (packingType == PTR_DATA ? 0 : 6 + type2size(key.type)));
            hword offsetAddr = *reinterpret_cast<hword *>(base + offset + 4 + (packingType == PTR_DATA ? 0 : 6 + type2size(key.type)));
            return combileVirtAddr(getFileAddr(), blockAddr, offsetAddr);
        }
        // }
    }
    return 0;
}
int bnode::insertKey(const element &key, const dword virtAddr, const packing packingType)
{
    //if (*cnt == idxMgr.bcnt[key.type] - 1)
    //    return FAILURE;
    const int &type = key.type;
    int idx = (*cnt == 0) ? 0 : binarySearch(key);
    origin.setDirty(getFileAddr(), getBlockAddr());
    char *p = static_cast<char *>(move(idx, 1, type, packingType));
    *reinterpret_cast<word *>(p + (packingType == PTR_DATA ? 0 : type2size(type))) = extractBlockAddr(virtAddr);
    *reinterpret_cast<hword *>(p + (packingType == PTR_DATA ? 0 : type2size(type)) + 4) = extractOffset(virtAddr);
    element(type, p + (packingType == PTR_DATA ? 6 : 0)).cpy(key);
    return idx;
}
int bnode::deleteKey(const element &key, const packing packingType)
{
    int idx = binarySearch(key);
    if (idx != *cnt)
    {
        // int offset = index2offset(idx, key.type) + 6;
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
        element t = getElement(idx, key.type);
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
    base = reinterpret_cast<char *>(phyAddr);
    parent = reinterpret_cast<word *>(base + 2);
    cnt = reinterpret_cast<int *>(base + 6);
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
    {
        *reinterpret_cast<word *>(base + src) = t.getBlockAddr();
        *reinterpret_cast<hword *>(base + src + 4) = 0;
    }
    else
        --*cnt;
    memcpy(phyAddrSave, phyAddr, src + 6 + type2size(key.type));
    phyAddr = phyAddrSave;
    base = reinterpret_cast<char *>(phyAddr);
    parent = reinterpret_cast<word *>(base + 2);
    cnt = reinterpret_cast<int *>(base + 6);
    return t;
}
int bnode::splice(bnode &par, bnode &src, int type)
{
    // bnode par(origin.getBlock(getFileAddr(), *parent));
    origin.setDirty(getFileAddr(), getBlockAddr());
    src.origin.setDirty(src.getFileAddr(), src.getBlockAddr());
    par.origin.setDirty(par.getFileAddr(), par.getBlockAddr());
    if (src.getElement(0, type) <= getElement(0, type))
    {
        // int firstKey = index2offset(0, type);
        int lastKey = index2offset(src.getCnt() - 1, type);
        element key = src.getElement(src.getCnt() - 1, type);
        dword virtAddr = combileVirtAddr(0, *reinterpret_cast<word *>(src.base + lastKey), *reinterpret_cast<hword *>(src.base + lastKey + 4));
        if (*base == 0)
        {
            element oldKey = getElement(0, type);
            par.replaceKey(oldKey, key);
            insertKey(key, virtAddr);
            src.move(*src.cnt, -1, type);
        }
        else
        {
            int idx = par.binarySearch(key);
            element parentKey = par.getElement(idx, type);
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
            // int secondKey = index2offset(1, type);
            element oldKey = src.getElement(0, type);
            element key = src.getElement(1, type);
            par.replaceKey(oldKey, key);
            insertKey(oldKey, virtAddr);
            src.move(1, -1, type);
        }
        else
        {
            // int lastKey = index2offset(*cnt - 1, type);
            element pKey = getElement(getCnt() - 1, type);
            int idx = par.binarySearch(pKey);
            element parKey = par.getElement(idx, type);
            element key = src.getElement(0, type);
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
    // int lastKey = index2offset(*cnt - 1, type);
    element key = getElement(getCnt() - 1, type);
    int head = src.index2offset(0, type);
    if (*base == 0)
    {
        int dest = index2offset(*cnt, type);
        int end = index2offset(*src.cnt, type) + 6;
        memcpy(base + dest, src.base + head, end - head);
        *cnt += *src.cnt;
    }
    else
    {
        int idx = par.binarySearch(key);
        element parKey = par.getElement(idx, type);
        element endKey = getElement(getCnt(), type);
        endKey.cpy(parKey);
        int dest = index2offset(*cnt + 1, type);
        int end = index2offset(*src.cnt, type) + 6;
        memcpy(base + dest, src.base + head, end - head);
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
        return find(extractBlockAddr(cur.find(keyValue, PTR_DATA, false, true)), curAddr, keyValue);
}
dword IndexManager::findAddrEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue)
{
    word leafAddr = find(rootAddr, 0, keyValue);
    bnode leaf(bufMgr.getBlock(indexFileAddr, leafAddr));
    dword res = leaf.find(keyValue, PTR_DATA, true);
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
        // element tKey = t.getElement(0, key.type);
        element tKey = p.getElement(p.getCnt(), key.type);
        return insertInParent(p, tKey, t.getBlockAddr());
    }
}
int IndexManager::insertEntry(attribute &attr, const element &keyValue, dword virtAddr)
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
        element key = t.getElement(0, type);
        word tAddr = insertInParent(leaf, key, t.getBlockAddr());
        if (tAddr > 0)
            rootAddr = tAddr;
    }
    return SUCCESS;
}
word IndexManager::erase(bnode &cur, const element &keyValue, const packing delType)
{
    const int type = keyValue.type;
    cur.deleteKey(keyValue, delType);
    if (*(cur.base + 1) == 1)
    {
        if (cur.getCnt() == 0)
        {
            word t = *reinterpret_cast<word *>(cur.base + cur.index2offset(0, type));
            bnode root(bufMgr.getBlock(indexFileAddr, t));
            *(root.base + 1) = 1;
            root.origin.setDirty(indexFileAddr, t);
            cur.deleteBlock();
            return t;
        }
    }
    else if ((cur.getBase() == 0 && cur.getCnt() < bLeafMincnt[type]) || (cur.getBase() == 1 && cur.getCnt() < bInMincnt[type]))
    {
        bnode par(bufMgr.getBlock(indexFileAddr, cur.getParent()));
        element searchKey = cur.getElement(cur.getCnt() - 1, type);
        int idx = par.binarySearch(searchKey);
        word tAddr;
        if (idx == par.getCnt())
            tAddr = *reinterpret_cast<word *>(par.base + par.index2offset(idx - 1, type));
        else
            tAddr = *reinterpret_cast<word *>(par.base + par.index2offset(idx + 1, type));
        bnode t(bufMgr.getBlock(indexFileAddr, tAddr));
        t.setParent(cur.getParent());
        bnode &a = (idx == par.getCnt()) ? t : cur;
        bnode &b = (idx == par.getCnt()) ? cur : t;
        if ((a.getBase() == 0 && a.getCnt() + b.getCnt() < bcnt[type]) || (a.getBase() == 1 && a.getCnt() + b.getCnt() < bcnt[type] - 1))
        {
            element key = a.merge(par, b, type);
            return erase(par, key, DATA_PTR);
        }
        else
            cur.splice(par, t, type);
    }
    return 0;
}
int IndexManager::deleteEntry(std::vector<attribute> &col, const filter &flt)
{
    int keyStart = 0;
    for (auto &attr : col)
    {
        if (attr.isUnique == false || attr.indexRootAddr == 0)
        {
            keyStart += type2size(attr.type);
            continue;
        }
        for (auto record : flt.res)
        {
            element key(attr.type, static_cast<char *>(record) + keyStart);
            word leafAddr = find(attr.indexRootAddr, 0, key);
            bnode leaf(bufMgr.getBlock(indexFileAddr, leafAddr));
            if (*leaf.base == 0 && *(leaf.base + 1) == 1)
                leaf.deleteKey(key);
            else
            {
                word tAddr = erase(leaf, key, PTR_DATA);
                if (tAddr > 0)
                    attr.indexRootAddr = tAddr;
            }
        }
        keyStart += type2size(attr.type);
    }
    return SUCCESS;
}
int IndexManager::createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos)
{
    if (col[pos].isUnique == false)
        return FAILURE;
    const int type = col[pos].type;
    int &rootAddr = col[pos].indexRootAddr;
    bnode root(bufMgr.getNextFree(indexFileAddr));
    rootAddr = root.getBlockAddr();
    *(root.base) = 0, *(root.base + 1) = 1, *(root.cnt) = 0;
    *reinterpret_cast<dword *>(root.base + root.index2offset(0, type)) = 0;
    root.origin.setDirty(indexFileAddr, rootAddr);
    int keyOffset = 0;
    for (int i = 0; i < pos; ++i)
        keyOffset += type2size(col[i].type);
    int blockNum = bufMgr.getBlockNumber(dataFileAddr);
    int rSize = bufMgr.getRecordSize(dataFileAddr);
    if (blockNum == FAILURE || rSize == FAILURE)
        return FAILURE;
    char *record = new char[rSize + 1];
    for (int i = 1; i < blockNum; ++i)
    {
        node t = bufMgr.getBlock(dataFileAddr, i);
        int tSize = t.getSize();
        for (int offset = 0; offset < tSize - rSize; offset += rSize + 1)
        {
            t.read(record, offset, rSize + 1);
            if (*record == true)
            {
                element key(type, record + 1 + keyOffset);
                insertEntry(col[pos], key, t.getVirtAddr() + offset);
            }
        }
    }
    delete[] record;
    return SUCCESS;
}
void IndexManager::drop(word curAddr, const int type)
{
    bnode cur(bufMgr.getBlock(indexFileAddr, curAddr));
    if (cur.getBase() == 1)
        for (int i = 0; i <= cur.getCnt(); ++i)
            drop(*reinterpret_cast<word *>(cur.base + cur.index2offset(i, type)), type);
    cur.deleteBlock();
}
int IndexManager::dropEntry(std::vector<attribute> &col, int pos)
{
    if (col[pos].indexRootAddr == 0)
        return FAILURE;
    const int type = col[pos].type;
    int &rootAddr = col[pos].indexRootAddr;
    drop(rootAddr, type);
    rootAddr = 0;
    return SUCCESS;
}

IndexManager idxMgr;
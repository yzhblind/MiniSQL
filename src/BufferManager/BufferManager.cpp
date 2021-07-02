#include "BufferManager.hpp"
//#include "CatalogManager.hpp"
#include "Type.hpp"

const long long BufferManager::pageSize = 4096;
long long BufferManager::bufferSize = 1024;

int BufferManager::writeBlock2File(const int bufID)
{
    FILE *fp = file.pointer[extractFileAddr(buf.tag[bufID])];
    fseeko64(fp, extractBlockAddr(buf.tag[bufID]) * pageSize, SEEK_SET);
    return fwrite(buf.pool[bufID], pageSize, 1, fp);
}
int BufferManager::movBlock2Buffer(const hword fileAddr, const word blockAddr)
{
    //if (notValidAddr(fileAddr))
    //    return FAILURE;
    dword tag = combileVirtAddr(fileAddr, blockAddr, 0);
    auto iter = buf.tagIndex.find(tag);
    if (iter != buf.tagIndex.end())
    {
        int index = iter->second;
        if (buf.pinned[index] == 0)
            buf.LRU.splice(buf.LRU.begin(), buf.LRU, buf.LRUIndex[index]);
        return index;
    }
    else
    {
        if (buf.LRU.size() < bufferSize / 2)
            resize();
        int index = *buf.LRU.rbegin();
        if (buf.valid[index] == true)
        {
            if (buf.dirty[index] == true)
                writeBlock2File(index);
            buf.tagIndex.erase(buf.tag[index]);
        }
        buf.valid[index] = true;
        buf.dirty[index] = false;
        buf.pinned[index] = 0;
        buf.tag[index] = tag;
        buf.tagIndex[tag] = index;
        FILE *fp = file.pointer[fileAddr];
        fseeko64(fp, blockAddr * pageSize, SEEK_SET);
        if (blockAddr < file.blockNum[fileAddr])
            fread(buf.pool[index], pageSize, 1, fp);
        else
        {
            file.blockNum[fileAddr] = blockAddr + 1;
            memset(buf.pool[index], 0, pageSize);
            fwrite(buf.pool[index], pageSize, 1, fp);
        }
        buf.LRU.splice(buf.LRU.begin(), buf.LRU, buf.LRUIndex[index]);
        return index;
    }
}
int BufferManager::writeMetaData(const hword fileAddr)
{
    int index = movBlock2Buffer(fileAddr, 0);
    int *meta = static_cast<int *>(buf.pool[index]);
    meta[0] = file.blockNum[fileAddr];
    meta[1] = file.recordSize[fileAddr];
    meta[2] = file.nextFree[fileAddr];
    buf.dirty[index] = true;
    return index;
}
void BufferManager::bufferAdjust()
{
    buf.valid.resize(bufferSize, false);
    buf.dirty.resize(bufferSize, false);
    buf.pinned.resize(bufferSize, 0);
    buf.tag.resize(bufferSize, 0);
}
BufferManager::BufferManager()
{
    bufferAdjust();
    for (int i = 0; i < bufferSize; ++i)
        buf.pool.push_back(new char[pageSize]);
    for (int i = 0; i < bufferSize; ++i)
    {
        buf.LRU.push_front(i);
        buf.LRUIndex.push_back(buf.LRU.begin());
    }
}
BufferManager::~BufferManager()
{
    for (int i = 0; i < file.pointer.size(); ++i)
        if (file.valid[i] == true)
            writeMetaData(i);
    for (int i = 0; i < bufferSize; ++i)
        if (buf.valid[i] == true && buf.dirty[i] == true)
            writeBlock2File(i);
    for (int i = 0; i < bufferSize; ++i)
        delete[] static_cast<char *>(buf.pool[i]);
    for (int i = 0; i < file.pointer.size(); ++i)
        if (file.valid[i] == true)
            fclose(file.pointer[i]);
}
int BufferManager::resize()
{
    bufferSize *= 2;
    if (bufferSize >= 1024 * 1024)
        return FAILURE;
    bufferAdjust();
    for (int i = bufferSize / 2; i < bufferSize; ++i)
        buf.pool.push_back(new char[pageSize]);
    for (int i = bufferSize / 2; i < bufferSize; ++i)
    {
        buf.LRU.push_front(i);
        buf.LRUIndex.push_back(buf.LRU.begin());
    }
    return SUCCESS;
}
int BufferManager::newDataBlock(const hword fileAddr)
{
    int index = movBlock2Buffer(fileAddr, file.blockNum[fileAddr]);
    buf.dirty[index] = true;
    char *base = static_cast<char *>(buf.pool[index]);
    hword *pHword;
    word *pWord;
    pWord = reinterpret_cast<word *>(base);
    *pWord = file.nextFree[fileAddr];
    file.nextFree[fileAddr] = file.blockNum[fileAddr] - 1;
    pHword = reinterpret_cast<hword *>(base + 4);
    *pHword = 6;
    int recordNumber = (pageSize - 6) / file.recordSize[fileAddr];
    int maxOffset = 6 + (recordNumber - 1) * file.recordSize[fileAddr];
    for (int offset = 6; offset <= maxOffset; offset += file.recordSize[fileAddr])
    {
        pHword = reinterpret_cast<hword *>(base + offset + 1);
        *pHword = offset + file.recordSize[fileAddr];
    }
    pHword = reinterpret_cast<hword *>(base + maxOffset + 1);
    *pHword = 0;
    return index;
}
int BufferManager::openFile(const std::string filename, const fileType type, const int recordSize)
{
    bool newFileFlag = false;
    FILE *fp = fopen64(filename.c_str(), "rb+");
    if (fp == NULL)
    {
        fp = fopen64(filename.c_str(), "wb+");
        newFileFlag = true;
    }
    if (fp == NULL || (newFileFlag == true && recordSize <= 1 && type == DATA))
        return FAILURE;
    int fileAddr;
    if (file.freelist.empty())
    {
        fileAddr = file.pointer.size();
        file.pointer.push_back(fp);
        file.type.push_back(type);
        file.valid.push_back(true);
        file.blockNum.push_back(newFileFlag ? 0 : 1);
        file.recordSize.push_back(recordSize + 1);
        file.nextFree.push_back(0);
    }
    else
    {
        fileAddr = *file.freelist.rbegin();
        file.freelist.pop_back();
        file.pointer[fileAddr] = fp;
        file.type[fileAddr] = type;
        file.valid[fileAddr] = true;
        file.blockNum[fileAddr] = newFileFlag ? 0 : 1;
        file.recordSize[fileAddr] = recordSize + 1;
        file.nextFree[fileAddr] = 0;
    }
    int index = movBlock2Buffer(fileAddr, 0);
    if (!newFileFlag)
    {
        int *meta = static_cast<int *>(buf.pool[index]);
        file.blockNum[fileAddr] = meta[0];
        file.recordSize[fileAddr] = meta[1];
        file.nextFree[fileAddr] = meta[2];
    }
    return fileAddr;
}
int BufferManager::removeFile(const hword fileAddr, const std::string filename)
{
    if (notValidAddr(fileAddr))
        return FAILURE;
    //writeMetaData(fileAddr);
    for (int i = 0; i < bufferSize; ++i)
        if (extractFileAddr(buf.tag[i]) == fileAddr && buf.valid[i] == true)
        {
            //if (buf.dirty[i] == true)
            //    writeBlock2File(i);
            buf.valid[i] = false;
            buf.tagIndex.erase(buf.tag[i]);
            if (buf.pinned[i] > 0)
                buf.LRU.splice(buf.LRU.end(), buf.fixed, buf.LRUIndex[i]);
            else
                buf.LRU.splice(buf.LRU.end(), buf.LRU, buf.LRUIndex[i]);
        }
    file.valid[fileAddr] = false;
    fclose(file.pointer[fileAddr]);
    file.freelist.push_front(fileAddr);
    return remove(filename.c_str()) ? FAILURE : SUCCESS;
}
node BufferManager::getNextFree(const hword fileAddr)
{
    if (notValidAddr(fileAddr))
        return node(*this);
    int index, blockAddr, offset, *metaBlock;
    char *recordHeader;
    hword *metaOffset, *recordContent;
    switch (file.type[fileAddr])
    {
    case DATA:
        if (file.nextFree[fileAddr] == 0)
            index = newDataBlock(fileAddr);
        else
            index = movBlock2Buffer(fileAddr, file.nextFree[fileAddr]);
        blockAddr = file.nextFree[fileAddr];
        metaOffset = static_cast<hword *>(buf.pool[index]) + 2;
        offset = *metaOffset;
        recordHeader = static_cast<char *>(buf.pool[index]) + offset;
        *recordHeader = true;
        recordContent = reinterpret_cast<hword *>(recordHeader + 1);
        *metaOffset = *recordContent;
        if (*metaOffset == 0)
        {
            metaBlock = static_cast<int *>(buf.pool[index]);
            file.nextFree[fileAddr] = *metaBlock;
            *metaBlock = 0;
        }
        return node(*this, combileVirtAddr(fileAddr, blockAddr, offset + 1), recordContent, index, file.recordSize[fileAddr] - 1);
        break;

    case INDEX:
        if (file.nextFree[fileAddr] == 0)
            index = movBlock2Buffer(fileAddr, (blockAddr = file.blockNum[fileAddr]));
        else
        {
            index = movBlock2Buffer(fileAddr, (blockAddr = file.nextFree[fileAddr]));
            metaBlock = static_cast<int *>(buf.pool[index]);
            file.nextFree[fileAddr] = *metaBlock;
        }
        return node(*this, combileVirtAddr(fileAddr, blockAddr, 0), buf.pool[index], index, pageSize);
        break;

    case CATALOG:
        index = movBlock2Buffer(fileAddr, (blockAddr = file.blockNum[fileAddr]));
        return node(*this, combileVirtAddr(fileAddr, blockAddr, 0), buf.pool[index], index, pageSize);
        break;

    default:
        return node(*this);
    }
}
// node BufferManager::getRootBlock(const hword fileAddr)
// {
//     if (notValidAddr(fileAddr) || file.type[fileAddr] != INDEX)
//         return node(*this);
//     int index = movBlock2Buffer(fileAddr, file.recordSize[fileAddr]);
//     return node(*this, combileVirtAddr(fileAddr, file.recordSize[fileAddr], 0), buf.pool[index], index, pageSize);
// }
// int BufferManager::setRootBlock(const hword fileAddr, const word blockAddr)
// {
//     if (notValidAddr(fileAddr) || notValidAddr(fileAddr, blockAddr))
//         return FAILURE;
//     file.recordSize[fileAddr] = blockAddr;
//     return SUCCESS;
// }
node BufferManager::getBlock(const hword fileAddr, const word blockAddr)
{
    if (notValidAddr(fileAddr))
        return node(*this);
    int index = movBlock2Buffer(fileAddr, blockAddr);
    int offset = (file.type[fileAddr] == DATA && blockAddr > 0) ? 6 : 0;
    return node(*this, combileVirtAddr(fileAddr, blockAddr, offset), static_cast<char *>(buf.pool[index]) + offset, index, pageSize - offset);
}
int BufferManager::setDirty(const hword fileAddr, const word blockAddr)
{
    auto iter = buf.tagIndex.find(combileVirtAddr(fileAddr, blockAddr, 0));
    if (iter == buf.tagIndex.end())
        return FAILURE;
    buf.dirty[iter->second] = true;
    return SUCCESS;
}
int BufferManager::deleteBlock(const hword fileAddr, const word blockAddr)
{
    if (notValidAddr(fileAddr) || notValidAddr(fileAddr, blockAddr) || file.type[fileAddr] != INDEX)
        return FAILURE;
    int index = movBlock2Buffer(fileAddr, blockAddr);
    buf.dirty[index] = true;
    // memset(buf.pool[index],0,pageSize);
    int *meta = static_cast<int *>(buf.pool[index]);
    *meta = file.nextFree[fileAddr];
    file.nextFree[fileAddr] = blockAddr;
    return SUCCESS;
}
int BufferManager::pinBlock(const hword fileAddr, const word blockAddr, const pinType type)
{
    if (notValidAddr(fileAddr) || notValidAddr(fileAddr, blockAddr))
        return FAILURE;
    auto iter = buf.tagIndex.find(combileVirtAddr(fileAddr, blockAddr, 0));
    int index = (iter == buf.tagIndex.end()) ? movBlock2Buffer(fileAddr, blockAddr) : iter->second;
    switch (type)
    {
    case PIN:
        ++buf.pinned[index];
        if (buf.pinned[index] == 1)
            buf.fixed.splice(buf.fixed.end(), buf.LRU, buf.LRUIndex[index]);
        break;

    case UNPIN:
        --buf.pinned[index];
        if (buf.pinned[index] == 0)
            buf.LRU.splice(buf.LRU.begin(), buf.fixed, buf.LRUIndex[index]);
        break;

    default:
        return FAILURE;
    }
    return SUCCESS;
}
node BufferManager::getRecord(const hword fileAddr, const word blockAddr, const hword offset)
{
    if (notValidAddr(fileAddr) || notValidAddr(fileAddr, blockAddr) || file.type[fileAddr] != DATA)
        return node(*this);
    if (offset < 6 || offset > pageSize - file.recordSize[fileAddr])
        return node(*this);
    int headOffset = offset - ((offset - 6) % file.recordSize[fileAddr]);
    int index = movBlock2Buffer(fileAddr, blockAddr);
    char *recordHeader = static_cast<char *>(buf.pool[index]) + headOffset;
    if (*recordHeader == false)
        return node(*this);
    char *recordContent = recordHeader + 1;
    return node(*this, combileVirtAddr(fileAddr, blockAddr, headOffset + 1), recordContent, index, file.recordSize[fileAddr] - 1);
}
int BufferManager::deleteRecord(const hword fileAddr, const word blockAddr, const hword offset)
{
    if (notValidAddr(fileAddr) || notValidAddr(fileAddr, blockAddr) || file.type[fileAddr] != DATA)
        return FAILURE;
    if (offset < 6 || offset > pageSize - file.recordSize[fileAddr])
        return FAILURE;
    int headOffset = offset - ((offset - 6) % file.recordSize[fileAddr]);
    int index = movBlock2Buffer(fileAddr, blockAddr);
    hword *metaOffset = static_cast<hword *>(buf.pool[index]) + 2;
    char *recordHeader = static_cast<char *>(buf.pool[index]) + headOffset;
    if (*recordHeader == false)
        return false;
    buf.dirty[index] = true;
    *recordHeader = false;
    hword *recordContent = reinterpret_cast<hword *>(recordHeader + 1);
    *recordContent = *metaOffset;
    *metaOffset = headOffset;
    if (*recordContent == 0)
    {
        int *metaBlock = static_cast<int *>(buf.pool[index]);
        *metaBlock = file.nextFree[fileAddr];
        file.nextFree[fileAddr] = blockAddr;
    }
    return SUCCESS;
}
bool BufferManager::checkNodeValid(const node &x) const
{
    if (x.getSize() <= 0)
        return false;
    if (buf.valid[x.getBufferID()] == true && buf.tag[x.getBufferID()] == (x.getVirtAddr() & 0xFFFFFFFFFFFF0000))
        return true;
    else
        return false;
}

int node::read(void *const dest, const int offset, const int byteSize) const
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return FAILURE;
    memcpy(dest, static_cast<char *>(phyAddr) + offset, byteSize);
    return SUCCESS;
}
int node::write(const void *const src, const int offset, const int byteSize)
{
    if (offset + byteSize > size || offset < 0 || byteSize <= 0)
        return FAILURE;
    origin.setDirty(extractFileAddr(virtAddr), extractBlockAddr(virtAddr));
    memcpy(static_cast<char *>(phyAddr) + offset, src, byteSize);
    return SUCCESS;
}

BufferManager bufMgr;
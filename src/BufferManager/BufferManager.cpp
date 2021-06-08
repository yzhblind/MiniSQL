#include "BufferManager.hpp"

const int BufferManager::pageSize = 4096;
int BufferManager::getPageSize() { return pageSize; }
const int BufferManager::bufferSize = 4096;
int BufferManager::getBufferSize() { return bufferSize; }

BufferManager::node::node(dword virtAddr, void *phyAddr, int size) : virtAddr(virtAddr), phyAddr(phyAddr), size(size) {}
hword BufferManager::node::getFileAddr() const { return virtAddr >> 48; }
hword BufferManager::node::getOffset() const { return virtAddr & 0xFFFF; }
word BufferManager::node::getBlockAddr() const { return (virtAddr >> 16) & 0xFFFFFFFF; }
dword BufferManager::node::getVirtAddr() const { return virtAddr; }

BufferManager::BufferManager()
{
}
BufferManager::~BufferManager()
{
}

BufferManager bufMgr;

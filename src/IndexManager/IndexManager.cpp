#include "IndexManager.hpp"
#include "Type.hpp"

bnode::bnode(const node &src) : node(src)
{
    origin.pinBlock(getFileAddr(), getBlockAddr(), PIN);
    //TODO
}
bnode::~bnode()
{
    //TODO
    origin.pinBlock(getFileAddr(), getBlockAddr(), UNPIN);
}

dword IndexManager::findAddrEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue, const int recordSize)
{
}
node IndexManager::findNodeEntry(const hword dataFileAddr, const word rootAddr, const element &keyValue, const int recordSize)
{
}
int IndexManager::insertEntry(const hword dataFileAddr, attribute &attr, const element &keyValue, const int recordSize)
{
}
int IndexManager::deleteEntry(const hword dataFileAddr, std::vector<attribute> &col, const filter &flt)
{
}
int IndexManager::createEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos)
{
}
int IndexManager::dropEntry(const hword dataFileAddr, std::vector<attribute> &col, int pos)
{
}

IndexManager idxMgr;
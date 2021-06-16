#include "BufferManager.hpp"
#include "CatalogManager.hpp"

#include <sstream>

int CatalogManager::startCata()
{
    cataFileAddr = bufMgr.openFile("CATALOG", CATALOG);
    char *readBuff;
    int schemaNum;
    int blockNum;
    std::stringstream ss;

    node firstBlock = bufMgr.getBlock(cataFileAddr, 1);
    blockNum = bufMgr.getBlockNumber(cataFileAddr);    
    
    readBuff = (char *)malloc(sizeof(char) * BufferManager::pageSize);
    if(readBuff == NULL)
        return FAILURE;

    firstBlock.read(readBuff, 0, BufferManager::pageSize);
    ss.write(readBuff, BufferManager::pageSize);
    
    // 一次性全部拿进来
    for(int i = 2; i < blockNum; i++)
    {
        node schemaBlock = bufMgr.getBlock(cataFileAddr, i);
        schemaBlock.read(readBuff, 0, BufferManager::pageSize);
        ss.write(readBuff, BufferManager::pageSize);
    }

    ss >> schemaNum;

    for(int i = 0; i < schemaNum; i++)
    {
        schema tmpSchema;
        attribute tmpAttr;
        int columnNum;
        ss >> tmpSchema.tableName;
        ss >> columnNum;
        ss >> tmpSchema.primaryKey;
        ss >> tmpSchema.recordSize;

        for(int j = 0; j < columnNum; j++)
        {
            ss >> tmpAttr.attrName;
            ss >> tmpAttr.type;
            ss >> tmpAttr.isUnique;
            ss >> tmpAttr.indexRootAddr;
            tmpSchema.column.push_back(tmpAttr);
            tmpSchema.attributeIndex[tmpAttr.attrName] = j;
        }
        tmpSchema.fileAddr = bufMgr.openFile(tmpSchema.tableName, DATA, tmpSchema.recordSize);
        ctgMgr.schemas.push_back(tmpSchema);
        ctgMgr.schemaIndex[tmpSchema.tableName] = i;
    }
    ss.str("");
    ss.clear();
    return SUCCESS;
}

/*
std::string tableName;
std::map<std::string, int> attributeIndex;
std::vector<attribute> column;
int fileAddr;
int primaryKey;
int recordSize;
*/

int CatalogManager::endCata()
{
    std::stringstream ss;
    char *writeBuff;
    int schemaNum = ctgMgr.schemas.size();
    int blockNum = bufMgr.getBlockNumber(cataFileAddr);
    node firstBlock = bufMgr.getBlock(cataFileAddr, 1);

    ss << schemaNum;
    ss.read(writeBuff, BufferManager::pageSize);
    firstBlock.write(writeBuff, 0, BufferManager::pageSize);

    for(int i = 0; i < schemaNum; i++)
    {
        ss << ctgMgr.schemas[i].tableName;
        ss << ctgMgr.schemas[i].column.size();
        ss << ctgMgr.schemas[i].primaryKey;
        ss << ctgMgr.schemas[i].recordSize;

        int columnNum = ctgMgr.schemas[i].column.size();
        for(int j = 0; j < columnNum; j++)
        {
            ss << ctgMgr.schemas[i].column[i].attrName;
            ss << ctgMgr.schemas[i].column[i].type;
            ss << ctgMgr.schemas[i].column[i].isUnique;
            ss << ctgMgr.schemas[i].column[i].indexRootAddr;
        }
    }

    int blockCNT = 2;
    while(!ss.eof())
    {
        ss.read(writeBuff, BufferManager::pageSize);
        node schemaBlock = blockCNT < blockNum ? bufMgr.getBlock(cataFileAddr, blockCNT) : bufMgr.getNextFree(cataFileAddr);
        schemaBlock.write(writeBuff, 0, BufferManager::pageSize);
        blockCNT++;
    }
}

int CatalogManager::addSchema(std::string tableName, std::vector<attribute>& column, int primaryKey)
{
    int recordSize = 0;
    schema tmpSchema;
    tmpSchema.tableName = tableName;
    tmpSchema.column = column;
    tmpSchema.primaryKey = primaryKey;

    for(auto & it : column)
        recordSize += it.type <= 1 ? 4 : it.type;
    
    tmpSchema.fileAddr = bufMgr.openFile(tableName, DATA, recordSize);
    ctgMgr.schemas.push_back(tmpSchema);
    ctgMgr.schemaIndex[tableName] = ctgMgr.schemas.size() - 1;
}

int CatalogManager::dropSchema(std::string& tableName)
{
    if(ctgMgr.schemaIndex.find(tableName) == ctgMgr.schemaIndex.end())
        return FAILURE;
    ctgMgr.schemas.erase(ctgMgr.schemas.begin() + ctgMgr.schemaIndex[tableName]);
    ctgMgr.schemaIndex.erase(tableName);
}

std::vector<attribute>& CatalogManager::getColumn(std::string& scheName)
{
    if(ctgMgr.schemaIndex.find(scheName) == ctgMgr.schemaIndex.end())
        return ;
    return ctgMgr.schemas[ctgMgr.schemaIndex[scheName]].column;
}


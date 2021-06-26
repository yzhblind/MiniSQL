#pragma once
#include "BufferManager.hpp"
#include "Type.hpp"
class CatalogManager
{
private:
    int cataFileAddr;
    std::vector<schema> schemas;
    std::map<std::string, int> schemaIndex;
public:
    //CatalogManager();
    //~CatalogManager();
    int startCata();
    int endCata();
    int addSchema(std::string tableName, std::vector<attribute>& column, int primaryKey);
    int dropSchema(std::string& tableName);
    std::vector<attribute>& getColumn(std::string& scheName);
    // todo
    // recordSize
    // string fileAddress column;
    bool findSchema(std::string& tableName);
    bool findSchemaColumn(std::string& tableName, std::string& attrName);
    int getRecordSize(std::string& tableName);
    int getColumnAddr(std::string& tableName, std::string& attrName);

};

extern CatalogManager ctgMgr;
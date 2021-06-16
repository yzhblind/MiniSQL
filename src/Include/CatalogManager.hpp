#pragma once

#include "Type.hpp"
class CatalogManager
{
private:
    int cataFileAddr;
    std::vector<schema> schemas;
    std::map<std::string, int> schemaIndex;
public:
    CatalogManager();
    ~CatalogManager();
    int startCata();
    int endCata();
    int addSchema(std::string tableName, std::vector<attribute>& column, int primaryKey);
    int dropSchema(std::string& tableName);
    std::vector<attribute>& getColumn(std::string& scheName);
    // todo
    // recordSize
    // string fileAddress column;
    getRecordSize(std::string& tableName);
    getColumnAddr(std::string& tableName, std::string& attrName);
};

extern CatalogManager ctgMgr;
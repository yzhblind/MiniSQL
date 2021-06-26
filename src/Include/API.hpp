#pragma once

#include "RecordManager.hpp"
#include "Type.hpp"
#include "CatalogManager.hpp"
#include "BufferManager.hpp"

void SQL_create_table(schema news);
void SQL_create_index(std::string indexName, std::string tableName, std::string attrName);
void SQL_drop_table(std::string tableName);
void SQL_drop_index(std::string indexName);
void SQL_select_all(std::string tableName);
void SQL_select_cond(std::string tableName, std::vector<condExpr> condition);
void SQL_insert(std::string tableName, std::vector<element> list);
void SQL_delete_all(std::string tableName);
void SQL_delete_cond(std::string tableName, std::vector<condExpr> condition);

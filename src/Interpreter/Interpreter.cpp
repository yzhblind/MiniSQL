#include <iostream>
#include <fstream>
#include <cstring>
#include "interpreter.hpp"
#include "Type.hpp"					//create
#include "CatalogManager.hpp"		//insert
#include "RecordManager.hpp"		//select

using namespace std;

inst_reader::inst_reader() {
	char tmp;
	cout << "Please give your order after '>'" << endl;
	cout << ">";
	while ((tmp = cin.get()) != ';') {
		if (tmp == '\n')cout << ">";
		i.put(tmp);
	}
	return;
}

inst_reader::inst_reader(int mode) {
	char tmp;
	//cout << "Please give your order after '>'" << endl;
	//cout << ">";
	while ((tmp = cin.get()) != ';') {
		//if (tmp == '\n')cout << ">";
		i.put(tmp);
	}
	return;
}

inst_reader::~inst_reader() {
	string().swap(buff);				//swap释放buff内存
	buff.clear();
	i.clear();
	i.str("");							//释放sstream内存
	return;
}

void inst_reader::reread() {
	char tmp;
	while ((tmp = cin.get()) != ';') {
		if (tmp == '\n')cout << ">";
		i.put(tmp);
	}
	return;
}

void inst_reader::reread(int mode) {
	char tmp;
	while ((tmp = cin.get()) != ';') {
		//if (tmp == '\n')cout << ">";
		i.put(tmp);
	}
	return;
}

void inst_reader::clear() {
	string().swap(buff);				//swap释放buff内存
	buff.clear();
	i.clear();
	i.str("");							//释放sstream内存
	return;
}

int scan(stringstream& i, char& tmp) {
	while ((tmp = i.get()) != '(') {
		switch (tmp) {
		case ' ':break;
		case '\n':break;
		default:
			cout << "Error: wrong format for table's name" << endl;
			return -1;
		}
	}
	return 0;
}

int check_comp(string str, string name) {
	if (str == "" || str.find(',') != str.npos) {
		cout << "Error: wrong format for " << name << "'s name!" << endl;
		return -1;
	}
	return 0;
}

int get_name(stringstream& i, string& name, string form, char div) {
	int worddiv = 0;
	char tmp;
	while ((tmp = i.get()) != div && tmp != -1) {
		switch (tmp) {
		case ',':
			cout << "Error: wrong format for " << form << "'s name" << endl;
			return -1;
		case ' ':
		case '\n':
			if (worddiv == 0)break;
			else if (worddiv == 1) {
				worddiv = 2; break;
			}
			else break;
		default:
			if (worddiv == 0) {
				worddiv = 1;
				name += tmp;
			}
			if (worddiv == 2) {
				cout << "Error: wrong format for " << form << "'s name" << endl;
				return -1;
			}
		}
	}
	if (name == "") {
		cout << "Error: lack of " << form << "'s name" << endl;
		return -1;
	}
	return 0;
}

int get_val_type(string val) {
	if (val.size() == 0)return -1;
	char p;
	int flo = 0, str = 0;
	int vi, error = -1, cnt = 0;
	for (vi = 0; vi < val.size(); vi++) {
		p = val[vi];
		switch (p) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case '.':
			flo = 1;
			break;
		case '\'':
			str = 1;
			cnt++;
			break;
		default:
			break;
		}
	}

	if (cnt >= 1 && (val[0] != '\'' || val[val.size() - 1] != '\''))return error;//str类型：当且仅当头尾都是单引号时成立,允许含单引号
	if (str && val.size() > 256)return error;	//str类型：数据过长
	if (str)return val.size() + 1;		//str类型：返回大小,含'\0'
	if (flo)return 1;					//float类型
	return 0;							//int类型
}


int inst_reader::translate() {
	string keywd1, keywd2, str, check;
	int res;
	i >> keywd1;
	switch (res = trans.count(keywd1) ? trans.at(keywd1) : -1) {	//根据第一个关键词分类
	case creat:
		i >> keywd2;
		switch (res = trans.count(keywd2) ? trans.at(keywd2) : -1) {//create下分成建表，建索引两类
		case table: {											//建表
			struct schema news;
			struct attribute tmp_att;
			i >> str;
			if (check_comp(str, "table"))return -1;
			else {												//检查和记录各列的名称，类型，主键
				news.tableName = str;	//表名
				char tmp;
				if (scan(i, tmp) == -1)return -1;
				stringstream ss,line;
				while ((tmp = i.get()) != -1) {					//化去括号
					if (tmp == '(' || tmp == ')') {
						ss << " ";
					}
					else ss << tmp;
				}
				while ((tmp = ss.get()) != -1) {
					if (tmp == ',') {
						line >> str;
						if (str != "int" || str != "char" || str != "float") {
							cout << "Error: illegal format for attribute to be created" << endl;
						}
						tmp_att.attrName = str;
						tmp_att.isUnique = false;
						line >> str;
						switch (trans.count(str) ? trans.at(str) : -1) {
						case int_type:
							tmp_att.type = 0;
							line >> str;
							if (str == "unique") {
								tmp_att.isUnique = true;
							}
							else if (str != "") {
								cout << "Error: illegal type define for attribute" << endl;
								return -1;
							}
							break;
						case flt_type:
							tmp_att.type = 1;
							line >> str;
							if (str == "unique") {
								tmp_att.isUnique = true;
							}
							else if (str != "") {
								cout << "Error: illegal type define for attribute" << endl;
								return -1;
							}
						case str_type: {
							int len;
							line >> len;
							if (len > 255 || len < 1) {
								cout << "Error: illegal length define for attribute" << endl;
								return -1;
							}
							else {
								tmp_att.type = len + 1;
							}
							line >> str;
							if (str == "unique") {
								tmp_att.isUnique = true;
							}
							else if (str != "") {
								cout << "Error: illegal type define for attribute" << endl;
								return -1;
							}
						}
						default:
							cout << "Error: illegal type define for attribute" << endl;
							return -1;
						}

						news.column.push_back(tmp_att);

						line.clear();
						line.str("");
					}
					else line << tmp;
				}
				cout << "crt table " << str << endl;			//测试用输出语句
			}
			//SQL_create(news);
			break; 
		}
		case index: {											//create index
			char tmp;
			string indexName, tableName, attrName;
			i >> indexName;		//要创建的索引名称
			if (check_comp(indexName, "index"))return -1;
			if (indexName == "on") {
				cout << "Error: lack of indexName" << endl;
				return -1;
			}
			i >> check;
			if (check != "on") {
				cout << "Error: wrong format for create order" << endl;
				return -1;
			}
			int worddiv = 0;
			if (get_name(i, tableName, "table", '('))return -1;
			if (get_name(i, attrName, "attribute", ')'))return -1;

			//SQL_create(index, indexName, tableName, attrName);

			cout << "crt index " << indexName << ' ' << "on" << tableName << '(' << attrName << ')' << endl;
			break;
		}
		default:
			cout << "Error: no such order\n";
		}
		break;
	case drop:
		i >> keywd2;
		switch (res = trans.count(keywd2) ? trans.at(keywd2) : -1) {
		case table:												//drop table
			i >> str;			//str此处指代tableName
			i >> check;			//检查是否有多余字符串
			if (check != "" || str.find(',') != str.npos || str == "") {		//table名只能是单个字符串，逗号将被视为分隔符（即使没有空格）
				cout << "Error: wrong format for table's name" << endl;
				return -1;
			}
			else {
				//SQL_drop(table, tablename);
				cout << "drop table " << str << endl;
			}
			break;
		case index:												//drop index
			i >> str;			//str此处指代indexName
			i >> check;			//检查是否有多余字符串
			if (check != "" || str.find(',') != str.npos || str == "") {
				cout << "Error: wrong format for index's name" << endl;
				return -1;
			}
			else {
				//SQL_drop(index, indexname);
				cout << "drop index " << str << endl;
			}
			break;
		default:
			cout << "Error: no such order\n";
		}
		break;
	case select: {
		string tableName;
		i >> str;
		if (str != "*") {
			cout << "Error: wrong format for select order" << endl;
			return -1;
		}
		i >> str;
		if (str != "from") {
			cout << "Error: wrong format for select order" << endl;
			return -1;
		}
		i >> tableName;
		if (check_comp(tableName, "table"))return -1;
		if (ctgMgr.findSchema(tableName) == false) {
			cout << "Error: such table does not exist" << endl;
			return -1;
		}
		i >> str;
		if (str == "") {
			//SQL_select(tableName);
		}
		else if (str == "where") {
			stringstream ss;
			string attrName, val;
			int tmpOP, pos, type;
			int int_val;
			float flo_val;
			string str_val;
			//vector<attribute>attr;
			vector<condExpr>conditions;
			int num_cond = 0, stt = 0;
			const std::vector<attribute, std::allocator<attribute>>attr = ctgMgr.getColumn(tableName);
			i >> str;
			while (str != "") {
				if (stt == 3 && str == "and") {
					i >> str; stt = 0;
				}
				else if (stt == 0 && str != "and") {
					if (ctgMgr.findSchemaColumn(tableName, str) == false) {
						cout << "Error: wrong attribute for table " << tableName << endl;
						return -1;
					}
					else {
						attrName = str;
						pos = ctgMgr.getColumnAddr(tableName, attrName);
						i >> str;
						stt = 1;
					}
				}
				else if (stt == 1) {
					if (str.size() > 2 || !optrans.count(str)) {
						cout << "Error: unknown operator" << endl;
						return -1;
					}
					else {
						tmpOP = optrans.at(str);
						i >> str;
						stt = 2;
					}
				}
				else if (stt == 2 && str != "and") {
					type = get_val_type(str);
					if (type == -1) {
						cout << "Error: wrong type of value" << endl;
						return -1;
					}
					switch (type) {
					case 0: {
						ss << str;
						ss >> int_val;
						//condExpr c(attr, pos, tmpOP, &int_val);
						//conditions.push_back(c);
						break;
					}
					case 1:
						ss << str;
						ss >> flo_val;
						//condExpr c(attr, pos, tmpOP, &flo_val);
						//conditions.push_back(c);
						break;
					default:
						str_val = str.substr(1, str.size() - 2);
						//condExpr c(attr, pos, tmpOP, &str_val);
						//conditions.push_back(c);
					}
					ss.str("");
					ss.clear();
				}
			}
			//SQL_select(tableName, conditions);
		}
		else {
			cout << "Error: wrong format for select order" << endl;
			return -1;
		}
		break;
	}
	case insert: {
		char tmp;
		int type, num = 0;
		int int_val;
		float flo_val;
		string str_val;
		string val;
		vector<element>list;
		string tableName;
		i >> str;
		if (str != "into") {
			cout << "Error: no such order" << endl;
			return -1;
		}
		i >> tableName;
		if (check_comp(tableName, "table"))return -1;
		if (ctgMgr.findSchema(tableName) == false) {
			cout << "Error: such table does not exist" << endl;
			return -1;
		}
		const std::vector<attribute, std::allocator<attribute>>attr = ctgMgr.getColumn(tableName);
		i >> str;
		if (str != "values") {
			cout << "Error: no such order" << endl;
			return -1;
		}
		if (scan(i, tmp) == -1)return -1;
		stringstream ss, tr;
		while ((tmp = i.get()) != ')') {
			if (tmp == ',') {
				ss << " ";
			}
			else ss << tmp;
		}
		ss >> val;
		while (val != "") {
			type = get_val_type(val);
			if (type == -1) {
				cout << "Error: wrong type of value" << endl;
				return -1;
			}
			if (type != attr[num].type) {
				cout << "Error: given value's type is not consistent with the table's" << endl;
				return -1;
			}
			switch (type) {
			case 0:
				tr << val;
				tr >> int_val;
				list.push_back({ type, &int_val });
				break;
			case 1:
				tr << val;
				tr >> flo_val;
				list.push_back({ type, &flo_val });
				break;
			default:
				str_val = str.substr(1, str.size() - 2);
				list.push_back({ type, &str_val });
			}
			tr.clear();
			tr.str("");
			num++;
			ss >> val;
		}
		//SQL_insert(tableName, list);
		break;
	}
	case dlt: {
		string tableName;
		i >> str;
		if (str != "from") {
			cout << "Error: no such order" << endl;
			return -1;
		}
		i >> tableName;
		if (check_comp(tableName, "table"))return -1;
		if (ctgMgr.findSchema(tableName) == false) {
			cout << "Error: such table does not exist" << endl;
			return -1;
		}
		i >> str;
		if (str == "") {
			//SQL_delete(tableName);
		}
		else if( str == "where" ){
			stringstream ss;
			string attrName, val;
			int tmpOP, pos, type;
			int int_val;
			float flo_val;
			string str_val;
			//vector<attribute>attr;
			vector<condExpr>conditions;
			int num_cond = 0, stt = 0;
			const std::vector<attribute,std::allocator<attribute>>attr = ctgMgr.getColumn(tableName);
			i >> str;
			while (str != "") {
				if (stt == 3 && str == "and") {
					i >> str; stt = 0;
				}
				else if(stt == 0 && str != "and"){
					if (ctgMgr.findSchemaColumn(tableName, str) == false) {
						cout << "Error: wrong attribute for table " << tableName << endl;
						return -1;
					}
					else {
						attrName = str;
						pos = ctgMgr.getColumnAddr(tableName, attrName);
						i >> str;
						stt = 1;
					}
				}
				else if (stt == 1 && str != "and") {
					if (str.size() > 2 || !optrans.count(str)) {
						cout << "Error: unknown operator" << endl;
						return -1;
					}
					else {
						tmpOP = optrans.at(str);
						i >> str;
						stt = 2;
					}
				}
				else if (stt == 2 && str != "and") {
					stt = 3;
					type = get_val_type(str);
					if (type == -1) {
						cout << "Error: wrong type of value" << endl;
						return -1;
					}
					switch (type) {
					case 0: {
						ss << str;
						ss >> int_val;
						//condExpr c(attr, pos, tmpOP, &int_val);
						//conditions.push_back(c);
						break; 
					}
					case 1:
						ss << str;
						ss >> flo_val;
						//condExpr c(attr, pos, tmpOP, &flo_val);
						//conditions.push_back(c);
						break;
					default:
						str_val = str.substr(1, str.size() - 2);
						//condExpr c(attr, pos, tmpOP, &str_val);
						//conditions.push_back(c);
					}
					ss.str("");
					ss.clear();
					i >> str;
				}
				else {
					cout << "Error: wrong format for delete order" << endl;
					return -1;
				}
			}
			//SQL_delete(tableName, conditions);
		}
		else {
			cout << "Error: wrong format for delete order" << endl;
			return -1;
		}
		break; 
	}
	case quit:
		break;
	case exec: {
		//cin重定向
		//cout << "Please input the name of SQL file:";
		string filename;
		i >> filename;
		//打开文件，等待读取
		ifstream fin(filename);
		
		//从文件读入
		if (fin) {
			cout << "result:" << endl;
			streambuf* oldcin;
			//用 rdbuf() 重新定向，返回旧输入流缓冲区指针
			oldcin = cin.rdbuf(fin.rdbuf());
			inst_reader m(1);
			while (m.translate() != quit || m.i.str() == "") {
				m.clear();
				m.reread(1);
			}
			res = quit;
			cin.rdbuf(oldcin); // 恢复键盘输入
			//打开的文件，最终需要手动关闭
			fin.close();
		}
		else cout << "Error: cannot open the file" << endl;
		break;
	}
	default:
		cout << "Error: no such order" << endl;
	}
	return res;
};

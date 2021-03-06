#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include "CatalogManager.hpp"
using namespace std;

enum inst_code {
	creat,
	drop,
	slct,
	insert,
	dlt,
	quit,
	exec,
	table,
	indx,
	int_type,
	flt_type,
	str_type,
	prmy_key
};

const static unordered_map<string, int> trans{
	{"create",	creat	},
	{"drop",	drop	},
	{"select",	slct	},
	{"insert",	insert	},
	{"delete",	dlt		},
	{"quit",	quit	},
	{"execfile",exec	},
	{"table",	table	},
	{"index",	indx	},
	{"int",		int_type},
	{"float",	flt_type},
	{"char",	str_type},
	{"key",		prmy_key}
};

const static unordered_map<string, int> optrans{
	{"=",		E	},
	{"<>",		NE	},
	{"<",		L	},
	{">",		G	},
	{"<=",		LE	},
	{">=",		GE	}
};


class inst_reader {
private:
	string buff;

public:
	stringstream i;
	inst_reader();
	inst_reader(int mode);
	void reread();
	void reread(int mode);
	void clear();
	~inst_reader();
	int translate();

};
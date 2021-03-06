#include "Interpreter.hpp"
#include "BufferManager.hpp"
#include "CatalogManager.hpp"
#include <iostream>

int main() {
	ctgMgr.startCata();
	inst_reader in;
	int res;
	while ((res = in.translate()) != quit) {
		std::cout << ">";
		in.clear();
		in.reread();
	}
	ctgMgr.endCata();
	return 0;
}

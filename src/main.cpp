#include "Interpreter.hpp"
#include "BufferManager.hpp"
#include "CatalogManager.hpp"

int main() {
	ctgMgr.startCata();
	inst_reader in;
	while (in.translate() != quit) {
		in.clear();
		in.reread();
	}
	ctgMgr.endCata();
	return 0;
}

#include "Interpreter.hpp"


int main() {
	inst_reader in;
	ctgMgr.startCata();
	while (in.translate() != quit) {
		in.clear();
		in.reread();
	}
	ctgMgr.endCata();
	return 0;
}

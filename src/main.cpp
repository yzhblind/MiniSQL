#include "interpreter.hpp"


int main() {
	inst_reader in;
	while (in.translate() != quit) {
		in.clear();
		in.reread();
	}
	return 0;
}

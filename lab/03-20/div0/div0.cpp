// #include <libce.h> // fa da se'
#include <iostream>
using namespace std;

extern "C" void a_div0();

int main() {
	// mettiamoci un gestore
	gate_init(0, a_div0);

	natl a = 5;
	a /= 0; // ahia!

	printf("Ho ottenuto: %d\n", a);
	return 0;
}

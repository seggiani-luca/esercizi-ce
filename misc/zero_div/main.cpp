#include <libce.h>

extern "C" void c_divzero(natq rip) {
	printf("E' successo qualcosa di brutto a %lx\n", rip);
}
extern "C" void a_divzero();

int main() {
	// imposta interruzione per fault divisione
	gate_init(0, a_divzero);

	volatile int a = 3;
	a /= 0; // male

	return 0;
}

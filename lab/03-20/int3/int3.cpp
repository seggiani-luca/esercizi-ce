#include <libce.h>

void foo() { // qui ci vogliamo mettere un breakpoint
	printf("foo e' in esecuzione\n");
}

// la funzione che fa da breakpoint
extern "C" void c_break(void* p) {
	printf("il debugger ha interrotto foo con rip: %d\n", p);
	pause();
}
extern "C" void a_break();

// la funzione che inserisce il breakpoint
extern "C" void a_add_break(void(*)()); // puntatore a func. generico

int main() {
	// imposta l'handler di int3
	gate_init(3, a_break);

	// metti un breakpoint in foo
	a_add_break(foo);

	// esegui foo
	foo();

	pause();
	return 0;
}

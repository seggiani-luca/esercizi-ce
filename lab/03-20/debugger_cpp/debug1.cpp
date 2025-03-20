#include <libce.h>

char saved_byte;

void foo(){
    printf("Ciao sono la funzione foo\n");
}

extern "C" void a_debug();
extern "C" void c_debug(void** p){
	// questo interrompe
	// p contiene il puntatore a %rsp
	
	// intanto dimmi qualcosa
	printf("il debugger ha interrotto foo a rip: %d\n", p);
	pause();

	// poi rimetti tutto a posto
	auto func_p = reinterpret_cast<char**>(p);
	--(*func_p); // e' l'istruzione precedente
	**func_p	= saved_byte;
}


void add_breakpoint(void (*func)(void)) {
	// questo mette il breakpoint
	
	// salvati saved_byte
	auto func_p = reinterpret_cast<char*>(func);
	saved_byte = *func_p;
	printf("saved_byte: %x\n", saved_byte);

	// al suo posto mettici 0xcc (int3)
	*func_p = 0xcc;
}

extern "C" void main(){
    gate_init(3, a_debug);
    add_breakpoint(foo);
    foo();
    // foo();
    pause();
}

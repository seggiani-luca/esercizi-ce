#include <libce.h>

char saved_byte;
char* saved_byte_addr;

void foo() {
	printf("foo e' in esecuzione\n\n");
}

extern "C" void a_debug(); // handler interruzione int3
extern "C" void c_debug(void** p) {
	// questo interrompe
	// p contiene il puntatore a %rsp
	
	// *p e' rip, decrementalo (vuoi ripartire da rip - 1, e questo sara' 
	// l'indirizzo salvato nello stack che iretq andra' a riprenersi)
	(*p)--;
	
	// prendilo come indirizzo del byte salvato
	saved_byte_addr = reinterpret_cast<char*>(*p);

	// intanto dimmi qualcosa
	printf("il debugger ha interrotto foo a rip: %p\n", *p);
	
	printf("salvato l'indirizzo del byte d'istruzione: %p\n", saved_byte_addr);
	pause();

	// poi rimetti tutto a posto
	*saved_byte_addr = saved_byte;
}

extern "C" void a_sstep(); // handler single step
extern "C" void c_sstep() {
	// questo interrompe in single step
	// saved_byte_conterra' il byte che abbiamo reinserito
	
	// rimettiamoci 0xcc (int3)
	*saved_byte_addr = 0xcc;
}

void add_breakpoint(void (*func)(void)) {
	// questo mette il breakpoint
	
	// salvati saved_byte
	auto func_p = reinterpret_cast<char*>(func);
	saved_byte = *func_p;
	printf("salvato il byte d'istruzione: %x\n", saved_byte);

	// al suo posto mettici 0xcc (int3)
	*func_p = 0xcc;
}

extern "C" void main(){
	gate_init(3, a_debug); // gate per l'interruzione int3
	gate_init(1, a_sstep); // gate per il single step

	add_breakpoint(foo);

	printf("lancio foo...\n");
	foo();
	
	printf("lancio foo...\n");
	foo();
	
	pause();
}

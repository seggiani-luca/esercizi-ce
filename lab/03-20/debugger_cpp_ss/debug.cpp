#include <libce.h>

char saved_byte;
char* addr; // bruttino in verita'

void foo(){
    printf("Ciao sono la funzione foo\n");
}

extern "C" void a_debug();
extern "C" void c_debug(void** p){
    char* rip = (char*)(*p);
    printf("DEBUG RIP=%p\n", rip);
    *(rip-1) = saved_byte;
    (*p) = rip - 1;
		addr = rip - 1;
		printf("addr is=%p\n", addr);
		pause();
}

extern "C" void a_single_step();
extern "C" void c_single_step(){
	*((char*)addr) = 0xcc;
}


void add_breakpoint(void (*func)(void)) {
    printf("func=%p\n", func);
    saved_byte = *((char*)func);
    *((char*)func) = 0xcc;
}

extern "C" void main(){
    gate_init(3, a_debug); // gestisce la int3
    gate_init(1, a_single_step); // gestisce il single step
    add_breakpoint(foo);
    foo();
    foo();
    pause();
}

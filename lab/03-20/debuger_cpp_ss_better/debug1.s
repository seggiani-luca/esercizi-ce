#include "libce.h"

.global a_debug, a_sstep
.extern c_debug, c_sstep

a_debug: // handler interruzione int3
	salva_registri
	
	leaq 120(%rsp), %rdi // fornisci il vecchio rip in argomento
	call c_debug

	carica_registri
  
	orw $0x100, 16(%rsp) // attiva la single step in eflags
											 // dovrebbe essere:
											 // $rsp		vecchio rip
											 // $rsp+8	vecchio cs
											 // $rsp+16	vecchio rflags
											 // a questo punto TF e' a 0x100 in rflags
    
	iretq

a_sstep: // handler single step
	salva_registri

	call c_sstep

	carica_registri

	andw $0xFEFF, 16(%rsp) // disattiva la single step in eflags
												 // come sopra, la maschera e' complementare

	iretq

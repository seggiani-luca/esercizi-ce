.global a_divzero
a_divzero:
	// l'IP è in cima alla pila
	mov (%rsp), %rdi
	call c_divzero
	iretq

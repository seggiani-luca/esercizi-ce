.global a_div0

a_div0: // l'handler per l'eccezione 0
	movq $0, %rax // metti che fa 0
	addq $2, (%rsp) // riparti da ip + 2 (dimensione istruzione div)
	iretq // con lui solo, visto che è di tipo fault, cicla all'infinito 

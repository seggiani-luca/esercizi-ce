#include <libce.h>

	.global a_tastiera
a_tastiera:
	salva_registri
	call c_tastiera
	carica_registri
	iretq

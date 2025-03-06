#include <libce.h>

	.global a_tastiera
a_tastiera:
	salva_registri
	call c_tastiera
	carica_registri
	iretq

	.global a_timer
a_timer:
	salva_registri
	call c_timer
	carica_registri
	iretq

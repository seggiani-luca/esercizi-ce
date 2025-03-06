#include <libce.h>

	.global a_timer
a_timer:
	salva_registri
	call c_timer
	carica_registri
	iretq

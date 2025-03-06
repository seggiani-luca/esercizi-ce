#include <libce.h>
.text
.extern	c_bmide
.global a_bmide
a_bmide:
	salva_registri
	call c_bmide
	carica_registri
	iretq

.data
.balign 4
.global prd
prd:
	.fill 16384, 4
.balign 65536
.global vv
vv:
	.fill 65536, 1

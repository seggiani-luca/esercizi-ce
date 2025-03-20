#include <libce.h>

.global a_break // io
.extern c_break // quella in c++

.global a_add_break

a_break:
	salva_registri // macro per salvare registri

	movq 120(%rsp), %rdi // passiamo quello che c'era prima dei 120 byte di registri
											 // salvati alla c_break, cioe' l'istruzione dopo (tipo fault)
	call c_break

	decq 120(%rsp) // rsp punta al codice, prendi l'istruzione prima
	movq 120(%rsp), %rax // ora %rax punta al codice
	
	mov saved_byte, %bl // argh e' memoria memoria
	movb %bl, (%rax) // ora hai sistemato il programma col saved_byte
	
	carica_registri // macro per caricare registri
	iretq // buona fortuna

a_add_break:
	movb (%rdi), %al // %rdi contiene il * a func.
	movb %al, saved_byte
	movb $0xcc, (%rdi) // ci metti int3
	ret

.data
	saved_byte: .byte 0 // sara' l'istruzione che modificheremo

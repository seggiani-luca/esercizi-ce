	.global _ZN2cl5elab1ER3st1R3st2
	.extern _ZN2clC1Ec3st2

	.set _v, 8
_ZN2cl5elab1ER3st1R3st2:
	// prologo
	pushq %rbp
	movq %rsp, %rbp

	// args:
	// %rdi: this
	// %rsi: addr s1 (char[4])
	// %rdx: addr s2 (int[4])

	// riserva stack
	subq $48, %rsp

	// costruzione classe cla
	// args:
	// %rdi: nuovo this
	// %sil: $'a'
	// %rdx: 0, 1 di s2 (int[4])
	// %rcx: 2, 3 di s2 (int[4])

	pushq %rdi
	lea -40(%rbp), %rdi

	pushq %rsi
	movb $'a', %sil

	movq %rdx, %r8
	movq (%r8), %rdx
	addq $8, %r8
	movq (%r8), %rcx

	call _ZN2clC1Ec3st2

	// -40(%rbp) contiene la classe

	popq %rsi

	popq %rdi

	// prepara il loop
	lea -40(%rbp), %r8
	movq $0, %r9

loop:
	// s1.vc[i]
	movb (%rsi, %r9, 1), %r10b
	// s.vc[i]
	movb (%rdi, %r9, 1), %r11b

	cmpb %r10b, %r11b
	jae ciclo

	movb (%r8, %r9, 1), %r10b
	movb %r10b, (%rdi, %r9, 1)

	movq _v(%r8, %r9, 8), %r10
	movq %r10, _v(%rdi, %r9, 8)

ciclo:
	inc %r9
	cmp $4, %r9
	jb loop

	// libera stack
	add $48, %rsp

	// epilogo
	pop %rbp
	ret

// void cl::elab1(st1& s1, st2& s2) {
// 	cl cla('a', s2);
// 	for (int i = 0; i < 4; i++) {
// 		if (s.vc[i] < s1.vc[i]) {
// 			s.vc[i] = cla.s.vc[i];
// 			v[i] = cla.v[i];
// 		}
// 	}
// }

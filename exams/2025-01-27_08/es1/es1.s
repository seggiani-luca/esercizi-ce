	.global _ZN2cl5elab1ER3st13st2
	.extern _ZN2clC1EPc3st2
_ZN2cl5elab1ER3st13st2:
	// prologo
	pushq %rbp
	movq %rsp, %rbp

	// 40 byte per cla
	subq $48, %rsp 
	
	// argomenti:
	// %rdi è il puntatore this
	// %rsi è un riferimento a s1
	// %rdx contiene 0, 1 di s2
	// %rcx contiene 2, 3 di s2

	// primo: crea una classe cla usando s1.vc e s2
	// il costruttore è _ZN2clC1EPc3st2
	// vuole this in %rdi, s1.vc (quindi s1) in %rsi e s2 in %rdx

	pushq %rdi

	// fai spazio per la classe
	// composta come:
	// 4 byte di s (char vc[4])
	// 4 byte di padding
	// 8 * 4 byte di long v[4]
	// tot. 40 byte
	
	lea -40(%rbp), %rdi
	// s1 è gia in %rsi
	// s2 è gia %rdc e %rcx

	call _ZN2clC1EPc3st2

	//da %rbp - 40 in poi dovrebbe esserci la classe
	// mi aspetto:
	// x y z w 
	// 130 123 152 123

	popq %rdi

	// loop con r8
	movq $0, %r8

	// prepearti al loop
	lea -40(%rbp), %r11

loop:
	// s1.vc
	movb (%rsi, %r8, 1), %r9b
	// s.vc
	movb (%rdi, %r8, 1), %r10b

	cmpb %r9b, %r10b
	jae ciclo
	
	// prima cla.vc
	movb (%r11, %r8, 1), %r9b
	movb %r9b, (%rdi, %r8, 1)

	// poi cla.v
	addq $8, %rdi
	addq $8, %r11
	
	movq (%r11, %r8, 8), %r9
	addq %r9, (%rdi, %r8, 8)

	subq $8, %rdi
	subq $8, %r11

ciclo:
	incq %r8
	cmpq $4, %r8
	jb loop
	
	// rendi i 40 byte di cla
	addq $48, %rsp
	
	// epilogo
	popq %rbp
	ret

// sarebbe:
// void cl::elab1(st1& s1, st2 s2)
// {
// 	cl cla(s1.vc, s2);
// 	for (int i = 0; i < 4; i++) {
// 		if (s.vc[i] < s1.vc[i]) {
// 			s.vc[i] = cla.s.vc[i];
// 			v[i] += cla.v[i];
// 		}
// 	}
// }

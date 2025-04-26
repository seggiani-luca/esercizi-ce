	.global _ZN2cl5elab1ER2sti
_ZN2cl5elab1ER2sti:
	// prologo
	pushq %rbp
	movq %rsp, %rbp

	// args
	// %rdi: this
	// %rsi: ss
	// %rdx: d

	// classe
	// char vv1[4]	1 + 4 byte
	// padding    	4 byte 
	// long vv2[4]	8 * 4 byte

	.set vv1, 0
	.set vv2, 8

	// prepara loop
	movq $0, %rcx

loop:
	movq %rdx, vv2(%rdi, %rcx, 8)
	addq %rcx, vv2(%rdi, %rcx, 8)

	cmpq vv2(%rsi, %rcx, 8), %rdx
	jae ciclo

	movb vv1(%rsi, %rcx, 1), %r8b
	addb %r8b, vv1(%rdi, %rcx, 1)

ciclo:
	incq %rcx
	cmpq $4, %rcx
	jb loop

	// epilogo
	popq %rbp
	ret

// aspettati
// 13 12 11 10 	13 12 11 10 

// 13 12 14 14 	2 3 4 5 

// void cl::elab1(st& ss, int d)
// {	
// 	for (int i = 0; i < 4; i++) {
// 		if (d < ss.vv2[i])
// 			s.vv1[i] += ss.vv1[i]; 
// 	  	s.vv2[i] = d + i;
// 	}
// }

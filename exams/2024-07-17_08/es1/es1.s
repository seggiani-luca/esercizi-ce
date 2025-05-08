.global _ZN2cl5elab1ER2sti 

.set vv2, 0
.set vv1, 32

_ZN2cl5elab1ER2sti:
	pushq %rbp
	movq %rsp, %rbp

	// this : %rdi
	// st& ss : addr. %rsi
	// int d : %rdx
	
	// prepara loop
	mov $0, %rcx

loop_head:
	movq %rdx, vv2(%rdi, %rcx, 8)
	addq %rcx, vv2(%rdi, %rcx, 8)

	cmpq vv2(%rsi, %rcx, 8), %rdx
	jbe loop_tail

	movb vv1(%rsi, %rcx, 1), %r8b
	addb %r8b, vv1(%rdi, %rcx, 1)

loop_tail:
	incq %rcx
	cmpq $4, %rcx
	jb loop_head

	popq %rbp
	ret

// void cl::elab1(st& ss, int d)
// {	
// 	for (int i = 0; i < 4; i++) {
// 		if (d > ss.vv2[i])
// 			s.vv1[i] += ss.vv1[i]; 
// 	  	s.vv2[i] = d + i;
// 	}
// }

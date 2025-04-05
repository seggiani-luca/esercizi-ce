	.extern _ZN2clC1EcR3st1
	.extern _ZN2cl6stampaEv
	.global _ZN2cl5elab1E3st1
_ZN2cl5elab1E3st1:
	pushq %rbp
	movq %rsp, %rbp

	subq $0x40, %rsp

	# %rdi contiene il puntatore this
	# %rsi contiene st1, cioe' vc

	# cl e' 32 bit di s + 64 * 4 bit di v ALLINEATI!
	# 32 bit | s
	# 32 bit | padding
	# 64 bit | v[0]
	# 64 bit | v[1]
	# 64 bit | v[2]
	# 64 bit | v[3]

	# crea cla
	push %rdi
	push %rsi

	mov %rbp, %rdi # this di cla
	sub $0x40, %rdi

	# metti il st1 in pila
	mov %rbp, %rbx
	sub $8, %rbx

	movl %esi, (%rbx)

	mov %rbx, %rdx # args
	mov $'k', %rsi
	
	call _ZN2clC1EcR3st1

	# stampa 
	call _ZN2cl6stampaEv

	pop %rsi
	pop %rdi

	mov $0, %rcx  # contatore

ciclo:
	# vogliamo s.vc <= s1.vc

	#   s.vc             s1.vc
	cmpb (%rdi, %rcx, 1), %sil
	jb step # ok

	# s.vc[i] = cla.s.vc[i]
	movb -0x40(%rbp, %rcx, 1), %bl
	movb %bl, (%rdi, %rcx, 1)
	
	# v[i] = i - cla.v[i]
	movq %rcx, %rbx
	sub -0x38(%rbp, %rcx, 8), %rbx
	mov %rbx, 8(%rdi, %rcx, 8)

step:
	shrq $8, %rsi

	inc %rcx # inc. contatore

	cmpq $4, %rcx
	jb ciclo

	addq $0x40, %rsp
	popq %rbp
	ret

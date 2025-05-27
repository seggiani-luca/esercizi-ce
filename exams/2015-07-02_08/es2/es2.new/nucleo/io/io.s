/// @file io.s
/// @brief Parte Assembler del modulo I/O
/// @addtogroup io
/// @{
#include <libce.h>
#include <costanti.h>

/// @addtogroup ioinit
/// @{

	.text
/// Entry point del modulo I/O
.global _start
_start:
	.cfi_startproc
	// chiamiamo eventuali costruttori di oggetti globali
	push %rdi
	call ctors
	pop %rdi
	// il resto dell'inizializzazione e' scritto in C++
	jmp main_io
	.cfi_endproc
/// @} 

/// @cond
////////////////////////////////////////////////////////////////////////////////
//                       CHIAMATE DI SISTEMA GENERICHE                        //
////////////////////////////////////////////////////////////////////////////////

	.global getmeminfo
getmeminfo:
	.cfi_startproc
	int $TIPO_GMI
	ret
	.cfi_endproc

	.global activate_p
activate_p:
	.cfi_startproc
	int $TIPO_A
	ret
	.cfi_endproc

	.global terminate_p
terminate_p:
	.cfi_startproc
	int $TIPO_T
	ret
	.cfi_endproc

	.global sem_ini
sem_ini:
	.cfi_startproc
	int $TIPO_SI
	ret
	.cfi_endproc

	.global sem_wait
sem_wait:
	.cfi_startproc
	int $TIPO_W
	ret
	.cfi_endproc

	.global sem_signal
sem_signal:
	.cfi_startproc
	int $TIPO_S
	ret
	.cfi_endproc

	.global delay
delay:
	.cfi_startproc
	int $TIPO_D
	ret
	.cfi_endproc

	.global do_log
do_log:
	.cfi_startproc
	int $TIPO_L
	ret
	.cfi_endproc

////////////////////////////////////////////////////////////////////////////////
//               CHIAMATE DI SISTEMA SPECIFICHE PER IL MODULO I/O             //
////////////////////////////////////////////////////////////////////////////////

	.global activate_pe
activate_pe:
	.cfi_startproc
	int $TIPO_APE
	ret
	.cfi_endproc

	.global wfi
wfi:
	.cfi_startproc
	int $TIPO_WFI
	ret
	.cfi_endproc

	.global abort_p
abort_p:
	.cfi_startproc
	int $TIPO_AB
	ret
	.cfi_endproc

	.global fill_gate
fill_gate:
	.cfi_startproc
	int $TIPO_FG
	ret
	.cfi_endproc

	.global trasforma
trasforma:
	.cfi_startproc
	int $TIPO_TRA
	ret
	.cfi_endproc

	.global access
access:
	.cfi_startproc
	int $TIPO_ACC
	ret
	.cfi_endproc

	.global phys_alloc
phys_alloc:
	.cfi_startproc
	int $TIPO_PA
	ret
	.cfi_endproc

	.global phys_dealloc
phys_dealloc:
	.cfi_startproc
	int $TIPO_PD
	ret
	.cfi_endproc
/// @endcond

/// @addtogroup ioinit
/// @{

/// @brief Associa una routine ad un gate della IDT
/// @param gate		numero del gate
/// @param off		indirizzo della routine
///
/// Chiama la primitiva fill_gate() e abortisce il processo
/// se la primitiva restituisce un errore.
/// In caso di successo, il gate sarà in ogni caso accessibile da livello
/// utente.
.macro fill_io_gate gate off
	movq $\gate, %rdi
	movabs $\off, %rax
	movq %rax, %rsi
	movq $LIV_UTENTE, %rdx
	call fill_gate
	cmp $0, %al
	jne 1f
	call abort_p
1:
.endm

/// @cond
	.global fill_io_gates
fill_io_gates:
	.cfi_startproc
	pushq %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq %rsp, %rbp
	.cfi_def_cfa_register 6

	fill_io_gate	IO_TIPO_HDR	a_readhd_n
	fill_io_gate	IO_TIPO_HDW	a_writehd_n
	fill_io_gate	IO_TIPO_DMAHDR	a_dmareadhd_n
	fill_io_gate	IO_TIPO_DMAHDW	a_dmawritehd_n
	fill_io_gate	IO_TIPO_RCON	a_readconsole
	fill_io_gate	IO_TIPO_WCON	a_writeconsole
	fill_io_gate	IO_TIPO_INIC	a_iniconsole
	fill_io_gate	IO_TIPO_GMI	a_getiomeminfo

	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
/// @endcond
/// @}

/// @addtogroup consolesyscall
/// @{

/// Parte Assembler della primitiva readconsole()
	.extern c_readconsole
a_readconsole:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_readconsole
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva writeconsole()
	.extern c_writeconsole
a_writeconsole:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_writeconsole
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva iniconsole()
	.extern c_iniconsole
a_iniconsole:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_iniconsole
	iretq
	.cfi_endproc
/// @}

/// @addtogroup atasyscall
/// @{

/// Parte Assembler della primitiva readhd_n()
	.extern	c_readhd_n
a_readhd_n:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_readhd_n
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva writehd_n()
	.extern	c_writehd_n
a_writehd_n:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_writehd_n
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva dmareadhd_n()
	.extern	c_dmareadhd_n
a_dmareadhd_n:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_dmareadhd_n
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva dmawritehd_n()
	.extern	c_dmawritehd_n
a_dmawritehd_n:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_dmawritehd_n
	iretq
	.cfi_endproc
/// @}

/// @addtogroup iodbg
/// @{

/// Parte Assembler della primitiva getiomeminfo()
	.extern	c_getiomeminfo
a_getiomeminfo:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call c_getiomeminfo
	iretq
	.cfi_endproc
/// @}
/// @}

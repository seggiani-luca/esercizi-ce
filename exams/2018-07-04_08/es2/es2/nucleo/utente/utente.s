/// @file utente.s
/// @brief Parte Assembler del modulo utente
/// @addtogroup usr
/// @{
#include <costanti.h>

/// @addtogroup usrinit
/// @{

/// Entry point del modulo utente
	.global _start
_start:
	.cfi_startproc
	// chiamiamo eventuali costruttori di oggetti globali
	push %rdi
	call ctors
	pop %rdi
	jmp main
	.cfi_endproc
/// @}

/// @cond
	.text
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

	.global abort_p
abort_p:
	.cfi_startproc
	int $TIPO_AB
	ret
	.cfi_endproc

	.global do_log
do_log:
	.cfi_startproc
	int $TIPO_L
	ret
	.cfi_endproc

	.global getmeminfo
getmeminfo:
	.cfi_startproc
	int $TIPO_GMI
	ret
	.cfi_endproc

	.global readconsole
readconsole:
	.cfi_startproc
	int $IO_TIPO_RCON
	ret
	.cfi_endproc

	.global writeconsole
writeconsole:
	.cfi_startproc
	int $IO_TIPO_WCON
	ret
	.cfi_endproc

	.global iniconsole
iniconsole:
	.cfi_startproc
	int $IO_TIPO_INIC
	ret
	.cfi_endproc

	.global readhd_n
readhd_n:
	.cfi_startproc
	int $IO_TIPO_HDR
	ret
	.cfi_endproc

	.global writehd_n
writehd_n:
	.cfi_startproc
	int $IO_TIPO_HDW
	ret
	.cfi_endproc

	.global dmareadhd_n
dmareadhd_n:
	.cfi_startproc
	int $IO_TIPO_DMAHDR
	ret
	.cfi_endproc

	.global dmawritehd_n
dmawritehd_n:
	.cfi_startproc
	int $IO_TIPO_DMAHDW
	ret
	.cfi_endproc

	.global getiomeminfo
getiomeminfo:
	.cfi_startproc
	int $IO_TIPO_GMI
	ret
	.cfi_endproc

// ( ESAME 2018-07-04
	.global receive
receive:
	.cfi_startproc
	int $IO_TIPO_RECEIVE
	ret
	.cfi_endproc
//   ESAME 2018-07-04 )
/// @endcond
/// @}

/// @file sistema.s
/// @brief Parte Assembler del modulo sistema
/// @addtogroup sistema
/// @{
#include <libce.h>
#include <costanti.h>

//////////////////////////////////////////////////////////////////////////
/// @addtogroup proc
/// @{
/// @addtogroup procstat	Salvataggio e caricamento dello stato
/// @{
//////////////////////////////////////////////////////////////////////////

/// @cond
// offset dei vari registri all'interno di des_proc
.set PUNT_NUCLEO, 8
.set CTX, 16
.set RAX, CTX + I_RAX * 8
.set RCX, CTX + I_RCX * 8
.set RDX, CTX + I_RDX * 8
.set RBX, CTX + I_RBX * 8
.set RSP, CTX + I_RSP * 8
.set RBP, CTX + I_RBP * 8
.set RSI, CTX + I_RSI * 8
.set RDI, CTX + I_RDI * 8
.set R8,  CTX + I_R8  * 8
.set R9,  CTX + I_R9  * 8
.set R10, CTX + I_R10 * 8
.set R11, CTX + I_R11 * 8
.set R12, CTX + I_R12 * 8
.set R13, CTX + I_R13 * 8
.set R14, CTX + I_R14 * 8
.set R15, CTX + I_R15 * 8
.set CR3, CTX + N_REG * 8
/// @endcond

/// @brief copia lo stato dei registri generali nel des_proc del processo puntato da
/// @ref esecuzione.
/// @note Nessun registro viene sporcato.
salva_stato:
	// salviamo lo stato di un paio di registri in modo da poterli
	// temporaneamente riutilizzare. In particolare, useremo %rax come
	// registro di lavoro e %rbx come puntatore al des_proc.
	.cfi_startproc
	.cfi_def_cfa_offset 8
	pushq %rbx
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbx, -16
	pushq %rax
	.cfi_adjust_cfa_offset 8
	.cfi_offset rax, -24

	// aggiorniamo esecuzione_precedente (si veda la
	// definizione in sistema.cpp per sapere a cosa serve)
	movq esecuzione, %rbx
	movq %rbx, esecuzione_precedente

	// copiamo per primo il vecchio valore di %rax
	movq (%rsp), %rax
	movq %rax, RAX(%rbx)
	// usiamo %rax come appoggio per copiare il vecchio %rbx
	movq 8(%rsp), %rax
	movq %rax, RBX(%rbx)
	// copiamo gli altri registri
	movq %rcx, RCX(%rbx)
	movq %rdx, RDX(%rbx)
	// salviamo il valore che %rsp aveva prima della chiamata a salva stato
	// (valore corrente meno gli 8 byte che contengono l'indirizzo di
	// ritorno e i 16 byte dovuti alle due push che abbiamo fatto
	// all'inizio)
	movq %rsp, %rax
	addq $24, %rax
	movq %rax, RSP(%rbx)
	movq %rbp, RBP(%rbx)
	movq %rsi, RSI(%rbx)
	movq %rdi, RDI(%rbx)
	movq %r8,  R8 (%rbx)
	movq %r9,  R9 (%rbx)
	movq %r10, R10(%rbx)
	movq %r11, R11(%rbx)
	movq %r12, R12(%rbx)
	movq %r13, R13(%rbx)
	movq %r14, R14(%rbx)
	movq %r15, R15(%rbx)

	popq %rax
	.cfi_adjust_cfa_offset -8
	.cfi_restore rax
	popq %rbx
	.cfi_adjust_cfa_offset -8
	.cfi_restore rbx

	ret
	.cfi_endproc


/// @brief carica nei registri del processore lo stato contenuto nel des_proc del
/// processo puntato da @ref esecuzione.
/// @note Questa funzione sporca tutti i registri.
carica_stato:
	.cfi_startproc
	.cfi_def_cfa_offset 8
	movq esecuzione, %rbx

	popq %rcx   // ind di ritorno, va messo nella nuova pila
	.cfi_adjust_cfa_offset -8
	.cfi_register rip, rcx

	// nuovo valore per cr3
	movq CR3(%rbx), %r10
	movq %cr3, %rax
	cmpq %rax, %r10		// se cr3 non cambia
	je 1f			// evitiamo di invalidare il TLB
	movq %r10, %rax
	movq %rax, %cr3		// il TLB viene invalidato
1:

	// anche se abbiamo cambiato cr3 siamo sicuri che l'esecuzione prosegue
	// da qui, perché ci troviamo dentro la finestra FM che è comune a
	// tutti i processi
	movq RSP(%rbx), %rsp    // cambiamo pila
	pushq %rcx              // rimettiamo l'indirizzo di ritorno
	.cfi_adjust_cfa_offset 8
	.cfi_offset rip, -8

	// se il processo precedente era terminato o abortito la sua pila
	// sistema non era stata distrutta, in modo da permettere a noi di
	// continuare ad usarla. Ora che abbiamo cambiato pila possiamo
	// disfarci della precedente.
	cmpq $0, ultimo_terminato
	je 1f
	call distruggi_pila_precedente
1:

	// aggiorniamo il puntatore alla pila sistema usata dal meccanismo
	// delle interruzioni
	movq PUNT_NUCLEO(%rbx), %rcx
	movq tss_punt_nucleo, %rdi
	movq %rcx, (%rdi)

	// carichiamo tutti i registri
	movq RAX(%rbx), %rax
	// rbx per ultimo, perché ci serve per puntare il contesto
	movq RCX(%rbx), %rcx
	movq RDI(%rbx), %rdi
	movq RSI(%rbx), %rsi
	movq RBP(%rbx), %rbp
	movq RDX(%rbx), %rdx
	movq R8(%rbx), %r8
	movq R9(%rbx), %r9
	movq R10(%rbx), %r10
	movq R11(%rbx), %r11
	movq R12(%rbx), %r12
	movq R13(%rbx), %r13
	movq R14(%rbx), %r14
	movq R15(%rbx), %r15
	// ora possiamo caricare anche rbx
	movq RBX(%rbx), %rbx

	ret
	.cfi_endproc
/// @}
/// @}

////////////////////////////////////////////////////////////////////////
/// @addtogroup init
/// @{
////////////////////////////////////////////////////////////////////////

/// @brief Carica un gate della IDT
/// @param num		indice (a partire da 0) in IDT del gate da caricare
/// @param routine	indirizzo della routine da associare al gate
/// @param dpl		dpl del gate (LIV_SISTEMA o LIV_UTENTE)
.macro carica_gate num routine dpl
	// per prima cosa controlliamo che il gate non sia già occupato
	movq $\num, %rdi
	call gate_present
	cmpb $0, %al
	je 1f
	// gate già occupato: è un errore di sistema, quindi chiamiamo
	// panic (con messaggio idt_error, definito in fondo al file)
	leaq idt_error(%rip), %rdi
	movq $\num, %rsi
	call _Z6fpanicPKcz
	// nel caso normale chiamiamo gate_init (in libce) passandole
	// tutti i parametri
1:	movq $\num, %rdi
	movq $\routine, %rsi
	xorq %rdx, %rdx		// forziamo il tipo interrupt
	movq $\dpl, %rcx
	call gate_init
.endm


/// @brief Carica la IDT
///
/// le prime 32 entrate sono definite dall'Intel e corrispondono
/// alle possibili eccezioni.
/// La funzione è chiamata da bare/start.s in libce.
.global init_idt
init_idt:
	.cfi_startproc
/// @cond
	//		indice		routine			dpl
	// gestori eccezioni:
	carica_gate	0 		exc_div_error 	LIV_SISTEMA
	carica_gate	1 		exc_debug 	LIV_SISTEMA
	carica_gate	2 		exc_nmi 	LIV_SISTEMA
	carica_gate	3 		exc_breakpoint 	LIV_SISTEMA
	carica_gate	4 		exc_overflow 	LIV_SISTEMA
	carica_gate	5 		exc_bound_re 	LIV_SISTEMA
	carica_gate	6 		exc_inv_opcode	LIV_SISTEMA
	carica_gate	7 		exc_dev_na 	LIV_SISTEMA
	carica_gate	8 		exc_dbl_fault 	LIV_SISTEMA
	carica_gate	9 		exc_coproc_so 	LIV_SISTEMA
	carica_gate	10 		exc_inv_tss 	LIV_SISTEMA
	carica_gate	11 		exc_segm_fault 	LIV_SISTEMA
	carica_gate	12 		exc_stack_fault	LIV_SISTEMA
	carica_gate	13 		exc_prot_fault 	LIV_SISTEMA
	carica_gate	14 		exc_page_fault 	LIV_SISTEMA
	// il tipo 15 è riservato
	carica_gate	16 		exc_fp 		LIV_SISTEMA
	carica_gate	17 		exc_ac 		LIV_SISTEMA
	carica_gate	18 		exc_mc 		LIV_SISTEMA
	carica_gate	19 		exc_simd 	LIV_SISTEMA
	carica_gate	20		exc_virt	LIV_SISTEMA
	// tipi 21-29 riservati
	carica_gate	30		exc_sec		LIV_SISTEMA
	// tipo 31 riservato

	//primitive comuni (tipi 0x2-)
	carica_gate	TIPO_A		a_activate_p	LIV_UTENTE
	carica_gate	TIPO_T		a_terminate_p	LIV_UTENTE
	carica_gate	TIPO_SI		a_sem_ini	LIV_UTENTE
	carica_gate	TIPO_W		a_sem_wait	LIV_UTENTE
	carica_gate	TIPO_S		a_sem_signal	LIV_UTENTE
	carica_gate	TIPO_D		a_delay		LIV_UTENTE
	carica_gate	TIPO_AB		a_abort_p	LIV_UTENTE
	carica_gate	TIPO_L		a_do_log	LIV_UTENTE
	carica_gate	TIPO_GMI	a_getmeminfo	LIV_UTENTE
	carica_gate TIPO_MB_INIT	a_msgbox_init	LIV_UTENTE
	carica_gate TIPO_MB_RECV	a_msgbox_recv	LIV_UTENTE
	carica_gate TIPO_MB_SEND	a_msgbox_send	LIV_UTENTE

	// primitive per il livello I/O (tipi 0x3-)
	carica_gate	TIPO_APE	a_activate_pe	LIV_SISTEMA
	carica_gate	TIPO_WFI	a_wfi		LIV_SISTEMA
	carica_gate	TIPO_FG		a_fill_gate	LIV_SISTEMA
	carica_gate	TIPO_TRA	a_trasforma	LIV_SISTEMA
	carica_gate	TIPO_ACC	a_access	LIV_SISTEMA
	carica_gate	TIPO_PA		a_phys_alloc	LIV_SISTEMA
	carica_gate	TIPO_PD		a_phys_dealloc	LIV_SISTEMA

	// i tipi 0x4- verranno usati per le primitive fornite dal modulo I/O
	// (si veda fill_io_gates() in io.s)

	// i tipi da 0x50 a 0xFD verrano usati per gli handler
	// (si veda load_handler() più avanti)

	// la priorità massima è riservata al driver del timer di sistema
	carica_gate	INTR_TIPO_TIMER	driver_td	LIV_SISTEMA

	// idt_pointer è definito nella libce
	lidt idt_pointer
	ret
/// @endcond
	.cfi_endproc
/// @}

////////////////////////////////////////////////////////
// a_primitive                                        //
////////////////////////////////////////////////////////

/// @addtogroup procsyscalls
/// @{

/// Parte Assembler della primitiva activate_p()
	.extern c_activate_p
a_activate_p:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_activate_p
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva terminate_p()
	.extern c_terminate_p
a_terminate_p:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $1, %rdi		// logmsg = true
	call c_terminate_p
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva abort_p()
	.extern c_abort_p
a_abort_p:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $1, %rdi		// selfdump = true
	call c_abort_p
	call carica_stato
	iretq
	.cfi_endproc
/// @}

/// @addtogroup semsyscall
/// @{

/// Parte Assembler della primitiva sem_ini()
	.extern c_sem_ini
a_sem_ini:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_sem_ini
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva sem_wait()
	.extern c_sem_wait
a_sem_wait:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_sem_wait
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva sem_signal()
	.extern c_sem_signal
a_sem_signal:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_sem_signal
	call carica_stato
	iretq
	.cfi_endproc
/// @}

/// @addtogroup timersyscall
/// @{

/// Parte Assembler della primitiva delay()
	.extern c_delay
a_delay:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_delay
	call carica_stato
	iretq
	.cfi_endproc
/// @}

/// @addtogroup debugsyscall
/// @{

/// Parte Assembler della primitiva do_log()
	.extern c_log
a_do_log:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_do_log
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva getmeminfo()
a_getmeminfo:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_getmeminfo
	call carica_stato
	iretq
	.cfi_endproc
/// @}

/// @addtogroup pgsyscall
/// @{

/// Parte Assembler della primitva trasforma()
	.extern c_trasforma
a_trasforma:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_trasforma
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva access()
	.extern c_access
a_access:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_access
	call carica_stato
	iretq
	.cfi_endproc

/// @}

/// @addtogroup sysiosyscall
/// @{

/// Parte Assembler della primitiva activate_pe()
	.extern c_activate_pe
a_activate_pe:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_activate_pe
	call carica_stato
	iretq
	.cfi_endproc

/// Primitiva wfi()
a_wfi:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call apic_send_EOI
	call schedulatore
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva fill_gate()
a_fill_gate:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_fill_gate
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva phys_alloc()
	.extern c_phys_alloc
a_phys_alloc:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_phys_alloc
	call carica_stato
	iretq
	.cfi_endproc

/// Parte Assembler della primitiva phys_dealloc()
	.extern c_phys_dealloc
a_phys_dealloc:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_phys_dealloc
	call carica_stato
	iretq
	.cfi_endproc
/// @}

	.extern c_msgbox_init
a_msgbox_init:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_msgbox_init
	call carica_stato
	iretq
	.cfi_endproc
	
	.extern c_msgbox_recv
a_msgbox_recv:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_msgbox_recv
	call carica_stato
	iretq
	.cfi_endproc

	.extern c_msgbox_send
a_msgbox_send:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_msgbox_send
	call carica_stato
	iretq
	.cfi_endproc

////////////////////////////////////////////////////////////////
/// @addtogroup exc
/// @{
/// @addtogroup excroutine Routine di risposta alle eccezioni
///
/// Gestiamo tutte le eccezioni (tranne nmi) nello stesso modo: inviamo un
/// messaggio al log e terminiamo forzatamente il processo che ha causato
/// l'eccezione. Per questo motivo, ogni gestore chiama la funzione
/// gestore_eccezioni() (definita in sistema.cpp), passandole il numero
/// corrispondente al suo tipo di eccezione (primo argomento) e il %rip che si
/// trova in cima alla pila (terzo argomento). Quest'ultimo da indicazioni sul
/// punto del programma che ha causato l'eccezione.
///
/// Il secondo parametro passato a gestore_eccezioni() richiede qualche
/// spiegazione in più.  Alcune eccezioni lasciano in pila un'ulteriore parola
/// quadrupla, il cui significato dipende dal tipo di eccezione.  Per avere la
/// pila sistema in uno stato uniforme prima di chiamare salva_stato(), estraiamo
/// questa ulteriore parola quadrupla copiandola nella variable globale
/// exc_error. Il contenuto di exc_error viene poi passato come secondo
/// parametro di gestore_eccezioni().  Le eccezioni che non prevedono questa
/// ulteriore parola quadrupla si limitano a passare 0. L'uso della variabile
/// globale exc_error non causa problemi, perché le interruzioni sono disabilitate.
/// @{

/// @brief Eccezione di divisione per zero
///
/// Tipo: 0; Classe: fault; exc_error: no
exc_div_error:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $0, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di debug
///
/// Tipo: 1; Classe: fault o trap; exc_error: no
/// @note se sollevata dal single-step, è di classe trap.
exc_debug:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $1, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Interruzione esterna non mascherabile
/// @note non si tratta di una eccezione, ma di una interruzine
/// esterna con tipo fisso.
exc_nmi:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	// gestiamo nmi (non-maskable-interrupt) in modo speciale.  La funzione
	// c_nmi chiamerà panic(), la quale stamperà un po' di informazioni sullo
	// stato del sistema e spegnerà la macchina.
	// In particolare, il sito dell'autocorrezione invia un nmi alla VM se
	// questa non termina nel tempo massimo prestabilito.
	call c_nmi
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di breakpoing
///
/// Tipo: 3; Classe: trap; exc_error: no
exc_breakpoint:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $3, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di overflow
///
/// Tipo: 4; Classe: trap; exc_error: no
/// @note sollevata da INTO se il flag OF è settato
exc_overflow:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $4, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di bound-check
///
/// Tipo: 5; Classe: fault; exc_error: no
/// @note sollevata dall'istruzione BOUND
exc_bound_re:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $5, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di istruzione non valida
///
/// Tipo: 6; Classe: fault; exc_error: no
exc_inv_opcode:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $6, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di FPU non disponibile
///
/// Tipo: 7; Classe: fault; exc_error: no
exc_dev_na:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $7, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Doppio fault
///
/// Tipo: 8; Classe: abort; exc_error: 0
exc_dbl_fault:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	popq exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $8, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di coprocessor segment overrun
///
/// Tipo: 9; Classe: abort; exc_error: no
/// @note vecchia eccezione del 386, oggi non usata
exc_coproc_so:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $9, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di TSS non valido
///
/// Tipo: 10; Classe: fault; exc_error: selettore del segmento TSS
exc_inv_tss:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pop exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $10, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di segmento non presente
///
/// Tipo: 11; Classe: fault; exc_error: selettore del segmento
exc_segm_fault:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pop exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $11, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di errore sulla pila
///
/// Tipo: 12; Classe: fault; exc_error: selettore del segmento pila o 0
/// @note nel modo a 64 bit, sollevata se si carica un indirizzo non
/// canonico in RSP
exc_stack_fault:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pop exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $12, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di protezione
///
/// Tipo: 13; Classe: fault; exc_error: selettore di segmento o zero
/// @note eccezione molto generale che viene sollevata in molti
/// casi (per esempio, se si usa un operando in memoria che ha un indirizzo
/// non canonico)
exc_prot_fault:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pop exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $13, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di errore durante la traduzione di un indirizzo (page fault)
///
/// Tipo: 14; Classe: fault; exc_error: ulteriori informazioni sul fault
exc_page_fault:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pop exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $14, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione floating point
///
/// Tipo: 16; Classe: fault; exc_error: no
/// @note sollevata dalla FPU in vari casi, ma solo se il flag NE di CR0
/// è settato.
exc_fp:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $16, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di accesso non allineato
///
/// Tipo: 16; Classe: fault; exc_error: flag che indica se l'eccezione
/// si è verificata durante una INT oppure no.
/// @note sollevata solo in modo utente, se sono settati il flag AM
/// in CR0 e il flag AC nel registro dei flag
exc_ac:
	.cfi_startproc
	.cfi_def_cfa_offset 48
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	pop exc_error
	.cfi_adjust_cfa_offset -8
	call salva_stato
	movq $13, %rdi
	movq exc_error, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di machine-check
///
/// Tipo: 18; Classe: abort; exc_error: no
/// @note sollevata se vengono rilevati errori hardware,
/// ma solo se è settato il flag MCE in CR4.
exc_mc:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $18, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione nelle istruzioni SIMD
///
/// Tipo: 19; Classe: fault; exc_error: no
exc_simd:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $19, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// @brief Eccezione di virtualizzazione
///
/// Tipo: 20; Classe fault; exc_error: no
exc_virt:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $20, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc

/// Eccezione di sicurezza
exc_sec:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_escape CE_FAULT
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	movq $31, %rdi
	movq $0, %rsi
	movq (%rsp), %rdx
	call gestore_eccezioni
	call carica_stato
	iretq
	.cfi_endproc
/// @}
/// @}

////////////////////////////////////////////////////////
// handler/driver                                     //
////////////////////////////////////////////////////////

/// @addtogroup timer
/// @{

/// Parte assembler del driver del timer
	.extern c_driver_td
driver_td:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call c_driver_td
	call apic_send_EOI
	call carica_stato
	iretq
	.cfi_endproc
/// @}

/// @addtogroup sysio
/// @{

/// @addtogroup handler Handler
/// @{
/// Il modulo sistema contiene un handler per ogni possibile piedino dell'APIC.
/// Inizialmente gli handler non sono collegati alla IDT, perché i tipo associati
/// ai piedini non sono noti. L'associazione IDT[tipo]->handler_i sarà creata
/// dalla activate_pe() usando la funzione load_handler().

/// Handler associato al piedino 0 dell'APIC
handler_0:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+0*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 1 dell'APIC
handler_1:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+1*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 2 dell'APIC
handler_2:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+2*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 3 dell'APIC
handler_3:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+3*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 4 dell'APIC
handler_4:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+4*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 5 dell'APIC
handler_5:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+5*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 6 dell'APIC
handler_6:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+6*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 7 dell'APIC
handler_7:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+7*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 8 dell'APIC
handler_8:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+8*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 9 dell'APIC
handler_9:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+9*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 10 dell'APIC
handler_10:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+10*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 11 dell'APIC
handler_11:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+11*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 12 dell'APIC
handler_12:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+12*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 13 dell'APIC
handler_13:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+13*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 14 dell'APIC
handler_14:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+14*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 15 dell'APIC
handler_15:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+15*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 16 dell'APIC
handler_16:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+16*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 17 dell'APIC
handler_17:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+17*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 18 dell'APIC
handler_18:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+18*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 19 dell'APIC
handler_19:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+19*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 20 dell'APIC
handler_20:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+20*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 21 dell'APIC
handler_21:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+21*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 22 dell'APIC
handler_22:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+22*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc

/// Handler associato al piedino 23 dell'APIC
handler_23:
	.cfi_startproc
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	call salva_stato
	call inspronti

	mov a_p+23*8, %rax
	movq %rax, esecuzione

	call carica_stato
	iretq
	.cfi_endproc
/// @}
/// @}

/// @cond
	.global load_handler
	// La funzione si aspetta un <tipo> in %rdi e un <irq> in %rsi.
	// Provvede quindi a caricare il gate <tipo> della ITD in modo che
	// punti a handler_<irq>.
load_handler:
	.cfi_startproc
	movq %rsi, %rax
	// visto che gli handler sono tutti della stessa dimensione,
	// calcoliamo l'indirizzo dell'handler che ci interessa usando
	// la formula
	//
	//	handler_0 + <dim_handler> * <irq>
	//
	// dove <dim_handler> si può ottenere sottraendo gli indirizzi
	// di due handler consecutivi qualunque.
	movq $(handler_1 - handler_0), %rcx
	mulq %rcx
	movq $handler_0, %rsi
	addq %rax, %rsi
	// ora %rsi contiene l'indirizzo dell'handler, mentre %rdi
	// contiene ancora il tipo
	xorq %rdx, %rdx	// tipo interrupt
	movq $LIV_SISTEMA, %rcx
	call gate_init
	ret
	.cfi_endproc
/// @endcond

/// @addtogroup init
/// @{

/// Per cedere il controllo da init ad un processo simuliamo l'esecuzione di una INT
///
/// In particolare, modichiamo il contenuto della pila in questo modo:
///
///           prima:                  dopo:
///
///          |             |      rsp | rip ritorno |
///          |             |          | cs sistema  |
///          |             |          | flags       |
///          |             |          | rsp         |--+
///      rsp | rip ritorno |          | 0           |  |
///          |     ...     |          |    ...      |<-+
///
	.global cedi_controllo
cedi_controllo: // (des_proc *p)
	.cfi_startproc
	// estraiamo il valore di rip attualmente in cima alla pila
	pop %rax
	.cfi_adjust_cfa_offset -8
	.cfi_register rip, rax
	// salviamo il valore di rsp attuale
	mov %rsp, %rsi
	// costruiamo il nuovo stato della pila
	.cfi_def_cfa_register rsi
	.cfi_def_cfa_offset 0
	push $0				// ss	(selettore nullo)
	push %rsi			// rsp	(come salvato sopra)
	pushf				// flags
	push $SEL_CODICE_SISTEMA	// cs sistema
	push %rax			// rip ritorno
	.cfi_def_cfa_register rsp
	.cfi_def_cfa_offset 40
	.cfi_offset rip, -40
	.cfi_offset rsp, -16
	// completiamo il salvataggio dello stato con salva_stato
	call salva_stato
	// inseriamo init in coda pronti
	call inspronti
	// carichiamo lo stato del processo richiesto
	mov %rdi, esecuzione(%rip)
	call carica_stato
	iretq
	.cfi_endproc
/// @}

/// @cond
////////////////////////////////////////////////////////////////
// sezione dati                                               //
////////////////////////////////////////////////////////////////
.data
idt_error:
	.asciz "Errore nel caricamento del gate %#02x (duplicato?)"
.bss
exc_error:
	.space 8, 0
.global tss_punt_nucleo
tss_punt_nucleo:
	.quad 0
/// @endcond
/// @}

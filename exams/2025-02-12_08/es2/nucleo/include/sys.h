/// @file sys.h
/// @brief Primitive comuni definite dal modulo sistema
///
/// Queste primitive possono essere usate sia dal modulo utente che
/// dal modulo I/O.

/// @name Primitive per la gestione dei processi
/// @{

/**
 * @brief Crea un nuovo processo
 *
 * Il nuovo processo eseguirà _f(a)_ con priorità _prio_ e a livello _liv_.
 *
 * Un processo non può usare questa primitiva per creare un processo a priorità
 * o livello maggiori dei propri.
 *
 * @param f		corpo del processo
 * @param a		parametro per il corpo del processo 
 * @param prio		priorità del processo 
 * @param liv		livello del processo (LIV_UTENTE o LIV_SISTEMA)
 *
 * @return 		id del nuovo processo, o 0xFFFFFFFF in caso di errore
 */
extern "C" natl activate_p(void f(natq), natq a, natl prio, natl liv);

/**
 * @brief Termina il processo corrente.
 *
 * I processi devono invocare questa primitiva per poter terminare.
 */
extern "C" void terminate_p();

/**
 * @brief Abortisce il processo corrente.
 *
 * Come terminate_p(), ma mostra lo stato del processo sul log.
 */
extern "C" void abort_p();

/// @}

/// @name Primitive per l'utilizzo dei semafori
/// @{

/**
 * @brief Crea un nuovo semaforo.
 *
 * @param val		numero di gettoni iniziali
 *
 * @return 		id del nuovo semaforo, o 0xFFFFFFFF in caso di errore
 */
extern "C" natl sem_ini(int val);

/**
 * @brief Estrae un gettone da un semaforo.
 *
 * @param sem		id del semaforo.
 */
extern "C" void sem_wait(natl sem);

/**
 * @brief Inserisce un gettone in un semaforo.
 *
 * @param sem		id del semaforo
 */
extern "C" void sem_signal(natl sem);

/// @}

/// @name Primitive per l'utilizzo del timer
/// @{

/**
 * @brief Sospende il processo corrente.
 *
 * @param n		numero di intervalli di tempo
 */
extern "C" void delay(natl n);

/// @}

/// @name Primitive di supporto al debugging
/// @{

/**
 * @brief Informazioni di debug
 *
 * Questa struttura contiene delle informazioni che sono usate nei testi d'esame
 * per eseguire alcuni controlli.
 */
struct meminfo {
	/// numero di byte liberi nello heap di sistema
	natl heap_libero;
	/// numero di frame liberi in M2
	natl num_frame_liberi;
	/// id del processo corrente
	natl pid;
};

/**
 * @brief Estrae informazioni di debug.
 *
 * @return		struttura contenente le informazioni
 */
extern "C" meminfo getmeminfo();

/**
 * @brief Invia un messaggio al log.
 *
 * Questa primitiva è usata dai moduli I/O e utente per inviare i propri
 * messaggi al log di sistema. 
 *
 * @note Il modulo sistema usa direttamente la do_log() definita in libce.
 *
 * @param sev		severità del messaggio
 * @param buf		buffer contenente il messaggio
 * @param quanti	lunghezza del messaggio
 */
extern "C" void do_log(log_sev sev, const char* buf, natl quanti);
/// @}

// ( ESAME 2025-02-12


/**
 * @brief Apre una pipe in lettura o scrittura.
 * @param pipeid	id della pipe da aprire
 * @param writer	true se va aperta in scrittura, altrimenti in lettura
 * @return 		identificatore privato, o 0xFFFFFFFF in caso di estremità occupata
 *
 * @pre La pipe deve esistere.
 * @pre Il processo non deve avere già MAX_OPEN_PIPES estremità di pipe aperte.
 */
extern "C" natl openpipe(natl pipeid, bool writer);

/**
 * @brief Scrive su una pipe.
 *
 * @pre slotid deve essere l'identificatore privato valido di una
 *  pipe aperta in scrittura.
 * @pre Il buffer deve essere accessibile in lettura da tutti i processi.
 *
 * @param slotid	identificatore privato della pipe
 * @param buf		buffer contenente i caratteri da scrivere
 * @param n		numero di caratteri da scrivere
 * @return		true se tutti i byte sono stati trasferiti,
 * 			false se il processo lettore ha chiuso la
 * 			pipe prima di completare il trasferimento
 */
extern "C" bool writepipe(natl slotid, const char *buf, natl n);

/**
 * @brief Legge da una pipe.
 *
 * @pre slotid deve essere l'identificatore privato valido di una
 *  pipe aperta in lettura.
 * @pre Il buffer deve essere accessibile in scrittura da tutti i processi.
 *
 * @param slotid	identificatore privato della pipe
 * @param buf		buffer destinato a contenere i caratteri da leggere
 * @param n		numero di caratteri da leggere
 * @return		true se tutti i byte sono stati trasferiti,
 * 			false se il processo scrittore ha chiuso la
 * 			pipe prima di completare il trasferimento
 */
extern "C" bool readpipe(natl slotid, char *buf, natl n);

/**
 * @brief Chiude una pipe.
 * @param slotid 	identificatore privato della pipe da chiudere
 *
 * @pre slotid deve essere l'identificatore privato valido di una
 * pipe ancora aperta.
 */
extern "C" void closepipe(natl slotid);
//   ESAME 2025-02-12 )

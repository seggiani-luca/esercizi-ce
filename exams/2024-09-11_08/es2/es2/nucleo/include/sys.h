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

// ( ESAME 2024-09-11


/**
 * @brief Inizializza un nuovo rwlock.
 * @return id del rwlock, 0xFFFFFFFF in caso di errore
 */
extern "C" natl rw_init();

/**
 * @brief Acquisisce un lock in scrittura su un rwlock.
 *
 * Se necessario, sospende il processo fino a quando non è possibile
 * acquisire il write lock (nessun processo deve avere read o write lock).
 ,
 * @pre rw deve essere l'id di un rwlock già allocato.
 * @pre Il processo non deve avere già qualche diritto sul rwlock.
 *
 * @param rw id del rwlock
 * @return false se il processo ha già troppi rwlock, true altrimenti.
 */
extern "C" bool rw_writelock(natl rw);

/**
 * @brief Acquisice un lock di lettura su un rwlock.
 *
 * Se necessario, sospende il processo fino a quando non è possibile
 * acquisire il read lock (nessun processo deve avere il write lock).
 *
 * @pre rw deve essere l'id di un rwlock già allocato.
 * @pre Il processo non deve avere già qualche diritto sul rwlock.
 *
 * @param rw id del rwlock
 * @return false se il processo ha già troppi rwlock, true altrimenti.
 */
extern "C" bool rw_readlock(natl rw);

/**
 * @brief Trasforma un lock in lettura in uno in scrittura.
 *
 * Se necessario, sospende il processo fino a quando non è possibile
 * acquisire il write lock (nessun altro processo deve avere il read
 * lock). 
 *
 * @pre rw deve essere l'id di un rwlock già allocato.
 * @pre Il processo deve avere il diritto di lettura sul rwlock.
 *
 * @param rw id del rwlock.
 */
extern "C" void rw_upgrade(natl rw);

/**
 * @brief Rilascia il lock posseduto dal processo.
 *
 * @note Se il processo possiede un write lock ottenuto
 * tramite rw_upgrade() lo ritrasfrorma in read lock.
 *
 * @pre rw deve essere l'id di un rwlock già allocato.
 * @pre Il processo deve avere un lock sul rwlock.
 *
 * @param rw id del RW
 */
extern "C" void rw_downgrade(natl rw);

// ESAME 2024-09-11 )

/*! @file sistema.cpp
 *  @brief Parte C++ del modulo sistema.
 *  @addtogroup sistema Modulo sistema
 *  @{
 */
#include <costanti.h>
#include <libce.h>
#include <sys.h>
#include <sysio.h>
#include <boot.h>

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup proc		Processi
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/processi.pdf>
///
/// @{
/////////////////////////////////////////////////////////////////////////////////

/// Priorità del processo dummy
const natl DUMMY_PRIORITY = 0;

/// Descrittore di processo
struct des_proc {
	/// identificatore numerico del processo
	natw id;
	/// livello di privilegio (LIV_UTENTE o LIV_SISTEMA)
	natw livello;
	/// precedenza nelle code dei processi
	natl precedenza;
	/// indirizzo della base della pila sistema
	vaddr punt_nucleo;
	/// copia dei registri generali del processore
	natq contesto[N_REG];
	/// radice del TRIE del processo
	paddr cr3;

// ( ESAME 2015-07-02
	/// Primo indirizzo libero nella parte utente/condivisa del processo
	vaddr avail_addr;
//   ESAME 2015-07-02 )

	/// prossimo processo in coda
	des_proc* puntatore;

	/// @name Informazioni utili per il per debugging
	/// @{

	/// parametro `f` passato alla `activate_p`/`_pe` che ha creato questo processo
	void (*corpo)(natq);
	/// parametro `a` passato alla `activate_p`/`_pe` che ha creato questo processo
	natq  parametro;
	/// @}
};

/// @brief Tabella che associa l'id di un processo al corrispondente des_proc.
///
/// I des_proc sono allocati dinamicamente nello heap del sistema (si veda
/// crea_processo()).
des_proc* proc_table[MAX_PROC];

/// Numero di processi utente attivi.
natl processi;

/// Coda esecuzione (contiene sempre un solo elemento)
des_proc* esecuzione;

/// Coda pronti (vuota solo quando dummy è in @ref esecuzione)
des_proc* pronti;

/*! @brief Inserimento in lista ordinato (per priorità)
 *  @param p_lista	lista in cui inserire
 *  @param p_elem	elemento da inserire
 *  @note a parità di priorità favorisce i processi già in coda
 */
void inserimento_lista(des_proc*& p_lista, des_proc* p_elem)
{
// inserimento in una lista semplice ordinata
//   (tecnica dei due puntatori)
	des_proc *pp, *prevp;

	pp = p_lista;
	prevp = nullptr;
	while (pp && pp->precedenza >= p_elem->precedenza) {
		prevp = pp;
		pp = pp->puntatore;
	}

	p_elem->puntatore = pp;

	if (prevp)
		prevp->puntatore = p_elem;
	else
		p_lista = p_elem;

}

/*! @brief Estrazione del processo a maggiore priorità
 *  @param  p_lista 	lista da cui estrarre
 *  @return 		processo a più alta priorità, o nullptr se la lista è vuota
 */
des_proc* rimozione_lista(des_proc*& p_lista)
{
// estrazione dalla testa
	des_proc* p_elem = p_lista;  	// nullptr se la lista è vuota

	if (p_lista)
		p_lista = p_lista->puntatore;

	if (p_elem)
		p_elem->puntatore = nullptr;

	return p_elem;
}

/// @brief Inserisce @ref esecuzione in testa alla lista @ref pronti
extern "C" void inspronti()
{
	esecuzione->puntatore = pronti;
	pronti = esecuzione;
}

/*! @brief Sceglie il prossimo processo da mettere in esecuzione
 *  @note Modifica solo la variabile @ref esecuzione.
 *  Il processo andrà effettivamente in esecuzione solo alla prossima
 *  `call carica_stato; iretq`
 */
extern "C" void schedulatore(void)
{
	// poiché la lista è già ordinata in base alla priorità,
	// è sufficiente estrarre l'elemento in testa
	esecuzione = rimozione_lista(pronti);
}
/// @}

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup sem                   Semafori
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/semafori.pdf>
/// @{
/////////////////////////////////////////////////////////////////////////////////

/// Descrittore di semaforo
struct des_sem {
	/// se >= 0, numero di gettoni contenuti;
	/// se < 0, il valore assoluto è il numero di processi in coda
	int counter;
	/// coda di processi bloccati sul semaforo
	des_proc* pointer;
};

/// @brief Array dei descrittori di semaforo.
///
/// I primi MAX_SEM semafori di array_dess sono per il livello utente, gli altri
/// MAX_SEM sono per il livello sistema.
des_sem array_dess[MAX_SEM * 2];

/*! @brief Restituisce il livello a cui si trovava il processore al momento
 *  in cui è stata invocata la primitiva.
 *
 *  @warning funziona solo nelle routine di risposta ad una interruzione
 *  (INT, esterna o eccezione) se è stata chiamata salva_stato().
 */
int liv_chiamante()
{
	// salva_stato ha salvato il puntatore alla pila sistema
	// subito dopo l'invocazione della INT
	natq* pila = ptr_cast<natq>(esecuzione->contesto[I_RSP]);
	// la seconda parola dalla cima della pila contiene il livello
	// di privilegio che aveva il processore prima della INT
	return pila[1] == SEL_CODICE_SISTEMA ? LIV_SISTEMA : LIV_UTENTE;
}

/// Numero di semafori allocati per il livello utente
natl sem_allocati_utente  = 0;

/// Numero di semafori allocati per il livello sistema (moduli sistema e I/O)
natl sem_allocati_sistema = 0;

/*! @brief Alloca un nuovo semaforo.
 *  @return 	id del nuovo semaforo (0xFFFFFFFF se esauriti)
 */
natl alloca_sem()
{
	// I semafori non vengono mai deallocati, quindi è possibile allocarli
	// sequenzialmente. Per far questo è sufficiente ricordare quanti ne
	// abbiamo già allocati (variabili sem_allocati_utente e
	// sem_allocati_sistema)

	int liv = liv_chiamante();
	natl i;
	if (liv == LIV_UTENTE) {
		if (sem_allocati_utente >= MAX_SEM)
			return 0xFFFFFFFF;
		i = sem_allocati_utente;
		sem_allocati_utente++;
	} else {
		if (sem_allocati_sistema >= MAX_SEM)
			return 0xFFFFFFFF;
		i = sem_allocati_sistema + MAX_SEM;
		sem_allocati_sistema++;
	}
	return i;
}

/*! @brief Verifica un id di semaforo
 *  @param sem		id da verificare
 *  @return  		true se sem è l'id di un semaforo allocato; false altrimenti
 */
bool sem_valido(natl sem)
{
	// dal momento che i semafori non vengono mai deallocati,
	// un semaforo è valido se e solo se il suo indice è inferiore
	// al numero dei semafori allocati

	int liv = liv_chiamante();
	return sem < sem_allocati_utente ||
		(liv == LIV_SISTEMA && sem - MAX_SEM < sem_allocati_sistema);
}

/// @cond
extern "C" void c_abort_p(bool selfdump = true);
/// @endcond

/// @addtogroup semsyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva sem_ini().
 *  @param val		numero di gettoni iniziali
 */
extern "C" void c_sem_ini(int val)
{
	natl i = alloca_sem();

	if (i != 0xFFFFFFFF)
		array_dess[i].counter = val;

	esecuzione->contesto[I_RAX] = i;
}

/*! @brief Parte C++ della primitiva sem_wait().
 *  @param sem 		id di semaforo
 */
extern "C" void c_sem_wait(natl sem)
{
	// una primitiva non deve mai fidarsi dei parametri
	if (!sem_valido(sem)) {
		flog(LOG_WARN, "sem_wait: semaforo errato: %u", sem);
		c_abort_p();
		return;
	}

	des_sem* s = &array_dess[sem];
	s->counter--;

	if (s->counter < 0) {
		inserimento_lista(s->pointer, esecuzione);
		schedulatore();
	}
}

/*! @brief Parte C++ della primitiva sem_signal().
 *  @param sem 		id di semaforo
 */
extern "C" void c_sem_signal(natl sem)
{
	// una primitiva non deve mai fidarsi dei parametri
	if (!sem_valido(sem)) {
		flog(LOG_WARN, "sem_signal: semaforo errato: %u", sem);
		c_abort_p();
		return;
	}

	des_sem* s = &array_dess[sem];
	s->counter++;

	if (s->counter <= 0) {
		des_proc* lavoro = rimozione_lista(s->pointer);
		inspronti();	// preemption
		inserimento_lista(pronti, lavoro);
		schedulatore();	// preemption
	}
}
/// @}
/// @}

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup  timer 		Timer
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/delay.pdf>
/// @{
/////////////////////////////////////////////////////////////////////////////////

/// Richiesta al timer
struct richiesta {
	/// tempo di attesa aggiuntivo rispetto alla richiesta precedente
	natl d_attesa;
	/// puntatore alla richiesta successiva
	richiesta* p_rich;
	/// descrittore del processo che ha effettuato la richiesta
	des_proc* pp;
};

/// Coda dei processi sospesi
richiesta* sospesi;

/*! @brief Inserisce un processo nella coda delle richieste al timer
 *  @param p		richiesta da inserire
 */
void inserimento_lista_attesa(richiesta* p)
{
	richiesta *r, *precedente;

	r = sospesi;
	precedente = nullptr;

	while (r != nullptr && p->d_attesa > r->d_attesa) {
		p->d_attesa -= r->d_attesa;
		precedente = r;
		r = r->p_rich;
	}

	p->p_rich = r;
	if (precedente != nullptr)
		precedente->p_rich = p;
	else
		sospesi = p;

	if (r != nullptr)
		r->d_attesa -= p->d_attesa;
}

/// @addtogroup timersyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva delay.
 *  @param n		numero di intervalli di tempo
 */
extern "C" void c_delay(natl n)
{
	// caso particolare: se n è 0 non facciamo niente
	if (!n)
		return;

	richiesta* p = new richiesta;
	p->d_attesa = n;
	p->pp = esecuzione;

	inserimento_lista_attesa(p);
	schedulatore();
}
/// @}

/// Driver del timer
extern "C" void c_driver_td(void)
{
	inspronti();

	if (sospesi != nullptr) {
		sospesi->d_attesa--;
	}

	while (sospesi != nullptr && sospesi->d_attesa == 0) {
		inserimento_lista(pronti, sospesi->pp);
		richiesta* p = sospesi;
		sospesi = sospesi->p_rich;
		delete p;
	}

	schedulatore();
}
/// @}

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup exc      		Eccezioni
///
/// Nel nucleo base le eccezioni causano o la terminazione forzata (abort) del
/// processo che ha causato l'erore, o un errore fatale di sistema (panic) se
/// l'errore è da imputare a qualche bug nel sistema stesso.
///
/// Alcuni esercizi di esame mostrano altri usi delle eccezioni.
/// @{
/////////////////////////////////////////////////////////////////////////////////

/// @cond
// Controlla che un indirizzo cada nella parte sistema/condivisa
bool in_sis_c(vaddr);

// Ferma tutto il sistema (in caso di bug nel sistema stesso)
// msg		messaggio da inviare al log (severità LOG_ERR)
extern "C" [[noreturn]] void panic(const char* msg);

// mostra sul log lo stato del processo (definita più avanti)
void process_dump(des_proc*, log_sev sev);
/// @endcond

/*! @brief Gestore generico di eccezioni.
 *
 *  Funzione chiamata da tutti i gestori di eccezioni in sistema.s, tranne
 *  quello per il Non-Maskable-Interrupt.
 *
 *  @param tipo 	tipo dell'eccezione
 *  @param errore	eventuale codice di errore aggiuntivo
 *  @param rip		instruction pointer salvato in pila
 */
extern "C" void gestore_eccezioni(int tipo, natq errore, vaddr rip)
{
	log_exception(tipo, errore, rip);

	if (tipo != 14 && (errore & SE_EXT)) {
		// la colpa dell'errore non si può attribuire al programma, ma al
		// sistema. Blocchiamo dunque l'esecuzione
		panic("ERRORE DI SISTEMA (EXT)");
	}

	if (tipo == 14 && (errore & PF_RES)) {
		// se la MMU ha rilevato un errore sui bit riservati vuol dire
		// che c'è qualche bug nel modulo sistema stesso, quindi
		// blocchiamo l'esecuzione
		panic("ERRORE NELLE TABELLE DI TRADUZIONE");
	}

	if (!(errore & PF_USER) && in_sis_c(rip)) {
		// l'indirizzo dell'istruzione che ha causato il fault è
		// all'interno del modulo sistema.
		panic("ERRORE DI SISTEMA");
	}

	// se siamo qui la colpa dell'errore è da imputare ai moduli utente o
	// I/O.  Inviamo sul log lo stato del processo al momento del sollevamento
	// dell'eccezione (come in questo momento si trova salvato in pila
	// sistema e descrittore di processo)
	process_dump(esecuzione, LOG_WARN);
	// abortiamo il processo (senza mostrare nuovamente il dump)
	c_abort_p(false /* no dump */);
}
/// @}

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup frame		Frame
/// @brief Allocazione e rilascio dei frame della memoria fisica.
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/paginazione-implementazione.pdf>
/// @{
/////////////////////////////////////////////////////////////////////////////////

/// Descrittore di frame
struct des_frame {
	union {
		/// numero di entrate valide (se il frame contiene una tabella)
		natw nvalide;
		/// prossimo frame libero (se il frame è libero)
		natl prossimo_libero;
// ( ESAME 2015-07-02
		/// @ingroup esame
		///  @brief Prossimo frame nella SHMEM (se il frame contiene una pagina)
		///
		/// Il campo è nullo se il frame è l'ultimo della SHMEM a cui appartiene,
		/// o se non appartiene a nessuna SHMEM.
		natl next_shmem;
//   ESAME 2015-07-02 )
	};
};

/// Numero totale di frame (M1 + M2)
natq const N_FRAME = MEM_TOT / DIM_PAGINA;

/// Numero di frame in M1
natq N_M1;

/// Numero di frame in M2
natq N_M2;

/// Array dei descrittori di frame
des_frame vdf[N_FRAME];

/// Testa della lista dei frame liberi
natq primo_frame_libero;

/// Numero di frame nella lista dei frame liberi
natq num_frame_liberi;

/*! @brief Inizializza la parte M2 e i descrittori di frame.
 *
 *  Funzione chiamata in fase di inizializzazione.
 */
void init_frame()
{
	extern char _end[];
	// Tutta la memoria non ancora occupata viene usata per i frame.  La
	// funzione si preoccupa anche di inizializzare i descrittori dei frame
	// in modo da creare la lista dei frame liberi.  end è l'indirizzo del
	// primo byte non occupato dal modulo sistema (è calcolato dal
	// collegatore). La parte M2 della memoria fisica inizia al primo frame
	// dopo end.

	// primo frame di M2
	paddr fine_M1 = allinea(int_cast<paddr>(_end), DIM_PAGINA);
	// numero di frame in M1 e indice di f in vdf
	N_M1 = fine_M1 / DIM_PAGINA;
	// numero di frame in M2
	N_M2 = N_FRAME - N_M1;

	if (!N_M2)
		return;

	// creiamo la lista dei frame liberi, che inizialmente contiene tutti i
	// frame di M2
	primo_frame_libero = N_M1;
/// @cond
#ifndef N_STEP
	// Alcuni esercizi definiscono N_STEP == 2 per creare mapping non
	// contigui in memoria virtale e controllare meglio alcuni possibili
	// bug
#define N_STEP 1
#endif
/// @endcond
	natq last = 0;
	for (natq j = 0; j < N_STEP; j++) {
		for (natq i = j; i < N_M2; i += N_STEP) {
			vdf[primo_frame_libero + i].prossimo_libero =
				primo_frame_libero + i + N_STEP;
			num_frame_liberi++;
			last = i;
		}
		vdf[primo_frame_libero + last].prossimo_libero =
			primo_frame_libero + j + 1;
	}
	vdf[primo_frame_libero + last].prossimo_libero = 0;
}

/*! @brief Estrae un frame dalla lista dei frame liberi.
 *  @return indirizzo fisico del frame estratto, o 0 se la lista è vuota
 */
paddr alloca_frame()
{
	if (!num_frame_liberi) {
		flog(LOG_ERR, "out of memory");
		return 0;
	}
	natq j = primo_frame_libero;
	primo_frame_libero = vdf[primo_frame_libero].prossimo_libero;
	vdf[j].prossimo_libero = 0;
	num_frame_liberi--;
	return j * DIM_PAGINA;
}

/*! @brief Restiuisce un frame alla lista dei frame liberi.
 *  @param f		indirizzo fisico del frame da restituire
 */
void rilascia_frame(paddr f)
{
	natq j = f / DIM_PAGINA;
	if (j < N_M1) {
		fpanic("tentativo di rilasciare il frame %lx di M1", f);
	}
	// dal momento che i frame di M2 sono tutti equivalenti, è
	// sufficiente inserire in testa
	vdf[j].prossimo_libero = primo_frame_libero;
	primo_frame_libero = j;
	num_frame_liberi++;
}
/// @}

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup pg		Paginazione
/// @brief Creazione e distruzione dei TRIE.
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/paginazione-implementazione.pdf>
///
/// @{
/////////////////////////////////////////////////////////////////////////////////
#include <vm.h>

/// @addtogroup ranges Parti della memoria virtuale dei processi
///
/// Le parti hanno dimensioni multiple della dimensione della pagina di livello
/// massimo (@ref PART_SIZE), sono allineate naturalmente e non si
/// sovrappongono.  In questo modo possiamo definire le varie parti
/// semplicemente specificando un intervallo di entrate della tabella radice.
/// Per esempio, la parte sistema/condivisa usa @ref N_SIS_C entrate a partire da
/// @ref I_SIS_C e contiene tutti e soli gli indirizzi la cui traduzione passa da
/// queste entrate.
/// @{

// Tutti gli N_* devono essere compresi in (0, 256]
static_assert(N_SIS_P > 0 && N_SIS_P <= 256);
static_assert(N_SIS_C > 0 && N_SIS_C <= 256);
static_assert(N_MIO_C > 0 && N_MIO_C <= 256);
static_assert(N_UTN_C > 0 && N_UTN_C <= 256);
static_assert(N_UTN_P > 0 && N_UTN_P <= 256);
// La parte sistema/condivisa deve iniziare da 0 per includere la finestra
// creata dal boot loader
static_assert(I_SIS_C == 0);
// Le parti sistema e modulo/IO devono trovarsi nella prima metà
static_assert(I_SIS_P >  0   && I_SIS_P < 256);
static_assert(I_MIO_C >  0   && I_MIO_C < 256);
// Le parti utente devono trovarsi nella seconda metà
static_assert(I_UTN_C >= 256 && I_UTN_C < 512);
static_assert(I_UTN_P >= 256 && I_UTN_P < 512);
// Le parti non si devono sovrapporre
static_assert(I_SIS_C + N_SIS_C <= I_SIS_P);
static_assert(I_SIS_P + N_SIS_P <= I_MIO_C);
static_assert(I_MIO_C + N_MIO_C <= 256);
static_assert(I_UTN_C + N_UTN_C <= I_UTN_P);
static_assert(I_UTN_P + N_UTN_P <= 512);

/// Granularità delle parti della memoria virtuale.
static const natq PART_SIZE = dim_region(MAX_LIV - 1);

const vaddr ini_sis_c = norm(I_SIS_C * PART_SIZE); ///< base di sistema/condivisa
const vaddr ini_sis_p = norm(I_SIS_P * PART_SIZE); ///< base di sistema/privata
const vaddr ini_mio_c = norm(I_MIO_C * PART_SIZE); ///< base di modulo IO/condivisa
const vaddr ini_utn_c = norm(I_UTN_C * PART_SIZE); ///< base di utente/condivisa
const vaddr ini_utn_p = norm(I_UTN_P * PART_SIZE); ///< base di utente/privata

const vaddr fin_sis_c = ini_sis_c + PART_SIZE * N_SIS_C; ///< limite di sistema/condivisa
const vaddr fin_sis_p = ini_sis_p + PART_SIZE * N_SIS_P; ///< limite di sistema/privata
const vaddr fin_mio_c = ini_mio_c + PART_SIZE * N_MIO_C; ///< limite di modulo IO/condivisa
const vaddr fin_utn_c = ini_utn_c + PART_SIZE * N_UTN_C; ///< limite di utente/condivisa
const vaddr fin_utn_p = ini_utn_p + PART_SIZE * N_UTN_P; ///< limite di utente/privata
/// @}

/// @addtogroup mapfuncs Funzioni necessarie per map() e unmap()
///
/// Le funzioni map() e unmap() di libce richiedono la definizione
/// di alcune funzioni per l'allocazione e la deallocazione delle
/// tabelle. Le definiamo qui, utilizzando i descrittori di frame.
/// In particolare, se un frame contiene una tabella, il campo
/// nvalide del suo descrittore (@ref des_frame) è il contatore
/// delle entrate valide della tabella (entrate della tabella con P == 1).
/// @{

/*! @brief Alloca un frame libero destinato a contenere una tabella.
 *
 *  Azzera tutta la tabella e il suo contatore di entrate di valide.
 *  @return indirizzo fisico della tabella
 */
paddr alloca_tab()
{
	paddr f = alloca_frame();
	if (f) {
		memset(voidptr_cast(f), 0, DIM_PAGINA);
		vdf[f / DIM_PAGINA].nvalide = 0;
	}
	return f;
}

/*! @brief Dealloca un frame che contiene una tabella
 *
 *  @warning La funzione controlla che la tabella non contenga entrate
 *  valide e causa un errore fatale in caso contrario.
 *
 *  @param f 		indirizzo fisico della tabella
 */
void rilascia_tab(paddr f)
{
	if (int n = get_ref(f)) {
		fpanic("tentativo di deallocare la tabella %lx con %d entrate valide", f, n);
	}
	rilascia_frame(f);
}

/*! @brief Incrementa il contatore delle entrate valide di una tabella
 *  @param f 		indirizzo fisico della tabella
 */
void inc_ref(paddr f)
{
	vdf[f / DIM_PAGINA].nvalide++;
}

/*! @brief Decrementa il contatore delle entrate valide di una tabella
 *  @param f 		indirizzo fisico della tabella
 */
void dec_ref(paddr f)
{
	vdf[f / DIM_PAGINA].nvalide--;
}

/*! @brief Legge il contatore delle entrate valide di una tabella
 *  @param f 		indirizzo fisico della tabella
 *  @return 		valore del contatore
 */
natl get_ref(paddr f)
{
	return vdf[f / DIM_PAGINA].nvalide;
}
/// @}

/*! @brief Controlla che un indirizzo appartenga alla zona utente/condivisa
 *  @param v 		indirizzo virtuale da controllare
 *  @return 		true sse _v_ appartiene alla parte utente/condivisa, false altrimenti
 */
bool in_utn_c(vaddr v)
{
	return v >= ini_utn_c && v < fin_utn_c;
}

/*! @brief Controlla che un indirizzo appartenga alla zona sistema/condivisa
 *  @param v 		indirizzo virtuale da controllare
 *  @return 		true sse _v_ appartiene alla parte sistema/condivisa, false altrimenti
 */
bool in_sis_c(vaddr v)
{
	return v >= ini_sis_c && v < fin_sis_c;
}

/// @addtogroup pgsyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva access()
 *
 *  Primitiva utilizzata dal modulo I/O per controllare che i buffer passati dal
 *  livello utente siano accessibili dal livello utente (problema del Cavallo di
 *  Troia) e non possano causare page fault nel modulo I/O (bit P tutti a 1 e
 *  scrittura permessa quando necessario).
 *
 *  @param begin	base dell'intervallo da controllare
 *  @param dim		dimensione dell'intervallo da controllare
 *  @param writeable	se true, l'intervallo deve essere anche scrivibile
 *  @param shared	se true, l'intevallo deve trovarsi in utente/condivisa
 *  @return		true se i vincoli sono rispettati, false altrimenti
 */
extern "C" bool c_access(vaddr begin, natq dim, bool writeable, bool shared = true)
{
	esecuzione->contesto[I_RAX] = false;

	if (!tab_iter::valid_interval(begin, dim))
		return false;

	if (shared && (!in_utn_c(begin) || (dim > 0 && !in_utn_c(begin + dim - 1))))
		return false;

	// usiamo un tab_iter per percorrere tutto il sottoalbero relativo
	// alla traduzione degli indirizzi nell'intervallo [begin, begin+dim).
	for (tab_iter it(esecuzione->cr3, begin, dim); it; it.next()) {
		tab_entry e = it.get_e();

		// interrompiamo il ciclo non appena troviamo qualcosa che non va
		if (!(e & BIT_P) || !(e & BIT_US) || (writeable && !(e & BIT_RW)))
			return false;
	}
	esecuzione->contesto[I_RAX] = true;
	return true;
}

/*! @brief Parte C++ della primitiva trasforma()
 *
 *  Traduce _ind_virt_ usando il TRIE del processo puntato
 *  da @ref esecuzione.
 *
 *  @param ind_virt 	indirizzo virtuale da tradurre
 */
extern "C" void c_trasforma(vaddr ind_virt)
{
	esecuzione->contesto[I_RAX] = trasforma(esecuzione->cr3, ind_virt);
}
/// @}
/// @}

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup proc
/// @{
/// @addtogroup create	Creazione e distruzione dei processi
/// @{
/////////////////////////////////////////////////////////////////////////////////

/// Associazione IRQ -> processo esterno che lo gestisce
des_proc* a_p[apic::MAX_IRQ];

/// @brief Valore da inserire in @ref a_p per gli IRQ che sono gestiti da driver
///
/// Nel nucleo base questo accade solo per l'IRQ del timer.
des_proc* const ESTERN_BUSY = reinterpret_cast<des_proc*>(1UL);

/// @name Funzioni di supporto alla creazione e distruzione dei processi
/// @{

/*! @brief Alloca un id di processo
 *  @param p		descrittore del processo a cui assegnare l'id
 *  @return		id del processo (0xFFFFFFFF se terminati)
 */
natl alloca_proc_id(des_proc* p)
{
	static natl next = 0;

	// La funzione inizia la ricerca partendo  dall'id successivo
	// all'ultimo restituito (salvato nella variable statica 'next'),
	// saltando quelli che risultano in uso.
	// In questo modo gli id tendono ad essere riutilizzati il più tardi possibile,
	// cosa che aiuta chi deve debuggare i propri programmi multiprocesso.
	natl scan = next, found = 0xFFFFFFFF;
	do {
		if (proc_table[scan] == nullptr) {
			found = scan;
			proc_table[found] = p;
		}
		scan = (scan + 1) % MAX_PROC;
	} while (found == 0xFFFFFFFF && scan != next);
	next = scan;
	return found;
}

/*! @brief Rilascia un id di processo non più utilizzato
 *  @param id		id da rilasciare
 */
void rilascia_proc_id(natw id)
{
	if (id > MAX_PROC_ID)
		fpanic("id %hu non valido (max %lu)", id, MAX_PROC_ID);

	if (proc_table[id] == nullptr)
		fpanic("tentativo di rilasciare id %d non allocato", id);

	proc_table[id] = nullptr;
}

/*! @brief Inizializza la tabella radice di un nuovo processo
 *  @param dest 	indirizzo fisico della tabella
 */
void init_root_tab(paddr dest)
{
	paddr pdir = esecuzione->cr3;

	// ci limitiamo a copiare dalla tabella radice corrente i puntatori
	// alle tabelle di livello inferiore per tutte le parti condivise
	// (sistema, utente e I/O). Quindi tutti i sottoalberi di traduzione
	// delle parti condivise saranno anch'essi condivisi. Questo, oltre a
	// semplificare l'inizializzazione di un processo, ci permette di
	// risparmiare un po' di memoria.
	copy_des(pdir, dest, I_SIS_C, N_SIS_C);
	copy_des(pdir, dest, I_MIO_C, N_MIO_C);
	copy_des(pdir, dest, I_UTN_C, N_UTN_C);
}

/*! @brief Ripulisce la tabella radice di un processo
 *  @param dest 	indirizzo fisico della tabella
 */
void clear_root_tab(paddr dest)
{
	// eliminiamo le entrate create da init_root_tab()
	set_des(dest, I_SIS_C, N_SIS_C, 0);
	set_des(dest, I_MIO_C, N_MIO_C, 0);
	set_des(dest, I_UTN_C, N_UTN_C, 0);
}

/*! @brief Crea una pila processo
 *
 *  @param root_tab	indirizzo fisico della radice del TRIE del processo
 *  @param bottom	indirizzo virtuale del bottom della pila
 *  @param size		dimensione della pila (in byte)
 *  @param liv		livello della pila (LIV_UTENTE o LIV_SISTEMA)
 *  @return		true se la creazione ha avuto successo, false altrimenti
 */
bool crea_pila(paddr root_tab, vaddr bottom, natq size, natl liv)
{
	vaddr v = map(root_tab,
		bottom - size,
		bottom,
		BIT_RW | (liv == LIV_UTENTE ? BIT_US : 0),
		[](vaddr) { return alloca_frame(); });
	if (v != bottom) {
		unmap(root_tab, bottom - size, v,
			[](vaddr, paddr p, int) { rilascia_frame(p); });
		return false;
	}
	return true;
}

/*! @brief Distrugge una pila processo
 *
 *  Funziona indifferentemente per pile utente o sistema.
 *
 *  @param root_tab	indirizzo fisico della radice del TRIE del processo
 *  @param bottom	indirizzo virtuale del bottom della pila
 *  @param size		dimensione della pila (in byte)
 */
void distruggi_pila(paddr root_tab, vaddr bottom, natq size)
{
	unmap(
		root_tab,
		bottom - size,
		bottom,
		[](vaddr, paddr p, int) { rilascia_frame(p); });
}

/*! @brief Funzione interna per la creazione di un processo.
 *
 *  Parte comune a activate_p() e activate_pe().  Alloca un id per il processo
 *  e crea e inizializza il descrittore di processo, la pila sistema e, per i
 *  processi di livello utente, la pila utente. Crea l'albero di traduzione
 *  completo per la memoria virtuale del processo.
 *
 *  @param f		corpo del processo
 *  @param a		parametro per il corpo del processo
 *  @param prio		priorità del processo
 *  @param liv		livello del processo (LIV_UTENTE o LIV_SISTEMA)
 *  @return		puntatore al nuovo descrittore di processo
 *  			(nullptr in caso di errore)
 */
des_proc* crea_processo(void f(natq), natq a, int prio, char liv)
{
	des_proc*	p;			// des_proc per il nuovo processo
	paddr		pila_sistema;		// pila_sistema del processo
	natq*		pl;			// pila sistema come array di natq
	natl		id;			// id del nuovo processo

	// allocazione (e azzeramento preventivo) di un des_proc
	p = new des_proc;
	if (!p)
		goto err_out;
	memset(p, 0, sizeof(des_proc));

// ( ESAME 2015-07-02
	/// @ingroup esame
	/// @note Assumiamo che che la parte di utente/condivisa già usata dalle
	/// sezioni .text, .data, .bss e dallo heap utente sia inferiore ad una
	/// regione di livello massimo (512GiB se MAX_LIV è 4) e facciamo
	/// partire la zona riservata alle SHMEM dalla seconda regione.
	p->avail_addr = ini_utn_c + dim_region(MAX_LIV - 1);
//   ESAME 2015-07-02 )

	// rimpiamo i campi di cui conosciamo già i valori
	p->precedenza = prio;
	p->puntatore = nullptr;
	// il registro RDI deve contenere il parametro da passare alla
	// funzione f
	p->contesto[I_RDI] = a;

	// selezione di un identificatore
	id = alloca_proc_id(p);
	if (id == 0xFFFFFFFF)
		goto err_del_p;
	p->id = id;

	// creazione della tabella radice del processo
	p->cr3 = alloca_tab();
	if (p->cr3 == 0)
		goto err_rel_id;
	init_root_tab(p->cr3);

	// creazione della pila sistema
	static_assert(DIM_SYS_STACK > 0 && (DIM_SYS_STACK & 0xFFF) == 0);
	if (!crea_pila(p->cr3, fin_sis_p, DIM_SYS_STACK, LIV_SISTEMA))
		goto err_rel_tab;
	// otteniamo un puntatore al fondo della pila appena creata.  Si noti
	// che non possiamo accedervi tramite l'indirizzo virtuale 'fin_sis_p',
	// che verrebbe tradotto seguendo l'albero del processo corrente, e non
	// di quello che stiamo creando.  Per questo motivo usiamo trasforma()
	// per ottenere il corrispondente indirizzo fisico. In questo modo
	// accediamo alla nuova pila tramite la finestra FM.
	//
	// Si noti anche che dobbiamo sottrarre almeno 1 a fin_sis_p prima di
	// tradurre, perché fin_sis_p stesso è fuori dalla parte
	// sistema/privata.
	pila_sistema = trasforma(p->cr3, fin_sis_p - 1) + 1;

	// convertiamo a puntatore a natq, per accedervi più comodamente
	pl = ptr_cast<natq>(pila_sistema);

	if (liv == LIV_UTENTE) {
		// inizializziamo la pila sistema.
		pl[-5] = int_cast<natq>(f);	    // RIP (codice utente)
		pl[-4] = SEL_CODICE_UTENTE;	    // CS (codice utente)
		pl[-3] = BIT_IF;	    	    // RFLAGS
		pl[-2] = fin_utn_p - sizeof(natq);  // RSP
		pl[-1] = SEL_DATI_UTENTE;	    // SS (pila utente)
		// eseguendo una IRET da questa situazione, il processo
		// passerà ad eseguire la prima istruzione della funzione f,
		// usando come pila la pila utente (al suo indirizzo virtuale)

		// creazione della pila utente
		static_assert(DIM_USR_STACK > 0 && (DIM_USR_STACK & 0xFFF) == 0);
		if (!crea_pila(p->cr3, fin_utn_p, DIM_USR_STACK, LIV_UTENTE)) {
			flog(LOG_WARN, "crea_processo: creazione pila utente fallita");
			goto err_del_sstack;
		}

		// inizialmente, il processo si trova a livello sistema, come
		// se avesse eseguito una istruzione INT, con la pila sistema
		// che contiene le 5 parole quadruple preparate precedentemente
		p->contesto[I_RSP] = fin_sis_p - 5 * sizeof(natq);

		p->livello = LIV_UTENTE;

		// dal momento che usiamo traduzioni diverse per le parti sistema/private
		// di tutti i processi, possiamo inizializzare p->punt_nucleo con un
		// indirizzo (virtuale) uguale per tutti i processi
		p->punt_nucleo = fin_sis_p;

		//   tutti gli altri campi valgono 0
	} else {
		// processo di livello sistema
		// inizializzazione della pila sistema
		pl[-6] = int_cast<natq>(f);		// RIP (codice sistema)
		pl[-5] = SEL_CODICE_SISTEMA;            // CS (codice sistema)
		pl[-4] = BIT_IF;  	        	// RFLAGS
		pl[-3] = fin_sis_p - sizeof(natq);      // RSP
		pl[-2] = 0;			        // SS
		pl[-1] = 0;			        // ind. rit.
							//(non significativo)
		// i processi esterni lavorano esclusivamente a livello
		// sistema. Per questo motivo, prepariamo una sola pila (la
		// pila sistema)

		// inizializziamo il descrittore di processo
		p->contesto[I_RSP] = fin_sis_p - 6 * sizeof(natq);

		p->livello = LIV_SISTEMA;

		// tutti gli altri campi valgono 0
	}

	// informazioni di debug
	p->corpo = f;
	p->parametro = a;

	return p;

err_del_sstack:	distruggi_pila(p->cr3, fin_sis_p, DIM_SYS_STACK);
err_rel_tab:	clear_root_tab(p->cr3);
		rilascia_tab(p->cr3);
err_rel_id:	rilascia_proc_id(p->id);
err_del_p:	delete p;
err_out:	return nullptr;
}

/// @cond
// usate da distruggi processo e definite più avanti
extern paddr ultimo_terminato;
extern des_proc* esecuzione_precedente;
extern "C" void distruggi_pila_precedente();
/// @endcond

/*! @brief Dealloca tutte le risorse allocate da crea_processo()
 *
 *  Dealloca l'id, il descrittore di processo, l'eventuale pila utente,
 *  comprese le tabelle che la mappavano nella memoria virtuale del processo.
 *  Per la pila sistema si veda sopra @ref esecuzione_precedente.
 */
void distruggi_processo(des_proc* p)
{
	paddr root_tab = p->cr3;
	// la pila utente può essere distrutta subito, se presente
	if (p->livello == LIV_UTENTE)
		distruggi_pila(root_tab, fin_utn_p, DIM_USR_STACK);
	// se p == esecuzione_precedente rimandiamo la distruzione alla
	// carica_stato, altrimenti distruggiamo subito
	ultimo_terminato = root_tab;
	if (p != esecuzione_precedente) {
		// riporta anche ultimo_terminato a zero
		distruggi_pila_precedente();
	}
	rilascia_proc_id(p->id);
	delete p;
}
/// @}

/// @name Distruzione della pila sistema corrente
///
/// Quando dobbiamo eliminare una pila sistema dobbiamo stare attenti a non
/// eliminare proprio quella che stiamo usando.  Questo succede durante una
/// terminate_p() o abort_p(), quando si tenta di distrugguere proprio il
/// processo che ha invocato la primitiva.
///
/// Per fortuna, se stiamo terminando il processo corrente, vuol dire anche che
/// stiamo per metterne in esecuzione un altro e possiamo dunque usare la pila
/// sistema di quest'ultimo. Operiamo dunque nel seguente modo:
///
/// - all'ingresso nel sistema (in salva_stato()), salviamo il valore di @ref esecuzione
///   in @ref esecuzione_precedente; questo è il processo a cui appartiene la
///   pila sistema che stiamo usando;
/// - in @ref distruggi_processo(), se @ref esecuzione è uguale a @ref esecuzione_precedente
///   (stiamo distruggendo proprio il processo a cui appartiene la pila corrente),
///   _non_ distruggiamo la pila sistema e settiamo la variabile @ref ultimo_terminato;
/// - in carica_stato(), dopo aver cambiato pila, se @ref ultimo_terminato è settato,
///   distruggiamo la pila sistema di @ref esecuzione_precedente.
///
/// @{

/// @brief Processo che era in esecuzione all'entrata nel modulo sistema
///
/// La salva_stato() ricorda quale era il processo in esecuzione al momento
/// dell'entrata nel sistema e lo scrive in questa variabile.
des_proc* esecuzione_precedente;

/// @brief Se diverso da zero, indirizzo fisico della root_tab dell'ultimo processo
///        terminato o abortito.
///
/// La carica_stato() legge questo indirizzo per sapere se deve distruggere la
/// pila del processo uscente, dopo aver effettuato il passaggio alla pila del
/// processo entrante.
paddr ultimo_terminato;

/*! @brief Distrugge la pila sistema del processo uscente e rilascia la sua tabella radice.
 *
 *  Chiamata da distruggi_processo() oppure da carica_stato() se
 *  distruggi_processo() aveva rimandato la distruzione della pila.  Dealloca la
 *  pila sistema e le traduzioni corrispondenti nel TRIE di radice
 *  @ref ultimo_terminato. La distruggi_processo() aveva già eliminato tutte le
 *  altre traduzioni, quindi la funzione può anche deallocare la radice del
 *  TRIE.
 */
extern "C" void distruggi_pila_precedente() {
	distruggi_pila(ultimo_terminato, fin_sis_p, DIM_SYS_STACK);
	// ripuliamo la tabella radice (azione inversa di init_root_tab())
	// in modo da azzerare il contatore delle entrate valide e passare
	// il controllo in rilascia_tab()
	clear_root_tab(ultimo_terminato);
	rilascia_tab(ultimo_terminato);
	ultimo_terminato = 0;
}
/// @}

/// @addtogroup procsyscalls Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva activate_p()
 *  @param f		corpo del processo
 *  @param a		parametro per il corpo del processo
 *  @param prio		priorità del processo
 *  @param liv		livello del processo (LIV_UTENTE o LIV_SISTEMA)
 */
extern "C" void c_activate_p(void f(natq), natq a, natl prio, natl liv)
{
	des_proc* p;			// des_proc per il nuovo processo
	natl id = 0xFFFFFFFF;		// id da restituire in caso di fallimento

	// non possiamo accettare una priorità minore di quella di dummy
	// o maggiore di quella del processo chiamante
	if (prio < MIN_PRIORITY || prio > esecuzione->precedenza) {
		flog(LOG_WARN, "activate_p: priorita' non valida: %u", prio);
		c_abort_p();
		return;
	}

	// controlliamo che 'liv' contenga un valore ammesso
	// [segnalazione di E. D'Urso]
	if (liv != LIV_UTENTE && liv != LIV_SISTEMA) {
		flog(LOG_WARN, "activate_p: livello non valido: %u", liv);
		c_abort_p();
		return;
	}

	// non possiamo creare un processo di livello sistema mentre
	// siamo a livello utente
	if (liv == LIV_SISTEMA && liv_chiamante() == LIV_UTENTE) {
		flog(LOG_WARN, "activate_p: errore di protezione");
		c_abort_p();
		return;
	}

	// accorpiamo le parti comuni tra c_activate_p e c_activate_pe
	// nella funzione ausiliare crea_processo
	p = crea_processo(f, a, prio, liv);

	if (p != nullptr) {
		inserimento_lista(pronti, p);
		processi++;
		id = p->id;			// id del processo creato
						// (allocato da crea_processo)
		flog(LOG_INFO, "proc=%u entry=%p(%lu) prio=%u liv=%u", id, f, a, prio, liv);
	}

	esecuzione->contesto[I_RAX] = id;
}

/// @brief Parte C++ della pritimiva terminate_p()
/// @param logmsg	set true invia un messaggio sul log
/// @note  Quando è invocata come primitiva logmsg viene posto forzatamente
/// a true (si veda @ref a_terminate_p in sistema.s).
extern "C" void c_terminate_p(bool logmsg)
{
	des_proc* p = esecuzione;

	if (logmsg)
		flog(LOG_INFO, "Processo %u terminato", p->id);
	distruggi_processo(p);
	processi--;
	schedulatore();
}

/*! @brief Distrugge il processo puntato da esecuzione
 *
 * @note Parte C++ della primitiva abort_p() e invocabile direttamente
 * dalle primitive atomiche.
 *
 * Fuziona come la terminate_p(), ma invia anche un warning al log.  La
 * funzione va invocata quando si vuole terminare un processo segnalando che
 * c'è stato un errore.
 *
 * @param selfdump	se true mostra sul log lo stato del processo
 *
 * @note Quando viene invocata come primitiva selfdump viene posto forzatamente
 * a true (si veda @ref a_abort_p in sistema.s)
 */
extern "C" void c_abort_p(bool selfdump)
{
	des_proc* p = esecuzione;

	if (esecuzione->livello == LIV_SISTEMA) {
		panic("abort di un processo di sistema");
	}

	if (selfdump) {
		dump_status(LOG_WARN);
	}
	flog(LOG_WARN, "Processo %u abortito", p->id);
	c_terminate_p(/* logmsg= */ false);
}
/// @}
/// @}
/// @}

/////////////////////////////////////////////////////////////////////////////////
/// @addtogroup sysio		Supporto per il modulo I/O
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/modulo-io.pdf>
///
/// @{
/////////////////////////////////////////////////////////////////////////////////

/*! @brief Carica un handler nella IDT.
 *
 *  Fa in modo che l'handler _irq_ -esimo sia associato alle richieste
 *  di interruzione provenienti dal piedino _irq_ dell'APIC. L'associazione
 *  avviene tramite l'entrata _tipo_ della IDT.
 *
 *  @param tipo 	tipo dell'interruzione a cui associare l'handler
 *  @param irq		piedino dell'APIC associato alla richiesta di interruzione
 */
extern "C" bool load_handler(natq tipo, natq irq);

/// @addtogroup sysiosyscall	Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva activate_pe().
 *  @param f		corpo del processo
 *  @param a		parametro per il corpo del processo
 *  @param prio		priorità del processo
 *  @param liv		livello del processo (LIV_UTENTE o LIV_SISTEMA)
 *  @param irq		IRQ gestito dal processo
 */
extern "C" void c_activate_pe(void f(natq), natq a, natl prio, natl liv, natb irq)
{
	des_proc*	p;			// des_proc per il nuovo processo
	natw		tipo;			// entrata nella IDT

	esecuzione->contesto[I_RAX] = 0xFFFFFFFF;

	if (prio < MIN_EXT_PRIO || prio > MAX_EXT_PRIO) {
		flog(LOG_WARN, "activate_pe: priorita' non valida: %u", prio);
		return;
	}
	// controlliamo che 'liv' contenga un valore ammesso
	// [segnalazione di F. De Lucchini]
	if (liv != LIV_UTENTE && liv != LIV_SISTEMA) {
		flog(LOG_WARN, "activate_pe: livello non valido: %u", liv);
		return;
	}
	// controlliamo che 'irq' sia valido prima di usarlo per indicizzare
	// 'a_p'
	if (irq >= apic::MAX_IRQ) {
		flog(LOG_WARN, "activate_pe: irq %hhu non valido (max %u)", irq, apic::MAX_IRQ);
		return;
	}
	// se a_p è non-nullo, l'irq è già gestito da un altro processo
	// esterno o da un driver
	if (a_p[irq]) {
		flog(LOG_WARN, "activate_pe: irq %hhu occupato", irq);
		return;
	}
	// anche il tipo non deve essere già usato da qualcos'altro.
	// Controlliamo quindi che il gate corrispondente non sia marcato
	// come presente (bit P==1)
	tipo = prio - MIN_EXT_PRIO;
	if (gate_present(tipo)) {
		flog(LOG_WARN, "activate_pe: tipo %hx occupato", tipo);
		return;
	}

	p = crea_processo(f, a, prio, liv);
	if (p == 0)
		return;

	// creiamo il collegamento irq->tipo->handler->processo esterno
	// irq->tipo (tramite l'APIC)
	apic::set_VECT(irq, tipo);
	// associazione tipo->handler (tramite la IDT)
	// Nota: in sistema.s abbiamo creato un handler diverso per ogni
	// possibile irq. L'irq che passiamo a load_handler serve ad
	// identificare l'handler che ci serve.
	load_handler(tipo, irq);
	// associazione handler->processo esterno (tramite 'a_p')
	a_p[irq] = p;

	// ora che tutti i collegamenti sono stati creati possiamo
	// iniziare a ricevere interruzioni da irq. Smascheriamo
	// dunque le richieste irq nell'APIC
	apic::set_MIRQ(irq, false);

	flog(LOG_INFO, "estern=%u entry=%p(%lu) prio=%u (tipo=%2x) liv=%u irq=%hhu",
			p->id, f, a, prio, tipo, liv, irq);

	esecuzione->contesto[I_RAX] = p->id;
	return;
}

/*! @brief Parte C++ della primitiva fill_gate().
 *  @param tipo		tipo del gate da riempire
 *  @param routine	funzione da associare al gate
 *  @param liv		DPL del gate (LIV_UTENTE o LIV_SISTEMA)
 */
extern "C" void c_fill_gate(natb tipo, void routine(), int liv)
{
	esecuzione->contesto[I_RAX] = false;

	if ( (tipo & 0xF0) != 0x40) {
		flog(LOG_WARN, "fill_gate: tipo non valido %#02x (deve essere 0x4*)", tipo);
		return;
	}

	if (gate_present(tipo)) {
		flog(LOG_WARN, "fill_gate: gate %#02x occupato", tipo);
		return;
	}

	if (liv != LIV_UTENTE && liv != LIV_SISTEMA) {
		flog(LOG_WARN, "fill_gate: livello %d non valido", liv);
		return;
	}

	gate_init(tipo, routine, true /* tipo trap */, liv);

	esecuzione->contesto[I_RAX] = true;
}

/*! @brief Parte C++ della primitiva phys_alloc().
 *  @param size		numero di byte da allocare
 *  @param align	allineamento richiesto
 */
extern "C" void c_phys_alloc(size_t size, std::align_val_t align)
{
	esecuzione->contesto[I_RAX] = int_cast<natq>(alloc_aligned(size, align));
}

/*! @brief Parte C++ della primitiva phys_dealloc().
 *  @param ptr		puntatore alla memoria da deallocare
 */
extern "C" void c_phys_dealloc(void *ptr)
{
	dealloc(ptr);
}
/// @}
/// @}

///////////////////////////////////////////////////////////////////////////////////
/// @addtogroup init		Inizializzazione
///
/// La funzione main viene eseguita in un processo speciale (init) che non viene
/// creato tramite crea_processo(), ma usa direttamente alcune risorse già allocate.
/// In particolare, usa una pila sistema allocata staticamente e la root table del
/// TRIE creato dal boot loader. Il suo descrittore di processo è allocato direttamente
/// sulla pila sistema. Il processo init resta in vita fino allo shutdown e,
/// una volta completate le inizializzazioni, svolge il ruolo del processo dummy.
///
/// @{
///////////////////////////////////////////////////////////////////////////////////

/// Periodo del timer di sistema.
const natl DELAY = 59659;

/// Tipo degli entry point dei moduli I/O e utente
typedef void (*entry_t)(natq);

/// @cond
entry_t carica_modulo(boot64_modinfo* mod, paddr root_tab, natq flags, natq heap_size);
/// @endcond

/*! @brief Cede il controllo ad un altro processo.
 *
 * Il processo corrente viene insierito in testa alla coda pronti.
 * @note La funzione è pensata per essere usata da init.
 *
 * @param p		processo a cui cedere il controllo
 */
extern "C" void cedi_controllo(des_proc* p);

/// @cond
void dummy(natq) {}
/// @endcond

/*! @brief Inizializza il sistema e poi esegue il ciclo dummy fino allo shutdown.
 */
extern "C" void main(natq)
{
	des_proc init,			// descrittore del processo init
		*main_io,		// processo di inizializzazione del modulo I/O
		*main_utente;		// primo processo utente
	entry_t mio_entry,		// entry point del modulo I/O
		utn_entry;		// entry point del modulo utente
	volatile int io_init_done = 0;	// sincronizzazione con il modulo I/O


	// nota: la funzione main è chimata da start (definita in libce).
	// Prima di chiamare main, start ha già chiamato init_idt.
	// In questo momento la IDT è inizializzata, ma le interruzioni
	// esterne sono ancora disabilitate. Durante l'esecuzione di questa
	// funzione saranno abilitate temporaneamente solo quando si esegue
	// la funzione halt().

	// azzeriamo il des_proc di init
	memset(&init, 0, sizeof(init));
	// Allochiamo un id per il processo (comparirà nei log da questo momento in poi)
	// Nota: l'id di init, il primo che allochiamo, sarà zero.
	// In questo modo il valore 0 può essere usato come valore non valido nelle strutture
	// dati che devono memorizzare un id di processo (in particolare negli esercizi),
	// in quanto init lo tiene occupato e dunque non assegnabile ad altri processi.
	if ( (init.id = alloca_proc_id(&init)) != 0 ) {
		flog(LOG_ERR, "Errore nell'allocazione del primo id (%u invece di 0)", init.id);
		goto error;
	}
	// Diamo direttamente la precedenza del processo dummy
	init.precedenza = DUMMY_PRIORITY;
	// Il processo iniziale usa il TRIE allocato dal boot loader
	// (che in questo momento mappa solo la finestra FM)
	init.cr3 = readCR3();
	// Il corpo del processo è questa funzione main
	init.corpo = main;
	// Il processo che abbiamo appena creato è l'unico in esecuzione
	esecuzione = &init;
	esecuzione_precedente = esecuzione;

	flog(LOG_INFO, "Nucleo di Calcolatori Elettronici, v8.3");

	// inizializziamo la parte M2 della memoria
	init_frame();
	flog(LOG_INFO, "Numero di frame: %lu (M1) %lu (M2)", N_M1, N_M2);

	flog(LOG_INFO, "Suddivisione della memoria virtuale:");
	flog(LOG_INFO, "- sis/cond [%16lx, %16lx)", ini_sis_c, fin_sis_c);
	flog(LOG_INFO, "- sis/priv [%16lx, %16lx)", ini_sis_p, fin_sis_p);
	flog(LOG_INFO, "- io /cond [%16lx, %16lx)", ini_mio_c, fin_mio_c);
	flog(LOG_INFO, "- usr/cond [%16lx, %16lx)", ini_utn_c, fin_utn_c);
	flog(LOG_INFO, "- usr/priv [%16lx, %16lx)", ini_utn_p, fin_utn_p);

	// Le parti sis/priv e usr/priv verranno create da crea_processo() ogni
	// volta che si attiva un nuovo processo.  La parte sis/cond contiene
	// la finestra FM, creata dal boot loader.  Le parti io/cond e usr/cond
	// devono contenere i segmenti ELF dei moduli I/O e utente,
	// rispettivamente. In questo momento le copie di questi due file ELF
	// si trovano in memoria nel secondo MiB (caricate dal primo boot
	// loader e non toccate dal secondo).  Estraiamo i loro segmenti codice
	// e dati e li copiamo in frame di M2, creando contestualmente le
	// necessarie traduzioni.  Creiamo queste traduzioni una sola volta
	// all'avvio (adesso) e poi le condividiamo tra tutti i processi.
	flog(LOG_INFO, "Carico il modulo I/O");
	if ( (mio_entry = carica_modulo(&boot_info->mod[1], init.cr3, 0, DIM_IO_HEAP)) == nullptr )
		goto error;
	flog(LOG_INFO, "Carico il modulo utente");
	if ( (utn_entry = carica_modulo(&boot_info->mod[2], init.cr3, BIT_US, DIM_USR_HEAP)) == nullptr )
		goto error;
	flog(LOG_INFO, "Frame liberi: %lu (M2)", num_frame_liberi);

	// a questo punto la copia dei moduli nel secondo MiB non serve più
	// e possiamo aggiungere tutta quella memoria allo heap di sistema
	heap_init(voidptr_cast(1*MiB), 1*MiB);
	flog(LOG_INFO, "Heap del modulo sistema: aggiunto [%llx, %llx)", 1*MiB, 2*MiB);

	// attiviamo il timer, in modo che i processi di inizializzazione
	// possano usare anche delay(), se ne hanno bisogno.
	// occupiamo a_p[2] (in modo che non possa essere sovrascritta
	// per errore tramite activate_pe()) e smascheriamo il piedino
	// 2 dell'APIC
	flog(LOG_INFO, "Attivo il timer (DELAY=%u)", DELAY);
	a_p[2] = ESTERN_BUSY;
	apic::set_VECT(2, INTR_TIPO_TIMER);
	apic::set_MIRQ(2, false);
	timer::start0(DELAY);

	// inizializziamo il modulo I/O creando un processo che esegue la sua
	// procedura start. Passiamo l'indirizzo di io_init_done come parametro.
	// Il processo deve scrivere un valore diverso da zero in questa variable
	// quando ha terminato l'inizializzazione.
	flog(LOG_INFO, "Creo il processo main I/O");
	main_io = crea_processo(mio_entry, int_cast<natq>(&io_init_done), MAX_EXT_PRIO, LIV_SISTEMA);
	if (main_io == nullptr) {
		flog(LOG_ERR, "impossibile creare il processo main I/O");
		goto error;
	}
	processi++;
	flog(LOG_INFO, "Attendo inizializzazione modulo I/O...");
	// cediamo il controllo al modulo I/O e aspettiamo che setti
	// la variable io_init_done
	cedi_controllo(main_io);
	while (!io_init_done)
		halt();	// abilita temporaneamente le interruzioni esterne

	// creazione del processo main utente
	flog(LOG_INFO, "Creo il processo main utente");
	main_utente = crea_processo(utn_entry, 0, MAX_PRIORITY, LIV_UTENTE);
	if (main_utente == nullptr) {
		flog(LOG_ERR, "impossibile creare il processo main utente");
		goto error;
	}
	processi++;

	flog(LOG_INFO, "Cedo il controllo al processo main utente...");
	// cediamo il controllo al processo main utente e giochiamo il ruolo
	// del processo dummy
	init.corpo = dummy; // per il debugger
	cedi_controllo(main_utente);
	while (processi)
		halt();
	// non ci sono più processi utente: ritorniamo a start che eseguirà
	// lo shutdown (nota: start è definita in libce)
	flog(LOG_INFO, "Shutdown");
	return;

error:
	panic("Errore di inizializzazione");
}

///////////////////////////////////////////////////////////////////////////////////
/// @addtogroup mod	Caricamento dei moduli I/O e utente
///
/// All'avvio troviamo il contenuto dei file `sistema`, `io` e `utente` già
/// copiati in memoria dal primo boot loader (quello realizzato da QEMU
/// stesso). Il secondo boot loader (quello della libce) ha caricato nella
/// posizione finale solo il modulo sistema e ha creato la riempito la
/// struttura boot64_info con le informazioni necessarie per caricare gli altri
/// moduli. Ora il modulo sistema deve rendere operativi anche i moduli IO e
/// utente, utilizzando le informazioni in boot64_info e creando le necessarie
/// traduzioni nella memoria virtuale.
/// Per ogni modulo, boot64_info contiene:
///
/// - mod_start: indirizzo fisico a partire dal quale è stato copiato il modulo
/// - segments[]: vettore che descrive i segmenti del modulo
///
/// Ogni elemento di _segments_ contiene le seguenti informazioni:
///
/// - offset:  inizio del segmento all'interno del modulo
/// - size:    numero di byte del segmento contenuti nel modulo
/// - vaddr:   indirizzo virtuale che il segmento dovrà avere
/// - memsize: dimensione che dovrà avere il segmento in memoria virtuale
/// - flags:   flag da settare nella traduzione (principalmente BIT_RW)
///
/// Il segmento deve occupare gli indirizzi virtuali _[vaddr, vaddr + memsize)_.
/// I byte che ora si trovano in _[mod_start + offset, mod_start + offset + size)_
/// dovranno diventare visibili a partire dall'indirizzo virtuale _vaddr_.
/// Si noti che _memsize_ può essere più grande di _size_, e in quel caso i
/// byte eccedenti devono essere azzerati.
///
/// Per poter creare le traduzioni dobbiamo copiare i byte del segmento da dove
/// si trovano ora in dei frame di M2, per almeno due motivi: la copia attuale
/// potrebbe non essere allineata correttamente; inoltre, potremmo non avere
/// spazio per azzerare l'eventuale parte eccedente.
///
/// @{
///////////////////////////////////////////////////////////////////////////////////

/// Oggetto da usare con map() per caricare un segmento in memoria virtuale.
struct copy_segment {
	// Il segmento si trova in memoria agli indirizzi (fisici) [mod_beg, mod_end)
	// e deve essere visibile in memoria virtuale a partire dall'indirizzo
	// virt_beg. Il segmento verrà copiato (una pagina alla volta) in
	// frame liberi di M2. La memoria precedentemente occupata dal modulo
	// sarà poi riutilizzata per lo heap di sistema.

	/// base del segmento in memoria fisica
	paddr mod_beg;
	/// limite del segmento in memoria fisica
	paddr mod_end;
	/// indirizzo virtuale della base del segmento
	vaddr virt_beg;

	paddr operator()(vaddr);
};

/*! @brief Funzione chiamata da map().
 *
 *  Copia il prossimo frame di un segmento in un frame di M2.
 *  @param v		indirizzo virtuale da mappare
 *  @return		indirizzo fisico del frame di M2
 */
paddr copy_segment::operator()(vaddr v)
{
	// allochiamo un frame libero in cui copiare la pagina
	paddr dst = alloca_frame();
	if (dst == 0)
		return 0;

	// offset della pagina all'interno del segmento
	natq offset = v - virt_beg;
	// indirizzo della pagina all'interno del modulo
	paddr src = mod_beg + offset;

	// il segmento in memoria può essere più grande di quello nel modulo.
	// La parte eccedente deve essere azzerata.
	natq tocopy = DIM_PAGINA;
	if (src > mod_end)
		tocopy = 0;
	else if (mod_end - src < DIM_PAGINA)
		tocopy =  mod_end - src;
	if (tocopy > 0)
		memcpy(voidptr_cast(dst), voidptr_cast(src), tocopy);
	if (tocopy < DIM_PAGINA)
		memset(voidptr_cast(dst + tocopy), 0, DIM_PAGINA - tocopy);
	return dst;
}

/*! @brief Carica un modulo in M2.
 *
 *  Copia il modulo in M2, lo mappa al suo indirizzo virtuale e
 *  aggiunge lo heap dopo l'ultimo indirizzo virtuale usato.
 *
 *  @param mod		informazioni sul modulo caricato dal boot loader
 *  @param root_tab	indirizzo fisico della radice del TRIE
 *  @param flags	BIT_US per rendere il modulo accessibile da livello utente,
 *  			altrimenti 0
 *  @param heap_size	dimensione dello heap (in byte)
 *  @return		entry point del modulo, o nullptr in caso di errore
 */
entry_t carica_modulo(boot64_modinfo* mod, paddr root_tab, natq flags, natq heap_size)
{
	vaddr last_vaddr = 0;
	// esaminiamo tutta la tabella dei segmenti
	for (natq i = 0; i < mod->numseg; i++) {
		boot64_segment *s = &mod->segments[i];

		// i byte che si trovano ora in memoria agli indirizzi (fisici)
		// [mod_beg, mod_end) devono diventare visibili nell'intervallo
		// di indirizzi virtuali [virt_beg, virt_end).
		paddr	mod_beg  = mod->mod_start + s->offset,
			mod_end  = mod_beg + s->size;
		vaddr	virt_beg = s->vaddr,
			virt_end = s->vaddr + s->memsize;

		// se necessario, allineiamo alla pagina gli indirizzi di
		// partenza e di fine
		natq page_offset = virt_beg & (DIM_PAGINA - 1);
		virt_beg -= page_offset;
		mod_beg  -= page_offset;
		virt_end = allinea(virt_end, DIM_PAGINA);

		// aggiorniamo l'ultimo indirizzo virtuale usato
		if (virt_end > last_vaddr)
			last_vaddr = virt_end;

		// I flag devono essere quelli richiesti per il modulo (utente
		// o sistema) più quelli specifici del segmento (che può essere
		// scrivibile o meno)
		natq seg_flags = flags | s->flags;

		// mappiamo il segmento
		if (map(root_tab,
			virt_beg,
			virt_end,
			seg_flags,
			copy_segment{mod_beg, mod_end, virt_beg}) != virt_end)
			return nullptr;

		flog(LOG_INFO, " - segmento %s %s mappato a [%16lx, %16lx)",
				(seg_flags & BIT_US) ? "utente " : "sistema",
				(seg_flags & BIT_RW) ? "read/write" : "read-only ",
				virt_beg, virt_end);
	}
	// dopo aver mappato tutti i segmenti, mappiamo lo spazio destinato
	// allo heap del modulo. I frame corrispondenti verranno allocati da
	// alloca_frame()
	if (map(root_tab,
		last_vaddr,
		last_vaddr + heap_size,
		flags | BIT_RW,
		[](vaddr) { return alloca_frame(); }) != last_vaddr + heap_size)
		return nullptr;
	flog(LOG_INFO, " - heap:                                 [%16lx, %16lx)",
				last_vaddr, last_vaddr + heap_size);
	flog(LOG_INFO, " - entry point: 0x%lx", mod->entry_point);
	return reinterpret_cast<entry_t>(mod->entry_point);
}
/// @}
/// @}

///////////////////////////////////////////////////////////////////////////////////
/// @addtogroup err 	Gestione errori
/// @brief Funzioni chiamate in situazioni di errore.
/// @{
///////////////////////////////////////////////////////////////////////////////////

/*! @brief Ferma il sistema e stampa lo stato di tutti i processi
 *  @param msg		messaggio da inviare al log (severità LOG_ERR)
 */
extern "C" void panic(const char* msg)
{
	static int in_panic = 0;

	if (in_panic) {
		flog(LOG_ERR, "panic ricorsivo. STOP");
		end_program();
	}
	in_panic = 1;

	flog(LOG_ERR, "PANIC: %s", msg);
	if (esecuzione_precedente) {
		flog(LOG_ERR, "  processi: %u", processi);
		flog(LOG_ERR, "------------------------------ PROCESSO IN ESECUZIONE -------------------------------");
		flog(LOG_ERR, "corpo %p(%lu), livello %s, precedenza %u", esecuzione->corpo, esecuzione->parametro,
				esecuzione->livello == LIV_UTENTE ? "UTENTE" : "SISTEMA",
				esecuzione->precedenza);
		dump_status(LOG_ERR);
		flog(LOG_ERR, "---------------------------------- ALTRI PROCESSI -----------------------------------");
		for (natl id = 0; id < MAX_PROC; id++) {
			if (proc_table[id] && proc_table[id] != esecuzione_precedente)
				process_dump(proc_table[id], LOG_ERR);
		}
	}
	end_program();
}

/*! @brief Routine di risposta a un non-maskable-interrupt
 *
 *  La routine ferma il sistema e stampa lo stato di tutti i processi.
 *
 *  @note Il sito dell'autocorrezione invia un nmi se il programma da
 *  testare non termina entro il tempo prestabilito.
 */
extern "C" void c_nmi()
{
	panic("INTERRUZIONE FORZATA");
}
/// @}

///////////////////////////////////////////////////////////////////////////////////
/// @addtogroup debug 	Supporto per il debugging
/// @brief Funzioni che forniscono informazioni sullo stato del sistema.
/// @{
///////////////////////////////////////////////////////////////////////////////////

#ifdef AUTOCORR
int MAX_LOG = 4;
#else
/// Massimo livello ammesso per la severità dei messaggi del log
int MAX_LOG = 5;
#endif

/// @addtogroup debugsyscall Parti/C++ Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva do_log().
 *  @param sev		severità del messaggio
 *  @param buf		buffer che contiene il messaggio
 *  @param quanti	lunghezza del messaggio in byte
 */
extern "C" void c_do_log(log_sev sev, const char* buf, natl quanti)
{
	if (liv_chiamante() == LIV_UTENTE) {
		if (!c_access(int_cast<vaddr>(buf), quanti, false, false)) {
			flog(LOG_WARN, "log: parametri non validi");
			c_abort_p();
			return;
		}
		if (sev == LOG_ERR)
			sev = LOG_WARN;
	}
	if (sev > MAX_LOG) {
		flog(LOG_WARN, "log: livello di warning errato");
		c_abort_p();
		return;
	}
	do_log(sev, buf, quanti);
}

/// Parte C++ della primitiva getmeminfo().
extern "C" void c_getmeminfo()
{
	meminfo m;

	// byte liberi nello heap di sistema
	m.heap_libero = disponibile();
	// numero di frame nella lista dei frame liberi
	m.num_frame_liberi = num_frame_liberi;
	// id del processo in esecuzione
	m.pid = esecuzione->id;

	memcpy(&esecuzione->contesto[I_RAX], &m, sizeof(natq));
	memcpy(&esecuzione->contesto[I_RDX], &m.pid, sizeof(natq));
}
/// @}

#include <cfi.h>

/*! @brief Callback invocata dalla funzione cfi_backstep() per leggere
 *         dalla pila di un qualunque processo.
 *  @param token 	(opaco) descrittore del processo di cui si
 *  			sta producendo il backtrace
 *  @param v		indirizzo virtuale da leggere
 *  @return		natq letto da _v_ nella memoria virtuale del
 *  			processo (0 se non mappato)
 */
natq read_mem(void* token, vaddr v)
{
	des_proc* p = static_cast<des_proc*>(token);
	paddr pa = trasforma(p->cr3, v);
	natq rv = 0;
	if (pa) {
		memcpy(&rv, voidptr_cast(pa), sizeof(rv));
	}
	return rv;
}

/*! @brief Invia sul log lo stato di un processo
 *  @param p		descrittore del processo
 *  @param sev		severità dei messaggi da inviare al log
 */
void process_dump(des_proc* p, log_sev sev)
{
	flog(sev, "proc %u: corpo %p(%lu), livello %s, precedenza %u", p->id, p->corpo, p->parametro,
			p->livello == LIV_UTENTE ? "UTENTE" : "SISTEMA", p->precedenza);

	cfi_d cfi;
	memset(&cfi, 0, sizeof(cfi));
	cfi.regs[CFI::RAX] = p->contesto[I_RAX];
	cfi.regs[CFI::RCX] = p->contesto[I_RCX];
	cfi.regs[CFI::RDX] = p->contesto[I_RDX];
	cfi.regs[CFI::RBX] = p->contesto[I_RBX];
	cfi.regs[CFI::RBP] = p->contesto[I_RBP];
	cfi.regs[CFI::RSI] = p->contesto[I_RSI];
	cfi.regs[CFI::RDI] = p->contesto[I_RDI];
	cfi.regs[CFI::R8]  = p->contesto[I_R8];
	cfi.regs[CFI::R9]  = p->contesto[I_R9];
	cfi.regs[CFI::R10] = p->contesto[I_R10];
	cfi.regs[CFI::R11] = p->contesto[I_R11];
	cfi.regs[CFI::R12] = p->contesto[I_R12];
	cfi.regs[CFI::R13] = p->contesto[I_R13];
	cfi.regs[CFI::R14] = p->contesto[I_R14];
	cfi.regs[CFI::R15] = p->contesto[I_R15];

	natq* pila = ptr_cast<natq>(trasforma(p->cr3, p->contesto[I_RSP]));

	if (pila) {
		cfi.rip            = pila[0];
		cfi.cs             = pila[1];
		cfi.flags          = pila[2];
		cfi.regs[CFI::RSP] = pila[3];

		cfi.token = p;
		cfi.read_stack = read_mem;
	} else {
		flog(sev, "  impossibile leggere la pila del processo");
	}

	cfi_dump(cfi, sev);
}
/// @}

// ( ESAME 2015-07-02


/// Descrittore di SHMEM
struct des_shmem {
	/// Dimensione della SHMEM (in pagine)
	natl npag;
	/// Primo frame della lista
	natl first_frame_number;
};

/// Array dei descrittori di SHMEM
des_shmem array_desshmem[MAX_SHMEM];
/// Numero di descrittori di SHMEM allocati
natl next_shmem = 0;

/**
 * @brief Controlla la validità di un id di SHMEM
 *
 * @param id	id da controllare
 *
 * @return 	true se _id_ corrisponde ad un descrittore allocato,
 * 		false altrimenti
 */
bool shmem_valid(natl id)
{
	return id < next_shmem;
}

/// Parte C++ della primitiva shmem_create()
extern "C" void c_shmem_create(natq npag)
{
	esecuzione->contesto[I_RAX] = 0xFFFFFFFF;

	if (next_shmem >= MAX_SHMEM) {
		flog(LOG_WARN, "shmem terminate");
		return;
	}
	natl id = next_shmem++;
	des_shmem *sh = &array_desshmem[id];
	natl *list = &sh->first_frame_number;
	for (natq i = 0; i < npag; i++) {
		paddr p = alloca_frame();
		natl fn = p / DIM_PAGINA;
		*list = fn;

		if (p == 0) {
			flog(LOG_WARN, "memoria terminata");
			goto error;
		}

		list = &vdf[fn].next_shmem;
	}
	*list = 0;
	sh->npag = npag;
	esecuzione->contesto[I_RAX] = id;
	return;

error:
	for (natl fn = sh->first_frame_number; fn; fn = vdf[fn].next_shmem)
		rilascia_frame(fn * DIM_PAGINA);
}
// ( SOLUZIONE 2015-07-02
/// Parte C++ della primitiva shmem_attach()
extern "C" void c_shmem_attach(natl id) {
	// controlli
	if(!shmem_valid(id)) {
		flog(LOG_WARN, "shmem_attach: id shmem inesistente");
		c_abort_p();
		return;
	}

	// ottieni la shmem
	des_shmem* ds = &array_desshmem[id];

	// vogliamo installare la shmem da avail_addr in poi,
	// fino a npag
	vaddr beg_addr = esecuzione->avail_addr;
	natl npag = ds->npag;
	vaddr end_addr = beg_addr + npag * DIM_PAGINA;

	// iniziamo a mappare da first_frame_number
	natl fnum = ds->first_frame_number;

	// prova a mappare
	vaddr v = map(
		esecuzione->cr3,
		beg_addr,
		end_addr,
		BIT_US | BIT_RW,
		[&](vaddr) {
			natl temp = fnum;
			fnum = vdf[fnum].next_shmem;
			return temp * DIM_PAGINA;
		}
	);

	if(v != end_addr) {
		// ripulisci
		unmap(
			esecuzione->cr3,
			beg_addr,
			end_addr,
			[](vaddr, paddr, int) {}
		);

		esecuzione->contesto[I_RAX] = 0;
		return;
	}

	esecuzione->contesto[I_RAX] = beg_addr;

	// il prossimo mappabile è end_addr
	esecuzione->avail_addr = end_addr;
}
//   SOLUZIONE 2015-07-02 )
//   ESAME 2015-07-02 )
/// @}

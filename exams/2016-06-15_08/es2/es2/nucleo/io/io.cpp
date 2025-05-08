/// @file io.cpp
/// @brief Parte C++ del modulo I/O
/// @addtogroup io Modulo I/O
///
/// Dispensa: <https://calcolatori.iet.unipi.it/resources/modulo-io.pdf>
///
/// @{
#include <costanti.h>
#include <libce.h>
#include <sys.h>
#include <sysio.h>
#include <io.h>

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup ioheap		Memoria Dinamica
/// @brief Funzioni per l'utilizzo dello heap da parte del modulo I/O
///
/// Dal momento che le funzioni del modulo I/O sono eseguite con le interruzioni
/// esterne mascherabili abilitate, dobbiamo proteggere lo heap I/O con un
/// semaforo di mutua esclusione.
///
/// @{
////////////////////////////////////////////////////////////////////////////////

/// Indice del semaforo di mutua esclusione per lo heap I/O
natl ioheap_mutex;

/*! @brief Alloca un oggetto nello heap I/O.
 *  @param s		dimensione dell'oggetto
 *  @return		puntatore all'oggetto (nullptr se heap esaurito)
 */
void* operator new(size_t s)
{
	void* p;

	sem_wait(ioheap_mutex);
	p = alloc(s);
	sem_signal(ioheap_mutex);

	return p;
}

/*! @brief Alloca un oggetto nello heap I/O, con vincoli di allineamento.
 *  @param s		dimensione dell'oggetto
 *  @param a		allineamento richiesto
 *  @return		puntatore all'oggetto (nullptr se heap esaurito)
 *
 *  Lo heap del modulo I/O si trova in memoria virtuale (parte I/O condivisa),
 *  quindi non possiamo garantire allineamenti maggiori di DIM_PAGINA.
 *  In quel caso ricorriamo a phys_alloc(), che usa lo heap del modulo sistema.
 */
void* operator new(size_t s, std::align_val_t a)
{
	void* p;

	if (static_cast<natq>(a) > DIM_PAGINA)
		return phys_alloc(s, a);

	sem_wait(ioheap_mutex);
	p = alloc_aligned(s, a);
	sem_signal(ioheap_mutex);
	return p;
}

/*! @brief Dealloca un oggetto restituendolo all'heap I/O.
 *  @param p		puntatore all'oggetto
 */
void operator delete(void* p)
{
	sem_wait(ioheap_mutex);
	dealloc(p);
	sem_signal(ioheap_mutex);
}

/**
 * @brief Dealloca un oggetto con vincoli di allineamento.
 *
 * @param p		puntatore all'oggetto
 * @param a		allineamento dell'oggetto
 */
void operator delete(void *p, std::align_val_t a)
{
	if (static_cast<natq>(a) > DIM_PAGINA)
		return phys_dealloc(p);

	sem_wait(ioheap_mutex);
	dealloc(p);
	sem_signal(ioheap_mutex);
}
/// @}

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup console 		Console
/// @brief Primitive per l'utilizzo di tastiera e video
///
/// @note consideriamo esclusivamente il caso di video in modalità testo.
///
/// @{
////////////////////////////////////////////////////////////////////////////////

/// Descrittore della console
struct des_console {
	/// Semaforo di mutua esclusione per l'accesso alla console
	natl mutex;
	/// Semafor di sincronizzazione (per le letture da tastiera)
	natl sincr;
	/// Dove scrivere il prossimo carattere letto
	char* punt;
	/// Quanti caratteri resta da leggere
	natq cont;
	/// Dimensione del buffer passato a @ref readconsole()
	natq dim;
};

/// Unica istanza di des_console
des_console console;

/// @addtogroup consolesyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C+++ della primitiva writeconsole()
 *  @param buff		buffer contenente i caratteri da scrivere
 *  @param quanti	numero di caratteri da scrivere
 */
extern "C" void c_writeconsole(const char* buff, natq quanti)
{
	des_console* p_des = &console;

	if (!access(buff, quanti, false, false)) {
		flog(LOG_WARN, "writeconsole: parametri non validi: %p, %lu:", buff, quanti);
		abort_p();
	}

	sem_wait(p_des->mutex);
#ifndef AUTOCORR
	for (natq i = 0; i < quanti; i++)
		vid::char_write(buff[i]);
#else /* AUTOCORR */
	if (quanti > 0 && buff[quanti - 1] == '\n')
		quanti--;
	if (quanti > 0)
		flog(LOG_USR, "%.*s", static_cast<int>(quanti), buff);
#endif /* AUTOCORR */
	sem_signal(p_des->mutex);
}
/// @}

/*! @brief Avvia una operazione di lettura dalla tastiera
 *  @param d		descrittore della console
 *  @param buff		buffer che deve ricevere i caratteri
 *  @param dim		dimensione di _buff_
 */
void startkbd_in(des_console* d, char* buff, natq dim)
{
	d->punt = buff;
	d->cont = dim;
	d->dim  = dim;
	kbd::enable_intr();
}

/// @addtogroup consolesyscall
/// @{

/*! @brief Parte C++ della primitiva readconsole()
 *  @param buff		buffer che deve ricevere i caratteri letti
 *  @param quanti	dimensione di _buff_
 *  @return		numero di caratteri effettivamente letti
 */
extern "C" natq c_readconsole(char* buff, natq quanti)
{
	des_console* d = &console;
	natq rv;

	if (!access(buff, quanti, true)) {
		flog(LOG_WARN, "readconsole: parametri non validi: %p, %lu:", buff, quanti);
		abort_p();
	}

#ifdef AUTOCORR
	return 0;
#endif

	if (!quanti)
		return 0;

	sem_wait(d->mutex);
	startkbd_in(d, buff, quanti);
	sem_wait(d->sincr);
	rv = d->dim - d->cont;
	sem_signal(d->mutex);
	return rv;
}
/// @}

/// Processo esterno associato alla tastiera
void estern_kbd(natq)
{
	des_console* d = &console;
	char a;
	bool fine;

	for(;;) {
		kbd::disable_intr();

		a = kbd::char_read_intr();

		fine = false;
		switch (a) {
		case 0:
			break;
		case '\b':
			if (d->cont < d->dim) {
				d->punt--;
				d->cont++;
				vid::str_write("\b \b");
			}
			break;
		case '\r':
		case '\n':
			fine = true;
			*d->punt = '\0';
			vid::str_write("\r\n");
			break;
		default:
			*d->punt = a;
			d->punt++;
			d->cont--;
			vid::char_write(a);
			if (d->cont == 0) {
				fine = true;
			}
			break;
		}
		if (fine)
			sem_signal(d->sincr);
		else
			kbd::enable_intr();
		wfi();
	}
}

/// @addtogroup consolesyscall
/// @{

/*! @brief Parte C++ della primitiva iniconsole()
 *  @param cc		Attributo colore per il video
 */
extern "C" void c_iniconsole(natb cc)
{
	vid::clear(cc);
}
/// @}

/// Piedino dell'APIC per le richieste di interruzione della tastiera
const int KBD_IRQ = 1;

/*! @brief Inizializza la tastiera
 *  @return		true in caso di successo, false altrimenti
 */
bool kbd_init()
{
	// blocchiamo subito le interruzioni generabili dalla tastiera
	kbd::disable_intr();

	// svuotiamo il buffer interno della tastiera
	kbd::drain();

	if (activate_pe(estern_kbd, 0, MIN_EXT_PRIO + INTR_TIPO_KBD, LIV_SISTEMA, KBD_IRQ) == 0xFFFFFFFF) {
		flog(LOG_ERR, "kbd: impossibile creare estern_kbd");
		return false;
	}
	flog(LOG_INFO, "kbd: tastiera inizializzata");
	return true;
}

/*! @brief Inizializza il video (modalità testo)
 *  @return		true in caso di successo, false altrimenti
 */
bool vid_init()
{
	vid::clear(0x07);
	flog(LOG_INFO, "vid: video inizializzato");
	return true;
}

/*! @brief Inizializza la console (tastiera + video)
 *  @return		true in caso di successo, false altrimenti
 */
bool console_init()
{
	des_console* d = &console;

	if ( (d->mutex = sem_ini(1)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "console: impossibile creare mutex");
		return false;
	}
	if ( (d->sincr = sem_ini(0)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "console: impossibile creare sincr");
		return false;
	}
	return kbd_init() && vid_init();
}
/// @}

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup ata 		Interfacce ATA
/// @brief Primitive per l'utilizzo dell'hard disk
/// @{
////////////////////////////////////////////////////////////////////////////////

/// Descrittore di interfaccia ATA
struct des_ata {
	/// Ultimo comando inviato all'interfaccia
	natb comando;
	/// Indice di un semaforo di mutua esclusione
	natl mutex;
	/// Indice di un semaforo di sincronizzazione
	natl sincr;
	/// Quanti settori resta da leggere o scrivere
	natb cont;
	/// Da dove leggere/dove scrivere il prossimo settore
	natb* punt;
	/// Array dei descrittori per il Bus Mastering
	natl* prd;
};

/// Descrittore dell'unico hard disk installato nel sistema
des_ata hard_disk;

/// Piedino dell'APIC per le richieste di interruzione dell'hard disk
const natb HD_IRQ = 14;

/*! @brief Prepara i descrittori per il Bus Mastering
 *  @param d		descrittore dell'hard disk
 *  @param vett		buffer coinvolto nel trasferimento DMA
 *  @param quanti	numero di settori da trasferire
 *  @return		false se i PRD non sono sufficienti per
 *  			trasferire tutti i byte richiesti,
 *  			true altrimenti
 */
bool prepare_prd(des_ata *d, natb* vett, natb quanti)
{
	natq n = quanti * DIM_BLOCK;
	int i = 0;

	while (n && i < MAX_PRD) {
		paddr p = trasforma(vett);
		natq  r = DIM_PAGINA - (p % DIM_PAGINA);
		if (r > n)
			r = n;
		d->prd[i] = p;
		d->prd[i + 1] = r;

		n -= r;
		vett += r;
		i += 2;
	}
	if (n)
		return false;
	d->prd[i - 1] |= 0x80000000;
	return true;
}

/*! @brief Avvia una operazione di ingresso dall'hard disk.
 *  @param d		descrittore dell'hard disk
 *  @param vetti	buffer che dovrà ricevere i settori letti
 *  @param primo	LBA del primo settore da leggere
 *  @param quanti	numero di settori da leggere
 */
void starthd_in(des_ata* d, natb vetti[], natl primo, natb quanti)
{
	d->cont = quanti;
	d->punt = vetti;
	d->comando = hd::READ_SECT;
	hd::start_cmd(primo, quanti, hd::READ_SECT);
}

/// @addtogroup atasyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva readhd_n().
 *  @param vetti	buffer che dovrà ricevere i settori letti
 *  @param primo	LBA del primo settore da leggere
 *  @param quanti	numero di settori da leggere
 */
extern "C" void c_readhd_n(natb vetti[], natl primo, natb quanti)
{
	des_ata* d = &hard_disk;

	if (!access(vetti, quanti * DIM_BLOCK, true)) {
		flog(LOG_WARN, "readhd_n: parametri non validi: %p, %d", vetti, quanti);
		abort_p();
	}

	if (!quanti)
		return;

	sem_wait(d->mutex);
	starthd_in(d, vetti, primo, quanti);
	sem_wait(d->sincr);
	sem_signal(d->mutex);
}
/// @}

/*! @brief Avvia una operazione di uscita verso l'hard disk.
 *  @param d		descrittore dell'hard disk
 *  @param vetto	buffer che contiene i settori da scrivere
 *  @param primo	LBA del primo settore da scrivere
 *  @param quanti	numero di settori da scrivere
 */
void starthd_out(des_ata* d, natb vetto[], natl primo, natb quanti)
{
	d->cont = quanti;
	d->punt = vetto + DIM_BLOCK;
	d->comando = hd::WRITE_SECT;
	hd::start_cmd(primo, quanti, hd::WRITE_SECT);
	hd::output_sect(vetto);
}

/// @addtogroup atasyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva writehd_n().
 *  @param vetto	buffer che contiene i settori da scrivere
 *  @param primo	LBA del primo settore da scrivere
 *  @param quanti	numero di settori da scrivere
 */
extern "C" void c_writehd_n(natb vetto[], natl primo, natb quanti)
{
	des_ata* d = &hard_disk;

	if (!access(vetto, quanti * DIM_BLOCK, false)) {
		flog(LOG_WARN, "writehd_n: parametri non validi: %p, %d", vetto, quanti);
		abort_p();
	}

	if (!quanti)
		return;

	sem_wait(d->mutex);
	starthd_out(d, vetto, primo, quanti);
	sem_wait(d->sincr);
	sem_signal(d->mutex);
}
/// @}

/*! @brief Avvia una operazione di ingresso in DMA dall'hard disk.
 *  @param d		descrittore dell'hard disk
 *  @param vetti	buffer che dovrà ricevere i settori letti
 *  @param primo	LBA del primo settore da leggere
 *  @param quanti	numero di settori da leggere
 */
void dmastarthd_in(des_ata* d, natb vetti[], natl primo, natb quanti)
{
	if (!prepare_prd(d, vetti, quanti)) {
		flog(LOG_ERR, "dmastarthd_in: numero di PRD insufficiente");
		sem_signal(d->sincr);
		return;
	}

	d->comando = hd::READ_DMA;
	d->cont = 1;
	paddr prd = trasforma(d->prd);
	bm::prepare(prd, false);
	hd::start_cmd(primo, quanti, hd::READ_DMA);
	bm::start();
}

/// @addtogroup atasyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva dmareadhd_n().
 *  @param vetti	buffer che dovrà ricevere i settori letti
 *  @param primo	LBA del primo settore da leggere
 *  @param quanti	numero di settori da leggere
 */
extern "C" void c_dmareadhd_n(natb vetti[], natl primo, natb quanti)
{
	des_ata* d = &hard_disk;

	if (quanti * DIM_BLOCK > MAX_PRD * DIM_PAGINA) {
		flog(LOG_WARN, "readhd_n: quanti %d troppo grande", quanti);
		abort_p();
	}

	if (!access(vetti, quanti * DIM_BLOCK, true)) {
		flog(LOG_WARN, "dmareadhd_n: parametri non validi: %p, %d", vetti, quanti);
		abort_p();
	}

	if (!quanti)
		return;

	sem_wait(d->mutex);
	dmastarthd_in(d, vetti, primo, quanti);
	sem_wait(d->sincr);
	sem_signal(d->mutex);
}
/// @}

/*! @brief Avvia una operazione di uscita in DMA verso l'hard disk.
 *  @param d		descrittore dell'hard disk
 *  @param vetto	buffer che contiene i settori da scrivere
 *  @param primo	LBA del primo settore da scrivere
 *  @param quanti	numero di settori da scrivere
 */
void dmastarthd_out(des_ata* d, natb vetto[], natl primo, natb quanti)
{
	if (!prepare_prd(d, vetto, quanti)) {
		flog(LOG_ERR, "dmastarthd_out: numero di PRD insufficiente");
		sem_signal(d->sincr);
		return;
	}

	d->comando = hd::WRITE_DMA;
	d->cont = 1; 				// informazione per il driver
	paddr prd = trasforma(d->prd);
	bm::prepare(prd, true);
	hd::start_cmd(primo, quanti, hd::WRITE_DMA);
	bm::start();
}

/// @addtogroup atasyscall Parti C++/Assembler delle primitive
/// @{

/*! @brief Parte C++ della primitiva dmawritehd_n().
 *  @param vetto	buffer che contiene i settori da scrivere
 *  @param primo	LBA del primo settore da scrivere
 *  @param quanti	numero di settori da scrivere
 */
extern "C" void c_dmawritehd_n(natb vetto[], natl primo, natb quanti)
{
	des_ata* d = &hard_disk;

	if (quanti * DIM_BLOCK > MAX_PRD * DIM_PAGINA) {
		flog(LOG_WARN, "readhd_n: quanti %d troppo grande", quanti);
		abort_p();
	}

	if (!access(vetto, quanti * DIM_BLOCK, false)) {
		flog(LOG_WARN, "dmawritehd_n: parametri non validi: %p, %d", vetto, quanti);
		abort_p();
	}

	if (!quanti)
		return;

	sem_wait(d->mutex);
	dmastarthd_out(d, vetto, primo, quanti);
	sem_wait(d->sincr);
	sem_signal(d->mutex);
}
/// @}

/// Processo esterno per le richieste di interruzione dell'hard disk
void estern_hd(natq)
{
	des_ata* d = &hard_disk;
	for(;;) {
		d->cont--;
		hd::ack();
		switch (d->comando) {
		case hd::READ_SECT:
			hd::input_sect(d->punt);
			d->punt += DIM_BLOCK;
			break;
		case hd::WRITE_SECT:
			if (d->cont != 0) {
				hd::output_sect(d->punt);
				d->punt += DIM_BLOCK;
			}
			break;
		case hd::READ_DMA:
		case hd::WRITE_DMA:
			bm::ack();
			break;
		}
		if (d->cont == 0)
			sem_signal(d->sincr);
		wfi();
	}
}

/*! @brief Inizializza il sottosistema per la gestione dell'hard disk
 *  @return		true in caso di successo, false altrimenti
 */
bool hd_init()
{
	natl id;
	natb bus = 0, dev = 0, fun = 0;
	des_ata* d;

	d = &hard_disk;

	if ( (d->mutex = sem_ini(1)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "hd: impossibile creare mutex");
		return false;
	}
	if ( (d->sincr = sem_ini(0)) == 0xFFFFFFFF) {
		flog(LOG_ERR, "hd: impossibile creare sincr");
		return false;
	}

	if (!bm::find(bus, dev, fun)) {
		flog(LOG_WARN, "hd: bus master non trovato");
		return false;
	}

	if ( (d->prd = new (std::align_val_t{65536}) natl[2 * MAX_PRD]) == nullptr) {
		flog(LOG_WARN, "hd: impossibile allocare vettore di PRD");
		return false;
	}

	flog(LOG_INFO, "bm: %02x:%02x.%1d", bus, dev, fun);
	bm::init(bus, dev, fun);

	id = activate_pe(estern_hd, 0, MIN_EXT_PRIO + INTR_TIPO_HD, LIV_SISTEMA, HD_IRQ);
	if (id == 0xFFFFFFFF) {
		flog(LOG_ERR, "hd: impossibile creare proc. esterno");
		return false;
	}

	hd::enable_intr();

	return true;
}
/// @}

// ( ESAME 2016-06-15


/// Descrittore di periferica CE.
struct des_ce {
	/// @name Indirizzi dei registri
	/// @{
	ioaddr iBMPTR,	///< Indirizzo del buffer di destinazione
	       iBMLEN,	///< Numero di byte da trasferire
	       iCMD,	///< Registro di comando
	       iSTS;	///< Registro di stato
	/// @}
	
	/// Indice del semaforo di sincronizzazione
	natl sync;
	/// Indice del semaforo di mutual esclusione
	natl mutex;
	/// Puntatore al buffer di destinazione
	char *buf;
	/// Numero di byte da trasferire
	natl quanti;
};

/// Array dei descrittori di periferica CE.
des_ce array_ce[MAX_CE];
/// Numero di descrittori di periferica CE allocati.
natl next_ce;

void leggi_pagina(natq id) {
	des_ce* d = &array_ce[id];
	
	// fisico prossimo elem.
	paddr p = trasforma(d->buf);

	// headroom nella pagina
	natq r = DIM_PAGINA - (p % DIM_PAGINA);

	// se hai più spazio del dovuto taglialo
	if (r > d->quanti)
		r = d->quanti;

	// imposta fisico e lunghezza
	outputl(p, d->iBMPTR);
	outputl(r, d->iBMLEN);

	// rimuovi quello che hai usato
	d->quanti -= r;
	d->buf += r;
	
	// inizia la lettura
	outputl(1, d->iCMD);
}

// qui si implementa la parte C++ della primitiva cedmaread()
extern "C" void c_cedmaread(natl id, char* buf, natl quanti) {
	flog(LOG_DEBUG, "cedmaread, id=%u, buf=%p, quanti=%u", id, buf, quanti);

	// controlli
	
	// l'id è valido?
	if(id >= next_ce) {
		flog(LOG_WARN, "cedmaread ha id invalido");
		abort_p();
	}
	
	// il buf è valido?
	if(!access(buf, quanti, true)) {
		flog(LOG_WARN, "cedmaread ha buf invalido");
		abort_p();
	}

	des_ce* d = &array_ce[id];

	// tutto ok, sincronizza
	sem_wait(d->mutex);

	// qui puoi fare la lettura
	d->buf = buf;
	d->quanti = quanti;

	leggi_pagina(id);

	sem_wait(d->sync);
	sem_signal(d->mutex);
}

// qui si implementa il processo esterno delle periferiche ce
void estern_ce(natq id) {
	des_ce* d = &array_ce[id];
	
	for(;;) {
		// ack alla periferica
		inputl(d->iSTS);

		if(!d->quanti) {
			// chiudi la sincronia
			sem_signal(d->sync);
		} else {
			// leggi la prossima pagina
			leggi_pagina(id);
		}
		
		wfi();
	}
}

/**
 * @brief Inizializza il sottosistema di gestione delle periferiche CE.
 *
 * Trova le periferiche CE installate. Per ciascuna dei esse inizializza
 * un descrittore e attiva un processo esterno.
 *
 * @return false in caso di errore, true altrimenti
 */
bool ce_init()
{

	for (natb bus = 0, dev = 0, fun = 0;
	     pci::find_dev(bus, dev, fun, 0xedce, 0x1234);
	     pci::next(bus, dev, fun))
	{
		if (next_ce >= MAX_CE) {
			flog(LOG_WARN, "troppi dispositivi ce");
			return false;
		}
		des_ce *ce = &array_ce[next_ce];
		ioaddr base = pci::read_confl(bus, dev, fun, 0x10);
		base &= ~0x1;
		ce->iBMPTR = base;
		ce->iBMLEN = base + 4;
		ce->iCMD   = base + 8;
		ce->iSTS   = base + 12;
		if ( (ce->sync = sem_ini(0)) == 0xFFFFFFFF ) {
			flog(LOG_WARN, "CE%u: impossibile allocare sync", next_ce);
			return false;
		}
		if ( (ce->mutex = sem_ini(1)) == 0xFFFFFFFF ) {
			flog(LOG_WARN, "CE%u: impossibile allocare mutex", next_ce);
			return false;
		}
		natb irq = pci::read_confb(bus, dev, fun, 0x3c);
		if (activate_pe(estern_ce, next_ce, MIN_EXT_PRIO + INTR_TIPO_CE + next_ce, LIV_SISTEMA, irq) == 0xFFFFFFFF) {
			flog(LOG_WARN, "CE%u: impossibile attivare processo esterno", next_ce);
			return false;
		}
		flog(LOG_INFO, "CE%u: %02x:%02x.%1x base=%04x IRQ=%u", next_ce, bus, dev, fun, base, irq);
		next_ce++;
	}
	return next_ce > 0;
}
//   ESAME 2016-06-15 )

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup ioinit		Inizializzazione
/// @{
////////////////////////////////////////////////////////////////////////////////

/// Ultimo indirizzo utilizzato dal modulo I/O (fornito dal collegatore)
extern "C" char _end[];

/*! @brief Riempie i gate della IDT relativi alle primitive fornite dal modulo I/O
 *  @return		true in caso di successo, false altrimenti
 */
extern "C" bool fill_io_gates();

/*! @brief Corpo del processo main I/O.
 *
 *  Il modulo sistema crea il processo main I/O all'avvio e gli cede
 *  il controllo, passandogli l'indirizzo di una variabile intera.
 *  Il modulo I/O deve scrivere 1 in questa variabile quando ha
 *  ha terminato la fase di inizializzazione.
 *
 *  @param p		indirizzo della variabile di sincronizzazione
 */
extern "C" void main_io(natq p)
{
	int *p_io_init_done = ptr_cast<int>(p);

	fill_io_gates();
	ioheap_mutex = sem_ini(1);
	if (ioheap_mutex == 0xFFFFFFFF) {
		flog(LOG_ERR, "impossible creare semaforo ioheap_mutex");
		abort_p();
	}
	char* end_ = allinea_ptr(_end, DIM_PAGINA);
	heap_init(end_, DIM_IO_HEAP);
	flog(LOG_INFO, "Heap del modulo I/O: %llxB [%p, %p)", DIM_IO_HEAP,
			end_, end_ + DIM_IO_HEAP);
	flog(LOG_INFO, "Inizializzo la console (kbd + vid)");
	if (!console_init()) {
		flog(LOG_ERR, "inizializzazione console fallita");
		abort_p();
	}
	flog(LOG_INFO, "Inizializzo la gestione dell'hard disk");
	if (!hd_init()) {
		flog(LOG_ERR, "inizializzazione hard disk fallita");
		abort_p();
	}
// [ ESAME 2016-06-15
	if (!ce_init()) {
		flog(LOG_ERR, "inizializzazione CE fallita");
		abort_p();
	}
//   ESAME 2016-06-15 ]
	*p_io_init_done = 1;
	terminate_p();
}
/// @}

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup iodbg		Supporto per il debugging
/// @brief Funzioni che forniscono informazioni sullo stato del sistema
/// @{
////////////////////////////////////////////////////////////////////////////////

/*! @brief Parte C++ della primitiva getiomeminfo()
 *  @return		byte liberi nello heap I/O
 */
extern "C" natq c_getiomeminfo()
{
	natq rv;
	sem_wait(ioheap_mutex);
	rv = disponibile();
	sem_signal(ioheap_mutex);
	return rv;
}
/// @}
/// @}

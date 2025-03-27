#include <all.h>

void lettore(natq) {
	flog(LOG_INFO, "lettore sta per leggere");
	natq msg = leggi();
	flog(LOG_INFO, "lettore ha letto %lu", msg);

	terminate_p();
}

void scrittore(natq) {
	flog(LOG_INFO, "scrittore sta per scrivere");
	scrivi(500);
	flog(LOG_INFO, "scrittore ha scritto 500");
	
	terminate_p();
}

void pulitore(natq) {
	flog(LOG_INFO, "pulitore sta per pulire");
	pulisci();
	flog(LOG_INFO, "pulitore ha pulito");

	terminate_p();
}

void main() {	
	// leggi
	activate_p(lettore, 0, 35, LIV_UTENTE);
	
	// prova a pulire ma fallisce
	activate_p(pulitore, 0, 30, LIV_UTENTE);

	// scrivi, cosi liberi la lettura precedente
	activate_p(scrittore, 0, 25, LIV_UTENTE);

	// ora puoi pulire
	activate_p(pulitore, 0, 20, LIV_UTENTE);

	// leggi, vai di nuovo in attesa
	activate_p(lettore, 0, 15, LIV_UTENTE);
	
	// scrivi, cosi liberi la lettura precedente
	activate_p(scrittore, 0, 10, LIV_UTENTE);
	
	// rileggi, non si è pulito quindi non vai in attesa 
	activate_p(lettore, 0, 5, LIV_UTENTE);

	terminate_p();
}

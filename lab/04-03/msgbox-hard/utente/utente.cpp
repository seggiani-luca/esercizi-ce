#include <all.h>

natl box;

int val = 16;

void scrittore(natq a) {
		msgbox_send(box, val);
		printf("[scrittore] inviato messaggio '%d'\n", val);

		// incrementa
		val++;
		
		flog(LOG_INFO, "scrittore ha terminato");
    terminate_p();
}

void lettore(natq i) {
    natl msg;
		
		msg = msgbox_recv(box);
		printf("[lettore %d] ricevuto messaggio '%d'\n", (int)i, msg);
		
		flog(LOG_INFO, "lettore ha terminato");
    terminate_p();
}

extern "C" void main()
{
    box = msgbox_init();
    flog(LOG_INFO, "[main] allocata msgbox %u", box);

		activate_p(scrittore, 0, 20, LIV_UTENTE); // 16, ci va
		activate_p(scrittore, 0, 20, LIV_UTENTE); // 17, ci va
		activate_p(scrittore, 0, 20, LIV_UTENTE); // 18, e' fuori, va messo in attesa
		
		activate_p(lettore, 0, 10, LIV_UTENTE); // lascia che il 18 scriva e legge 16

		activate_p(lettore, 0, 10, LIV_UTENTE); // legge 17
		activate_p(lettore, 0, 10, LIV_UTENTE); // legge 18
	
		activate_p(lettore, 0, 10, LIV_UTENTE); // va in attesa, evita che si chiuda tutto 
    
		terminate_p();
}

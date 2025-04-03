#include <all.h>

natl box;

void scrittore(natq a) {
		printf("[scrittore] inviato messaggio '%d'\n", 420);
		msgbox_send(box, 420);
		
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

		activate_p(lettore, 0, 40, LIV_UTENTE);
    flog(LOG_INFO, "[main] lettore");
		activate_p(lettore, 0, 40, LIV_UTENTE);
    flog(LOG_INFO, "[main] lettore");
		
		activate_p(scrittore, 0, 20, LIV_UTENTE);
    flog(LOG_INFO, "[main] scrittore 0");
		
		activate_p(scrittore, 0, 20, LIV_UTENTE);
    flog(LOG_INFO, "[main] scrittore 1");
		
		activate_p(scrittore, 0, 20, LIV_UTENTE);
    flog(LOG_INFO, "[main] scrittore 2");
		

    terminate_p();
}

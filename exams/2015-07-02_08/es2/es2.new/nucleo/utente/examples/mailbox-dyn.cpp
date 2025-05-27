/*
 * Mailbox con memoria dinamica
 */

#include <all.h>

const int NMESG = 5;
const int MSG_SIZE = 100;

natl mailbox_piena;
natl mailbox_vuota;
natl terminati;

struct mess {
	int mittente;
	char corpo[MSG_SIZE];
};

mess* mailbox;

void pms(natq a)
{
	mess *ptr;
	for (int i = 0; i < NMESG; i++) {
		ptr = new mess;
		if (!ptr) {
			flog(LOG_WARN, "memoria esaurita");
			break;
		}
		ptr->mittente = a;
		snprintf(ptr->corpo, MSG_SIZE, "Messaggio numero %d", i);
		sem_wait(mailbox_vuota);
		mailbox = ptr;
		sem_signal(mailbox_piena);
		delay(20);
	}
	printf("fine scrittore %lu\n", a);
	sem_signal(terminati);

	terminate_p();
}
void pml(natq a)
{
	mess *ptr;
	for (int i = 0; i < 2 * NMESG; i++) {
		sem_wait(mailbox_piena);
		ptr = mailbox;
		sem_signal(mailbox_vuota);
		printf("messaggio %d da %d: %s\n",
			i, ptr->mittente, ptr->corpo);
		delete ptr;
		ptr = 0;
	}
	printf("fine lettore\n");
	sem_signal(terminati);

	terminate_p();
}

extern "C" void main()
{
	if ( (mailbox_piena = sem_ini(0)) == 0xFFFFFFFF )
		fpanic("Impossibile creare semaforo mailbox_piena");
	if ( (mailbox_vuota = sem_ini(1)) == 0xFFFFFFFF )
		fpanic("Impossibile creare semaforo mailbox_vuota");
	if ( (terminati = sem_ini(0)) == 0xFFFFFFFF )
		fpanic("Impossibile creare semaforo terminati");
	if (activate_p(pms, 1, 5, LIV_UTENTE) == 0xFFFFFFFF)
		fpanic("Impossibile creare processo pms(1)");
	if (activate_p(pms, 2, 5, LIV_UTENTE) == 0xFFFFFFFF)
		fpanic("Impossibile creare processo pms(2)");
	if (activate_p(pml, 0, 5, LIV_UTENTE) == 0xFFFFFFFF)
		fpanic("Impossibile creare processo pml");
	for (int i = 0; i < 3; i++)
		sem_wait(terminati);
	pause();

	terminate_p();
}

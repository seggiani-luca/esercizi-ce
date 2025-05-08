/*
 * Programma di test 2016-06-15
 */

#include <all.h>

const int MSG_SIZE = 60;
const int NMESG = 1;

extern natl mailbox_piena;
extern natl mailbox_vuota;

extern natl scrittore1;

extern natl scrittore2;

extern natl lettore;
struct mess {
	int mittente;
	char corpo[MSG_SIZE];
};

mess mailbox;

char buf[2][MSG_SIZE * 100];

void pms(natq a)
{
	for (int i = 0; i < MSG_SIZE * 100; i++) {
		buf[a][i] = 'a' + a;
	}
	for (int i = 0; i < NMESG; i++) {
		cedmaread(0, buf[a], MSG_SIZE * 100);
		sem_wait(mailbox_vuota);
		for (int i = 0; i < MSG_SIZE - 1; i++) {
			mailbox.corpo[i] = buf[a][i * 100];
		}
		mailbox.corpo[MSG_SIZE - 1] = '\0';
		mailbox.mittente = a;
		sem_signal(mailbox_piena);
	}
	printf("fine scrittore %lu\n", a);

	terminate_p();
}
void pml(natq a)
{
	char corpo[MSG_SIZE];
	int mittente;
	for (int i = 0; i < 2 * NMESG; i++) {
		sem_wait(mailbox_piena);
		mittente = mailbox.mittente;
		memcpy(corpo, mailbox.corpo, sizeof(corpo));
		sem_signal(mailbox_vuota);
		printf("mittente=%d corpo=%s\n", mittente, corpo);
	}
	printf("fine lettore\n");
	pause();

	terminate_p();
}
natl mailbox_piena;
natl mailbox_vuota;
natl scrittore1;
natl scrittore2;
natl lettore;

extern "C" void main()
{
	mailbox_piena = sem_ini(0);
	mailbox_vuota = sem_ini(1);
	scrittore1 = activate_p(pms, 0, 6, LIV_UTENTE);
	scrittore2 = activate_p(pms, 1, 7, LIV_UTENTE);
	lettore = activate_p(pml, 0, 5, LIV_UTENTE);

	terminate_p();
}

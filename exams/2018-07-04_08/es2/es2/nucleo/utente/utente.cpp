#include <all.h>

struct msg {
	natl s;
	char p[MAX_PAYLOAD + 1];
};

#define MSG(src, dst, payload) { src, payload },

msg toreceive[] = {
#include "messages"
};


extern natl hello;

extern natl bad1;

extern natl bad2;

extern natl bad3;

extern natl bad4;

extern natl bad5;
void bad1_body(natq a)
{
	char buf[MAX_PAYLOAD + 1];
	natq len = MAX_PAYLOAD;
	receive(*(natl *)0x200000, buf, len);
	printf("processo errato 1\n");

	terminate_p();
}
void bad2_body(natq a)
{
	natl src;
	natq len = MAX_PAYLOAD;
	receive(src, (char *)0x200000, len);
	printf("processo errato 2\n");

	terminate_p();
}
void bad3_body(natq a)
{
	natl src;
	char buf[MAX_PAYLOAD + 1];
	receive(src, buf, *(natq *)0x200000);
	printf("processo errato 3\n");

	terminate_p();
}
void bad4_body(natq a)
{
	natl src;
	char buf[MAX_PAYLOAD + 1];
	receive(src, buf, *(natq *)(0UL-4));
	printf("processo errato 4\n");

	terminate_p();
}
void bad5_body(natq a)
{
	natl src;
	natq len = 8;
	receive(src, (char *)(0UL-4), len);
	printf("processo errato 5\n");

	terminate_p();
}
void hello_body(natq a)
{
	char buf[MAX_PAYLOAD + 1];
	natl src;
	natq len;
	int errors = 0;
	for (unsigned i = 0; i < sizeof(toreceive)/sizeof(msg); i++) {
		len = MAX_PAYLOAD;
		if (!receive(src, buf, len)) {
			printf("message %d: receive error\n", i);
			errors++;
			continue;
		}
		if (src != toreceive[i].s) {
			printf("message %d: received %8x, expected %8x\n", i, src, toreceive[i].s);
			errors++;
			continue;
		}
		buf[len] = '\0';
		unsigned int j;
		for (j = 0; buf[j] && toreceive[i].p[j] && buf[j] == toreceive[i].p[j]; j++)
			;
		if (buf[j] || toreceive[i].p[j]) {
			printf("message %d: payload error at byte %d\n", i, j);
			printf("   received '%d', expected '%d'\n", buf[j], toreceive[i].p[j]);
			errors++;
			continue;
		}
	}
	if (errors == 0)
		printf("receiver: no errors\n");
	pause();

	terminate_p();
}
natl hello;
natl bad1;
natl bad2;
natl bad3;
natl bad4;
natl bad5;

extern "C" void main()
{
	hello = activate_p(hello_body, 0, 20, LIV_UTENTE);
	bad1 = activate_p(bad1_body, 0, 50, LIV_UTENTE);
	bad2 = activate_p(bad2_body, 0, 49, LIV_UTENTE);
	bad3 = activate_p(bad3_body, 0, 48, LIV_UTENTE);
	bad4 = activate_p(bad4_body, 0, 47, LIV_UTENTE);
	bad5 = activate_p(bad5_body, 0, 46, LIV_UTENTE);

	terminate_p();
}

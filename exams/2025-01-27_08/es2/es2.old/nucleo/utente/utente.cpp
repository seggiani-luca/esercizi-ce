#include <all.h>

natb data0[] = { 0x0, 0x3, 0x5, 0x2, 0x2, 0xa, 0x0 };
natb data1[] = { 0xf, 0x2, 0x2, 0x3, 0x1 };
natb data2[] = { 0x7 };
natb data3[] = { 0xc, 0x2, 0xe, 0x1, 0xf, 0x4, 0x4, 0xb, 0x3 };

struct test {
	natb* data;
	natl len;
} tests[] = {
	{ data0, sizeof data0 }, 
	{ data1, sizeof data1 }, 
	{ data2, sizeof data2 }, 
	{ data3, sizeof data3 }, 
};

static const natl num_tests = sizeof tests / sizeof tests[0];

natl sem[16];

void p1(natq testid)
{
	if (!vcenew()) {
		fpanic("vcenew fallita");
	}
	test *t = &tests[testid];
	for (natl i = 0; i < t->len; i++)
		vcewrite(testid*16 + t->data[i]);
	sem_signal(sem[testid]);
	terminate_p();
}

void main()
{
	natq before = getiomeminfo();
	for (natl i = 0; i < num_tests; i++) {
		sem[i] = sem_ini(0);
		activate_p(p1, i, 50 + i, LIV_UTENTE);
	}
	for (natl i = 0; i < num_tests; i++)
		sem_wait(sem[i]);
	printf("attendo 5 secondi\n");
	delay(100);
	natq after = getiomeminfo();
	int errors = 0;
	if (after != before) {
		printf("Errore: heap I/O prima %lu, dopo %lu", before, after);
		errors++;
	}
	for (natl i = 0; i < num_tests; i++) {
		test *t = tests + i;
		if (!vcedbg(i, t->len, t->data)) {
			printf("Errore nel test %d (vedere i log)\n", i);
			errors++;
		}
	}
	if (!errors)
		printf("OK\n");
	pause();
	terminate_p();
}

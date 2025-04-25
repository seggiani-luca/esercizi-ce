#include <all.h>
#include <vm.h>

#define dbg(s, ...) flog(LOG_DEBUG, "TEST %lu: " s, test_num, ## __VA_ARGS__)
#define msg(s, ...) printf("TEST %lu PROC %u: " s "\n", test_num, getpid(), ## __VA_ARGS__)
#define err(s, ...) msg("ERRORE: " s, ## __VA_ARGS__)
#define die(s, ...) do { err(s, ## __VA_ARGS__); goto error; } while (0)

#define new_proc(tn, pn)\
	t##tn##p##pn = activate_p(t##tn##p##pn##b, test_num, prio--, LIV_UTENTE);

natl end_test; // sync

#define end_subtest() do {\
	(void)&&error;\
error:\
	terminate_p();\
} while (0)

#define end_test() do {\
	(void)&&error;\
error:\
	sem_signal(end_test);\
	terminate_p();\
} while (0)

#define TCNT(n)	natl t##n##m0; natl t##n##n0;
#define testok(n) do {\
	sem_wait(t##n##m0);\
	t##n##n0++;\
	sem_signal(t##n##m0);\
} while (0)

#define sloterr(s_)  ((s_) == 0xFFFFFFFF ? "0xFFFFFFFF" : "uno slot")

#define ckopen(p_, w_, es_) ({\
	natl s_ = openpipe(p_, w_);\
	if (((es_) == 0xFFFFFFFF && s_ != 0xFFFFFFFF) || ((es_) != 0xFFFFFFFF && s_ == 0xFFFFFFFF) ) {\
		err("openpipe: ottenuto %s invece di %s", sloterr(s_), sloterr(es_));\
		goto error;\
	}\
	s_;\
})

#define bool2str(r_) ((r_) ? "true" : "false")

#define cktrns(d_, s_, b_, n_, er_) do {\
	bool r_ = d_##pipe(s_, b_, n_);\
	if ((er_) != r_) {\
		err(#d_ ": ottenuto %s (%x) invece di %s", bool2str(r_), r_, bool2str(er_));\
		goto error;\
	}\
} while (0)

natq test_num;

///**********************************************************************
// *             test 00: errori vari                                   *
// **********************************************************************/

natl t00p0;
natl t00p1;
natl t00p2;
natl t00p3;
char t00v0[2];

void t00p0b(natq test_num)
{
	openpipe(MAX_PIPES, true);
	err("openpipe con pipeid non valido non ha causato abort\n");
	terminate_p();
}

void t00p1b(natq test_num)
{
	writepipe(0, "x", 1);
	err("writepipe su pipe non aperta non ha causato abort\n");
	terminate_p();
}

void t00p2b(natq test_num)
{
	readpipe(0, t00v0, 2);
	err("writepipe su pipe non aperta non ha causato abort\n");
	terminate_p();
}

void t00p3b(natq test_num)
{
	closepipe(0);
	err("closepipe su pipe non aperta non ha causato abort\n");
	terminate_p();
}

///**********************************************************************
// *             test 01: open/close senza scambio di dati              *
// **********************************************************************/

natl t01p0;
natl t01p1;
natl t01p2;
TCNT(01);

void t01p0b(natq test_num)
{
	natl s = ckopen(0, true, 0);
	closepipe(s);
	testok(01);
	end_test();
}

void t01p1b(natq test_num)
{
	ckopen(0, true, 0xFFFFFFFF);
	testok(01);
	end_test();
}

void t01p2b(natq test_num)
{
	natl s = ckopen(0, false, 0);
	closepipe(s);
	testok(01);
	end_test();
}

///**********************************************************************
// *             test 02: più pipe                                      *
// **********************************************************************/

natl t02p0;
natl t02p1;
TCNT(02);

void t02p0b(natq test_num)
{
	natl s1, s2;

	s1 = ckopen(0, true, 0);
	s2 = ckopen(1, false, 0);
	closepipe(s1);
	closepipe(s2);
	testok(02);
	end_test();
}

void t02p1b(natq test_num)
{
	natl s1, s2;

	s1 = ckopen(0, false, 0);
	s2 = ckopen(1, true, 0);
	closepipe(s1);
	closepipe(s2);
	testok(02);
	end_test();
}

///**********************************************************************
// *             test 03: troppe pipe                                   *
// **********************************************************************/

natl t03p0;
natl t03p1;
TCNT(03);

void t03p0b(natq test_num)
{
	ckopen(0, true, 0);
	ckopen(1, false, 0);
	ckopen(2, true, 0);
	err("apertura di 3 pipe non ha causato abort");
	end_test();
}

void t03p1b(natq test_num)
{
	natl s1, s2;

	s1 = ckopen(0, false, 0);
	s2 = ckopen(1, true, 0);
	closepipe(s1);
	closepipe(s2);
	testok(03);
	end_test();
}

///**********************************************************************
// *             test 04: chisura per terminazione                      *
// **********************************************************************/

natl t04p0;
natl t04p1;
natl t04p2;
natl t04p3;
TCNT(04);

void t04p0b(natq test_num)
{
	natl s1, s2;

	s1 = ckopen(0, true, 0);
	s2 = ckopen(1, false, 0);
	closepipe(s1);
	closepipe(s2);
	testok(04);
	end_test();
}

void t04p1b(natq test_num)
{
	ckopen(0, false, 0);
	ckopen(1, true, 0);
	testok(04);
	end_test();
}

void t04p2b(natq test_num)
{
	ckopen(0, true, 0);
	ckopen(1, false, 0);
	testok(04);
	end_test();
}

void t04p3b(natq test_num)
{
	natl s1, s2;

	s1 = ckopen(0, false, 0);
	s2 = ckopen(1, true, 0);
	testok(04);
	closepipe(s1);
	closepipe(s2);
	end_test();
}


///**********************************************************************
// *             test 05: errori su read/write                          *
// **********************************************************************/

natl t05p0;
natl t05p1;
natl t05p2;
natl t05p3;
natl t05p4;
natl t05p5;
natl t05p6;
natl t05p7;
char t05v0[2];
TCNT(05);

void t05p0b(natq test_num)
{
	natl s = ckopen(0, true, 0);
	readpipe(s, t05v0, 2);
	err("lettura da pipe aperta in scrittura non ha causato abort");
	end_test();
}

void t05p1b(natq test_num)
{
	natl s = ckopen(0, false, 0);
	closepipe(s);
	testok(05);
	end_test();
}

void t05p2b(natq test_num)
{
	natl s = ckopen(2, false, 0);
	writepipe(s, t05v0, 2);
	err("scrittura su pipe aperta in lettura non ha causato abort");
	end_test();
}

void t05p3b(natq test_num)
{
	natl s = ckopen(2, true, 0);
	closepipe(s);
	testok(05);
	end_test();
}

void t05p4b(natq test_num)
{
	char local[2];
	natl s = ckopen(3, false, 0);
	readpipe(s, local, 2);
	err("lettura in buffer locale non ha causato abort");
	end_test();
}

void t05p5b(natq test_num)
{
	natl s = ckopen(3, true, 0);
	closepipe(s);
	testok(05);
	end_test();
}

void t05p6b(natq test_num)
{
	char local[2];
	natl s = ckopen(5, true, 0);
	writepipe(s, local, 2);
	err("scrittura da buffer locale non ha causato abort");
	end_test();
}

void t05p7b(natq test_num)
{
	natl s = ckopen(5, false, 0);
	closepipe(s);
	testok(05);
	end_test();
}

///**********************************************************************
// *             test 06: chiusura prima di un trasferimento            *
// **********************************************************************/

natl t06p0;
natl t06p1;
natl t06p2;
natl t06p3;
char t06v0[5];
TCNT(06);

void t06p0b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, true, 0);
	closepipe(s1);
	testok(06);
	end_test();
}

void t06p1b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, false, 0);
	cktrns(read, s1, t06v0, 5, false);
	closepipe(s1);
	testok(06);
	end_test();
}

void t06p2b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, false, 0);
	closepipe(s1);
	testok(06);
	end_test();
}

void t06p3b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, true, 0);
	cktrns(write, s1, t06v0, 5, false);
	closepipe(s1);
	testok(06);
	end_test();
}

///**********************************************************************
// *             test 07: chiusura durante un trasferimento             *
// **********************************************************************/

natl t07p0;
natl t07p1;
natl t07p2;
natl t07p3;
char t07v0[5] = { 1, 2, 3, 4, 5 };
char t07v1[5];
char t07v2[5] = { 1, 2, 3, 4, 5 };
char t07v3[5];
TCNT(07);

void t07p0b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, true, 0);
	cktrns(write, s1, t07v0, 5, true);
	cktrns(write, s1, t07v0, 5, false);
	closepipe(s1);
	testok(07);
	end_test();
}

void t07p1b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, false, 0);
	cktrns(read, s1, t07v1, 5, true);
	closepipe(s1);
	testok(07);
	end_test();
}

void t07p2b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, false, 0);
	cktrns(read, s1, t07v3, 5, true);
	cktrns(read, s1, t07v3, 5, false);
	closepipe(s1);
	testok(07);
	end_test();
}

void t07p3b(natq test_num)
{
	natl s1;

	s1 = ckopen(0, true, 0);
	cktrns(write, s1, t07v2, 5, true);
	testok(07);
	end_test();
}

/**********************************************************************/

extern natl mainp;
void main_body(natq id)
{
	natl prio = 600;

	end_test = sem_ini(0);

	test_num = 0;
	dbg(">>>INIZIO<<<: errori vari");
	new_proc(00, 0);
	new_proc(00, 1);
	new_proc(00, 2);
	new_proc(00, 3);
	delay(1);
	dbg("=== FINE ===");
	
	pause();

	test_num = 1;
	dbg(">>>INIZIO<<<: open/close senza scambio di dati");
	new_proc(01, 0);
	new_proc(01, 1);
	new_proc(01, 2);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t01n0 == 3) msg("OK");
	dbg("=== FINE ===");

	pause();
	
	test_num = 2;
	dbg(">>>INIZIO<<<: più pipe");
	new_proc(02, 0);
	new_proc(02, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t02n0 == 2) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 3;
	dbg(">>>INIZIO<<<: troppe pipe");
	new_proc(03, 0);
	new_proc(03, 1);
	sem_wait(end_test);
	if (t03n0 == 1) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 4;
	dbg(">>>INIZIO<<<: chiusura per terminazione");
	new_proc(04, 0);
	new_proc(04, 1);
	new_proc(04, 2);
	new_proc(04, 3);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t04n0 == 4) msg("OK");
	dbg("=== FINE ===");

	pause();
	
	test_num = 4;
	dbg(">>>INIZIO<<<: chiusura per terminazione");
	new_proc(04, 0);
	new_proc(04, 1);
	new_proc(04, 2);
	new_proc(04, 3);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t04n0 == 4) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 5;
	dbg(">>>INIZIO<<<: errori su read/write");
	new_proc(05, 0);
	new_proc(05, 1);
	new_proc(05, 2);
	new_proc(05, 3);
	new_proc(05, 4);
	new_proc(05, 5);
	new_proc(05, 6);
	new_proc(05, 7);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t05n0 == 4) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 6;
	dbg(">>>INIZIO<<<: chiusura prima di un trasferimento");
	new_proc(06, 0);
	new_proc(06, 1);
	new_proc(06, 2);
	new_proc(06, 3);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t06n0 == 4) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 7;
	dbg(">>>INIZIO<<<: chiusura durante un trasferimento");
	new_proc(07, 0);
	new_proc(07, 1);
	new_proc(07, 2);
	new_proc(07, 3);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t07n0 == 4) msg("OK");
	dbg("=== FINE ===");

	pause();

	terminate_p();
}
natl mainp;

extern "C" void main()
{
	mainp = activate_p(main_body, 0, 900, LIV_UTENTE);

	terminate_p();
}

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
	t##n##n0++;\
} while (0)

#define PSEQ(s_) natq t##s_##S0[20]; natl t##s_##S1
#define SEQM	(1UL<<32)

/***********************************************************************
 * chkseq(numero_del_test, nuovo turno, processo_precedente)           *
 *                                                                     *
 * Controlla l'ordine di esecuzione dei processi all'interno del test  *
 ***********************************************************************/
#define chkseq(s_, n_, v_) do {\
	natl& s1_ = t##s_##S1;\
	natq* s0_ = t##s_##S0;\
	natl  p_  = t##s_##p##v_;\
	if (s1_ != (n_))\
		die("dovremmo essere al turno %u, invece siamo a %u",\
				n_, s1_);\
	if (s1_ && (s0_[s1_-1]) != (p_ | SEQM))\
		die("al turno %u il processo e' stato %u invece di %u",\
				s1_-1, (natl)(s0_[s1_-1] & ~SEQM), p_);\
	s0_[s1_++] = getpid() | SEQM;\
} while (0)

natq test_num;
///**********************************************************************
// *             test 00: errori vari                                   *
// **********************************************************************/

natl t00p0;
natl t00p1;
natl t00p2;
natl t00p3;
natl t00p4;
natl t00t1;

void t00p0b(natq test_num)
{
	rw_upgrade(100);	
	err("rw_upgrade() con rw non valido non ha causato abort");
	terminate_p();
}

void t00p1b(natq test_num)
{
	t00t1 = rw_init();
	rw_upgrade(t00t1);
	err("rw_upgrade() senza essere reader non ha causato abort");
	terminate_p();
}

void t00p2b(natq test_num)
{
	rw_downgrade(t00t1);
	err("rw_downgrade() su rwlock non attivo non ha causato abort");
	terminate_p();
}

void t00p3b(natq test_num)
{
	rw_writelock(t00t1);
	rw_upgrade(t00t1);
	err("rw_upgrade() da writer non ha causato abort");
	terminate_p();
}

void t00p4b(natq test_num)
{
	rw_downgrade(t00t1+1);	
	err("rw_downgrade() con rw non valido non ha causato abort");
	terminate_p();
}


///**********************************************************************
// *             test 01: caso base                                     *
// **********************************************************************/

natl t01p0;
natl t01t0;
TCNT(01);

void t01p0b(natq test_num)
{
	t01t0 = rw_init();
	for (int i = 0; i < 10; i++) {
		rw_readlock(t01t0);
		rw_upgrade(t01t0);
		rw_downgrade(t01t0);
		rw_downgrade(t01t0);
	}
	testok(01);
	end_test();
}

///**********************************************************************
// *             test 02: più lettori                                   *
// **********************************************************************/

natl t02p0;
natl t02p1;
natl t02p2;
natl t02t0;
natl t02s0;
PSEQ(02);
TCNT(02);

void t02p0b(natq test_num)
{
	t02t0 = rw_init();
	t02s0 = sem_ini(0);

	rw_readlock(t02t0);
	sem_wait(t02s0);
	chkseq(02, 1, 1);
	rw_downgrade(t02t0);
	testok(02);
	end_test();
}

void t02p1b(natq test_num)
{
	rw_readlock(t02t0);
	chkseq(02, 0, 0);
	rw_upgrade(t02t0);
	chkseq(02, 2, 0);
	rw_downgrade(t02t0);
	rw_downgrade(t02t0);
	testok(02);
	end_test();
}

void t02p2b(natq test_num)
{
	sem_signal(t02s0);
	testok(02);
	end_test();
}

///**********************************************************************
// *             test 03: piu' scrittori (precedenza superiore)         *
// **********************************************************************/

natl t03p0;
natl t03p1;
natl t03s0;
natl t03t0;
PSEQ(03);
TCNT(03);

void t03p0b(natq test_num)
{
	t03t0 = rw_init();
	t03s0 = sem_ini(0);

	sem_wait(t03s0);
	chkseq(03, 1, 1);
	rw_writelock(t03t0);
	chkseq(03, 3, 1);
	rw_downgrade(t03t0);
	testok(03);
	end_test();
}

void t03p1b(natq test_num)
{
	rw_readlock(t03t0);
	chkseq(03, 0, 0);
	sem_signal(t03s0);
	chkseq(03, 2, 0);
	rw_upgrade(t03t0);
	chkseq(03, 4, 0);
	rw_downgrade(t03t0);
	testok(03);
	end_test();
}

///**********************************************************************
// *             test 04: lettori e scrittori                           *
// **********************************************************************/

natl t04p0;
natl t04p1;
natl t04p2;
natl t04p3;
natl t04s0;
natl t04s1;
natl t04t0;
PSEQ(04);
TCNT(04);

void t04p0b(natq test_num)
{
	t04t0 = rw_init();
	t04s0 = sem_ini(0);
	t04s1 = sem_ini(0);

	rw_readlock(t04t0);
	chkseq(04, 0, 0);
	sem_wait(t04s0);
	chkseq(04, 5, 1);
	rw_downgrade(t04t0);
	testok(04);
	end_test();
}

void t04p1b(natq test_num)
{
	rw_readlock(t04t0);
	chkseq(04, 1, 0);
	sem_wait(t04s1);
	chkseq(04, 4, 3);
	sem_signal(t04s0);
	chkseq(04, 6, 0);
	rw_upgrade(t04t0);
	chkseq(04, 7, 1);
	rw_downgrade(t04t0);
	chkseq(04, 8, 1);
	rw_downgrade(t04t0);
	chkseq(04, 9, 1);
	testok(04);
	end_test();
}

void t04p2b(natq test_num)
{
	chkseq(04, 2, 1);
	rw_writelock(t04t0);
	chkseq(04, 10, 1);
	rw_downgrade(t04t0);
	chkseq(04, 11, 2);
	testok(04);
	end_test();
}

void t04p3b(natq test_num)
{
	chkseq(04, 3, 2);
	sem_signal(t04s1);
	chkseq(04, 12, 2);
	rw_writelock(t04t0);
	rw_downgrade(t04t0);
	chkseq(04, 13, 3);
	testok(04);
	end_test();
}

///**********************************************************************
// *             test 05: lettori arrivati durante l'upgrade            *
// **********************************************************************/

natl t05p0;
natl t05p1;
natl t05s0;
natl t05t0;
PSEQ(05);
TCNT(05);

void t05p0b(natq test_num)
{
	t05t0 = rw_init();
	t05s0 = sem_ini(0);

	sem_wait(t05s0);
	chkseq(05, 1, 1);
	rw_readlock(t05t0);
	chkseq(05, 3, 1);
	rw_downgrade(t05t0);
	chkseq(05, 4, 0);
	testok(05);
	end_test();
}

void t05p1b(natq test_num)
{
	rw_readlock(t05t0);
	rw_upgrade(t05t0);
	chkseq(05, 0, 0);
	sem_signal(t05s0);
	chkseq(05, 2, 0);
	rw_downgrade(t05t0);
	chkseq(05, 5, 0);
	testok(05);
	end_test();
}

///**********************************************************************
// *             test 06: piu' upgrade                                  *
// **********************************************************************/

natl t06p0;
natl t06p1;
natl t06p2;
natl t06s0;
natl t06t0;
PSEQ(06);
TCNT(06);

void t06p0b(natq test_num)
{
	t06t0 = rw_init();
	t06s0 = sem_ini(0);

	rw_readlock(t06t0);
	chkseq(06, 0, 0);
	sem_wait(t06s0);
	chkseq(06, 4, 2);
	rw_upgrade(t06t0);
	chkseq(06, 6, 2);
	rw_downgrade(t06t0);
	chkseq(06, 7, 0);
	rw_downgrade(t06t0);
	chkseq(06, 8, 0);
	testok(06);
	end_test();
}

void t06p1b(natq test_num)
{
	chkseq(06, 1, 0);
	rw_readlock(t06t0);
	chkseq(06, 2, 1);
	rw_upgrade(t06t0);
	chkseq(06, 9, 0);
	rw_downgrade(t06t0);
	chkseq(06, 10, 1);
	rw_downgrade(t06t0);
	chkseq(06, 11, 1);
	testok(06);
	end_test();
}

void t06p2b(natq test_num)
{
	rw_readlock(t06t0);
	chkseq(06, 3, 1);
	sem_signal(t06s0);
	chkseq(06, 5, 0);
	rw_downgrade(t06t0);
	chkseq(06, 12, 1);
	testok(06);
	end_test();
}

// **********************************************************************/
// *             test 07: piu' rwlock                                   *
// **********************************************************************/

natl t07p0;
natl t07p1;
natl t07p2;
natl t07t0;
natl t07t1;
natl t07s0;
PSEQ(07);
TCNT(07);

void t07p0b(natq test_num)
{
	t07t0 = rw_init();
	t07t1 = rw_init();
	t07s0 = sem_ini(0);

	rw_readlock(t07t0);
	rw_writelock(t07t1);
	chkseq(07, 0, 0);
	sem_wait(t07s0);
	chkseq(07, 3, 2);
	rw_upgrade(t07t0);
	chkseq(07, 4, 0);
	rw_downgrade(t07t1);
	sem_wait(t07s0);
	chkseq(07, 6, 1);
	rw_downgrade(t07t0);
	chkseq(07, 7, 0);
	rw_downgrade(t07t0);
	chkseq(07, 8, 0);
	testok(07);
	end_test();
}

void t07p1b(natq test_num)
{
	chkseq(07, 1, 0);
	rw_writelock(t07t1);
	chkseq(07, 5, 0);
	sem_signal(t07s0);
	chkseq(07, 9, 0);
	rw_downgrade(t07t1);
	testok(07);
	end_test();
}

void t07p2b(natq test_num)
{
	chkseq(07, 2, 1);
	sem_signal(t07s0);
	testok(07);
	end_test();
}

extern "C" void main()
{
	natl prio = 600;

	end_test = sem_ini(0);

	test_num = 0;
	dbg(">>>INIZIO<<<: errori vari");
	new_proc(00, 0);
	new_proc(00, 1);
	new_proc(00, 2);
	new_proc(00, 3);
	new_proc(00, 4);
	delay(10);
	dbg("=== FINE ===");

	test_num = 1;
	dbg(">>>INIZIO<<<: caso base");
	new_proc(01, 0);
	sem_wait(end_test);
	if (t01n0 == 1) msg("OK");
	dbg("=== FINE ===");

	test_num = 2;
	dbg(">>>INIZIO<<<: piu' lettori");
	new_proc(02, 0);
	new_proc(02, 1);
	new_proc(02, 2);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t02n0 == 3) msg("OK");
	dbg("=== FINE ===");

	test_num = 3;
	dbg(">>>INIZIO<<<: piu' scrittori (precedenza superior)");
	new_proc(03, 0);
	new_proc(03, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t03n0 == 2) msg("OK");
	dbg("=== FINE ===");

	test_num = 4;
	dbg(">>>INIZIO<<<: lettori e scrittori");
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

	test_num = 5;
	dbg(">>>INIZIO<<<: lettori arrivati durante l'upgrade");
	new_proc(05, 0);
	new_proc(05, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t05n0 == 2) msg("OK");
	dbg("=== FINE ===");

	test_num = 6;
	dbg(">>>INIZIO<<<: piu' upgrade");
	new_proc(06, 0);
	new_proc(06, 1);
	new_proc(06, 2);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t06n0 == 3) msg("OK");
	dbg("=== FINE ===");

	test_num = 7;
	dbg(">>>INIZIO<<<: piu' rwlock");
	new_proc(07, 0);
	new_proc(07, 1);
	new_proc(07, 2);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t07n0 == 3) msg("OK");
	dbg("=== FINE ===");

	pause();
	terminate_p();
}

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

#define ckrecv(er_, b_, s_) do {\
	natq r_ = mq_recv(b_, s_);\
	if ((er_) != r_) {\
		err("ottenuto %lu invece di %lu", r_, er_);\
		goto error;\
	}\
} while (0)

#define cksend(er_, b_, s_) do {\
	bool r_ = mq_send(b_, s_);\
	if ((er_) != r_) {\
		err("ottenuto %x invece di %x", r_, er_);\
		goto error;\
	}\
} while (0)

#define ckbuf(b_, eb_, s_) do {\
	for (natq i_ =  0; i_ < (s_); i_++) {\
		if ((b_)[i_] != (eb_)[i_]) {\
			err("byte %lu: ottenuto %02x (%c) invece di %02x (%c)",\
					i_, (b_)[i_], (b_)[i_], (eb_)[i_], (eb_)[i_]);\
			goto error;\
		}\
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
natl t00p4;

void t00p0b(natq test_num)
{
	char dummy[1];
	mq_recv(dummy, 1);
	err("mq_recv() da processo non registrato non ha causato abort");
	terminate_p();
}

void t00p1b(natq test_num)
{
	char dummy[1];
	mq_reg();
	mq_send(dummy, 1);
	err("mq_send() da processo registrato non ha causato abort");
	terminate_p();
}

void t00p2b(natq test_num)
{
	mq_send(nullptr, 1);
	err("mq_send() con indirizzo non valido non ha causato abort");
	terminate_p();
}

void t00p3b(natq test_num)
{
	mq_reg();
	mq_recv(nullptr, 1);
	err("mq_recv() con indirizzo non valido non ha causato abort");
	terminate_p();
}

void t00p4b(natq test_num)
{
	mq_reg();
	mq_recv(reinterpret_cast<char*>(t00p4b), 1);
	err("mq_recv() con indirizzo non valido non ha causato abort");
	terminate_p();
}

///**********************************************************************
// *             test 01: messaggio contiguo, un lettore bloccato       *
// **********************************************************************/

natl t01p0;
natl t01p1;
TCNT(01);

void t01p0b(natq test_num)
{
	char buf[10];
	mq_reg();
	ckrecv(5UL, buf, 10UL);
	ckbuf(buf, "12345", 5UL);
	testok(01);
	end_test();
}

void t01p1b(natq test_num)
{
	char buf[] = "12345";
	cksend(true, buf, 5UL);
	testok(01);
	end_test();
}

///**********************************************************************
// *             test 02: messaggio contiguo, uno scrittore bloccato    *
// **********************************************************************/

natl t02p0;
natl t02p1;
natl t02s1;
TCNT(02);

void t02p1b(natq test_num)
{
	char buf[] = "abcdef";
	sem_wait(t02s1);
	cksend(true, buf, 7UL);
	testok(02);
	end_test();
}


void t02p0b(natq test_num)
{
	char buf[10];
	mq_reg();
	sem_signal(t02s1);
	ckrecv(7UL, buf, 10UL);
	ckbuf(buf, "abcdef", 7UL);
	testok(02);
	end_test();
}

///**********************************************************************
// *             test 03: messaggio contiguo, due lettori bloccati      *
// **********************************************************************/

natl t03p0;
natl t03p1;
natl t03p2;
TCNT(03);

void t03p0b(natq test_num)
{
	char buf[10];
	mq_reg();
	ckrecv(4UL, buf, 10UL);
	ckbuf(buf, "ABCD", 4UL);
	testok(03);
	end_test();
}

void t03p1b(natq test_num)
{
	char buf[15];
	mq_reg();
	ckrecv(4UL, buf, 15UL);
	ckbuf(buf, "ABCD", 4UL);
	testok(03);
	end_test();
}

void t03p2b(natq test_num)
{
	char buf[] = "ABCD";
	cksend(true, buf, 4UL);
	testok(03);
	end_test();
}

///**********************************************************************
// *             test 04: messaggio non contiguo, un lettore bloccato   *
// **********************************************************************/

natl t04p0;
natl t04p1;
TCNT(04);

void t04p0b(natq test_num)
{
	char buf[2*DIM_PAGINA];
	vaddr vbuf = int_cast<vaddr>(&buf);
	vaddr b = limit(vbuf, 0) - 3;
	if (b < vbuf)
		b += DIM_PAGINA;
	char *dst = ptr_cast<char>(b);
	mq_reg();
	ckrecv(9UL, dst, 10UL);
	ckbuf(dst, "qwertyuop", 9UL);
	testok(04);
	end_test();
}

void t04p1b(natq test_num)
{
	char buf[] = "qwertyuop";
	cksend(true, buf, 9UL);
	testok(04);
	end_test();
}

///**********************************************************************
// *             test 05: messaggio non contiguo, uno scrittore bloccato*
// **********************************************************************/

natl t05p0;
natl t05p1;
natl t05s1;
TCNT(05);

void t05p1b(natq test_num)
{
	char buf[2*DIM_PAGINA];
	vaddr vbuf = int_cast<vaddr>(&buf);
	vaddr b = limit(vbuf, 0) - 3;
	if (b < vbuf)
		b += DIM_PAGINA;
	char *src = ptr_cast<char>(b);
	memcpy(src, "zxcvbnm", 7);
	sem_wait(t05s1);
	cksend(true, src, 7UL);
	testok(05);
	end_test();
}


void t05p0b(natq test_num)
{
	char buf[10];
	mq_reg();
	sem_signal(t05s1);
	ckrecv(7UL, buf, 10UL);
	ckbuf(buf, "zxcvbnm", 7UL);
	testok(05);
	end_test();
}

///**********************************************************************
// *             test 06: più lettori e scrittori                       *
// **********************************************************************/

natl t06p0;
natl t06p1;
natl t06p2;
natl t06p3;
natl t06p4;
natl t06s1;
natl t06s2;
natl t06s3;
natl t06s4;
TCNT(06);

void t06p0b(natq test_num)
{
	char buf[] = "!01!";
	sem_wait(t06s1);
	cksend(true, buf, 4UL); // send
	testok(06);
	end_test();
}

void t06p1b(natq test_num)
{
	char buf[] = "^20^asdfgh";
	sem_wait(t06s2);
	cksend(true, buf, 10UL); // send
	testok(06);
	end_test();
}

void t06p2b(natq test_num)
{
	char buf[] = "#03#";
	sem_wait(t06s3);
	cksend(false, buf, 4UL); // send
	testok(06);
	end_test();
}

void t06p3b(natq test_num)
{
	char buf[4];
	mq_reg();
	sem_wait(t06s4);
	ckrecv(4UL, buf, 4UL); // rec
	ckbuf(buf, "!01!", 4UL);
	ckrecv(10UL, buf, 4UL); // rec (il buffer è piccolo quindi non lo riceve)
	ckbuf(buf, "!01!", 4UL);
	testok(06);
	end_test();
}

void t06p4b(natq test_num)
{
	char buf[15];
	mq_reg();
	sem_signal(t06s4);
	sem_signal(t06s1);
	sem_signal(t06s2);
	sem_signal(t06s3);
	ckrecv(4UL, buf, 15UL); // rec
	ckbuf(buf, "!01!", 4UL);
	ckrecv(10UL, buf, 15UL); // rec
	ckbuf(buf, "^20^asdfgh", 10UL);
	testok(06);
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
	new_proc(00, 4);
	delay(1);
	dbg("=== FINE ===");

	pause();
	
	test_num = 1;
	dbg(">>>INIZIO<<<: messaggio contiguo, 1 lettore bloccato");
	new_proc(01, 0);
	new_proc(01, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t01n0 == 2) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 2;
	t02s1 = sem_ini(0);
	dbg(">>>INIZIO<<<: messaggio contiguo, uno scrittore bloccato");
	new_proc(02, 0);
	new_proc(02, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t02n0 == 2) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 3;
	dbg(">>>INIZIO<<<: messaggio contiguo, 2 lettori bloccati");
	new_proc(03, 0);
	new_proc(03, 1);
	new_proc(03, 2);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t03n0 == 3) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 4;
	dbg(">>>INIZIO<<<: messaggio non contiguo, 1 lettore bloccato");
	new_proc(04, 0);
	new_proc(04, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t04n0 == 2) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 5;
	dbg(">>>INIZIO<<<: messaggio non contiguo, uno scrittore bloccato");
	t05s1 = sem_ini(0);
	new_proc(05, 0);
	new_proc(05, 1);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t05n0 == 2) msg("OK");
	dbg("=== FINE ===");
	
	pause();

	test_num = 6;
	dbg(">>>INIZIO<<<: più lettori e più scrittori");
	t06s1 = sem_ini(0);
	t06s2 = sem_ini(0);
	t06s3 = sem_ini(0);
	t06s4 = sem_ini(0);
	new_proc(06, 0);
	new_proc(06, 1);
	new_proc(06, 2);
	new_proc(06, 3);
	new_proc(06, 4);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	sem_wait(end_test);
	if (t06n0 == 5) msg("OK");
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

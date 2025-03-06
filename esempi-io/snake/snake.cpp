#include <libce.h>

static const natl MAXSZ = 4000;

short snakex[MAXSZ];
char snakey[MAXSZ];
int snake_head;
int snake_tail;
char snake_shape;
natq snake_nexttick = 0;
natq snake_period = 25;
enum directions { UP, DOWN, LEFT, RIGHT } snake_dir;

volatile natq ticks = 0;
natw N = 11932; // 10ms
extern "C" void a_timer();
void start_timer()
{
	gate_init(0x50, a_timer);
	apic::set_VECT(2, 0x50);
	apic::set_TRGM(2, false); // false: fronte, true: livello
	timer::start0(N);
	apic::set_MIRQ(2, false);
}

extern "C" void c_timer()
{
	ticks++;
	apic::send_EOI();
}

natw border = 0x0200;
natw snake_col = 0x0600;
natw background = 0x0F00;
natw dead = 0x0400;
natw food = 0x0a00;

natq food_nexttick = 5;
natq food_period = 1000;
void dropfood()
{
	natl foodx, foody;
	do {
		foodx = random() % vid::cols();
		foody = random() % vid::rows();
	} while ((vid::pos(foodx, foody) & 0xFF) != ' ');
	natl r = random() % 10;
	char f;
	if (r < 1)
		f = 'O';
	else if (r < 4)
		f = 'o';
	else
		f = '.';
	vid::pos(foodx, foody) = food | f;
	food_nexttick += food_period;
}

enum notes {
	PA = 0,
	C3 = 9121,
	D3 = 8126,
	E3 = 7240,
	F3 = 6833,
	G3 = 6088,
	A3 = 5425,
	B3 = 4832,
	C4 = 4561,
	D4 = 4063,
	E4 = 3620,
	F4 = 3417,
	F4s= 3225,
	G4 = 3043,
	A4 = 2712,
	B4 = 2416,
	C5 = 2280,
	D5 = 2032,
};

struct single {
	notes note;
	natw  lenght;
};

natq song_nexttick;
int next_note = 0;
const ioaddr iSPR = 0x61;
natq song_beat = 40;
single song[] = {
//     |---------|---------|---------|---------|---------|---------|---------|---------|
	{ C4, 4 },                              { C5, 4 },
	{ B4, 2 },          { G4, 1 },{ A4, 1 },{ B4, 2 },          { C5, 2 },
	{ C4, 4 },                              { A4, 4 },
	{ G4, 6 },                                                  { PA, 2 },
	{ A3, 4 },                              { F4, 4 },
	{ E4, 2 },          { C4, 1 },{ D4, 1 },{ E4, 2 },          { F4, 2 },
	{ D4, 2 },          { B3, 1 },{ C4, 1 },{ D4, 2 },          { E4, 2 },
	{ C4, 4 },                              { PA, 4 },
	{ C4, 4 },                              { C5, 4 },
	{ B4, 2 },          { G4, 1 },{ A4, 1 },{ B4, 2 },          { C5, 2 },
	{ C4, 4 },                              { A4, 4 },
	{ G4, 6 },                                                  { PA, 2 },
	{ A3, 4 },                              { F4, 4 },
	{ E4, 2 },          { C4, 1 },{ D4, 1 },{ E4, 2 },          { F4, 2 },
	{ D4, 2 },          { B3, 1 },{ C4, 1 },{ D4, 2 },          { E4, 2 },
	{ C4, 6 },                                                  { PA, 1 },{ G4, 1 },
	{ E4, 1 },{ G4, 1 },{ E4, 1 },{ G4, 1 },{ E4, 1 },{ G4, 1 },{ E4, 1 },{ G4, 1 },
	{ F4, 1 },{ G4, 1 },{ F4, 1 },{ G4, 1 },{ F4, 1 },{ G4, 1 },{ F4, 1 },{ G4, 1 },
	{ A4, 4 },                              { A4, 10 },
	                                                            { PA, 1 },{ G4, 1 },
	{ E4, 1 },{ G4, 1 },{ E4, 1 },{ G4, 1 },{ E4, 1 },{ G4, 1 },{ E4, 1 },{ G4, 1 },
	{F4s, 1 },{ G4, 1 },{F4s, 1 },{ G4, 1 },{F4s, 1 },{ G4, 1 },{F4s, 1 },{ G4, 1 },
	{ B4, 4 },                              { B4, 4 },
	{ B4, 4 },                              { A4, 4 },
};
natq song_size = sizeof(song)/sizeof(single);

void init_song()
{
	next_note = 0;
	song_nexttick = ticks;
}
void stop_song()
{
	outputb(0, iSPR);
}
void update_song()
{
	single *s = &song[next_note];
	if (s->note == PA) {	
		timer::disable_spk();
	} else {
		timer::start2(s->note);
		timer::enable_spk();
	}
	next_note = (next_note + 1) % song_size;
	song_nexttick += s->lenght * song_beat;
}

void main()
{
	start_timer();
restart:
	vid::clear(background);
	init_song();

	snake_head = 0;
	snake_tail = 0;
	snakex[snake_head] = vid::cols()/2;
	snakey[snake_head] = vid::rows()/2;
	snake_dir = RIGHT;
	snake_shape = '>';
	snake_col = 0x0600;

	dropfood();

	for (natl i = 0; i < vid::cols(); i++) {
		vid::pos(i, 0) = border | '#';
		vid::pos(i, vid::rows() - 1) = border | '#';
	}
	for (natl i = 0; i < vid::rows(); i++) {
		vid::pos(0, i) = border | '#';
		vid::pos(vid::cols() - 1, i) = border | '#';
	}

	bool terminate = false;
	int eat = 0;
	for (;;) {

		// spostiamo la testa
		natl new_head = (snake_head + 1) % MAXSZ;
		switch (snake_dir) {
		case UP:
			snakex[new_head] = snakex[snake_head];
			snakey[new_head] = snakey[snake_head] - 1;
			snake_shape = '^';
			break;
		case DOWN:
			snakex[new_head] = snakex[snake_head];
			snakey[new_head] = snakey[snake_head] + 1;
			snake_shape = 'V';
			break;
		case LEFT:
			snakex[new_head] = snakex[snake_head] - 1;
			snakey[new_head] = snakey[snake_head];
			snake_shape = '<';
			break;
		case RIGHT:
			snakex[new_head] = snakex[snake_head] + 1;
			snakey[new_head] = snakey[snake_head];
			snake_shape = '>';
			break;
		}

		if (ticks >= snake_nexttick) {
			char next_pos  = vid::pos(snakex[new_head], snakey[new_head]) & 0xFF;
			switch (next_pos) {
			case '#':
			case '^':
			case '>':
			case '<':
			case 'V':
				snake_shape = 'X';
				snake_col = dead;
				terminate = true;
				break;
			case '.':
				eat += 1;
				break;
			case 'o':
				eat += 10;
				break;
			case 'O':
				eat += 100;
				break;
			}

			snake_head = new_head;
			// disegniamo la nuova testa
			vid::pos(snakex[snake_head], snakey[snake_head]) = snake_col | snake_shape;
			if (eat) {
				eat--;
			} else {
				// cancelliamo la coda
				vid::pos(snakex[snake_tail], snakey[snake_tail]) = background | ' ';
				// spostiamo la coda
				snake_tail = (snake_tail + 1) % MAXSZ;
			}
			snake_nexttick += snake_period;
		}

		if (terminate)
			break;

		char c = kbd::char_read_intr();
		switch (c) {
		case 'w':
			if (snake_dir != DOWN)
				snake_dir = UP;
			break;
		case 'a':
			if (snake_dir != RIGHT)
				snake_dir = LEFT;
			break;
		case 's':
			if (snake_dir != UP)
				snake_dir = DOWN;
			break;
		case 'd':
			if (snake_dir != LEFT)
				snake_dir = RIGHT;
			break;
		}

		if (ticks >= food_nexttick) {
			dropfood();
		}

		if (ticks >= song_nexttick) {
			update_song();
		}
	}

	stop_song();
	pause();

	goto restart;
}

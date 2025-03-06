#include <libce.h>

volatile bool fine = false;
char spinner[] = { '|', '/', '-', '\\' };

extern "C" void c_tastiera()
{
	natb c = kbd::get_code();
	if (c == 0x01)	// make code di ESC
		fine = true;
	for (int i = 0; i < 8; i++) {
		vid::char_write('0' + !!(c & 0x80));
		c <<= 1;
		for (volatile int j = 0; j < 10000000; j++)
			;
	}
	vid::char_write('\n');
	apic::send_EOI();
}

extern "C" void c_timer()
{
	static natq timer_spinpos = 0;
	vid::char_put(spinner[timer_spinpos % 4], 60, 12);
	timer_spinpos++;
	apic::send_EOI();
}

extern "C" void a_tastiera();
extern "C" void a_timer();

const natb KBD_VECT = 0x40;
const natb TIM_VECT = 0x50;
void main()
{
	vid::clear(0x0f);

	apic::set_VECT(1, KBD_VECT);
	gate_init(KBD_VECT, a_tastiera);
	apic::set_MIRQ(1, false);
	kbd::enable_intr();

	apic::set_VECT(2, TIM_VECT);
	gate_init(TIM_VECT, a_timer);
	timer::start0(59660); // periodo di 50ms
	apic::set_TRGM(2, false); // false: fronte, true: livello
	apic::set_MIRQ(2, false);

	natq spinpos = 0;
	while (!fine) {
		vid::char_put(spinner[spinpos % 4], 40, 12);
		spinpos++;
	}
}

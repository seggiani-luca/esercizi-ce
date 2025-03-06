#include <libce.h>

namespace timer {

	static const ioaddr iCWR = 0x43;
	static const ioaddr iCTR0_LOW = 0x40;
	static const ioaddr iCTR0_HIG = 0x40;

	void start(natw N)
	{
		outputb(0b00110100, iCWR);        // contatore 0 in modo 2
		outputb(N, iCTR0_LOW);
		outputb(N >> 8, iCTR0_HIG);
	}
}

volatile natq counter = 0;
extern "C" void c_timer()
{
	counter++;
	apic::send_EOI();
}

extern "C" void a_timer();
const natb TIM_VECT = 0x50;
void main()
{
	apic::set_VECT(2, TIM_VECT);
	gate_init(TIM_VECT, a_timer);
	timer::start0(59660); // periodo di 50ms (50/1000 * 1193192.66...)
	apic::set_TRGM(2, false); // false: fronte, true: livello
	apic::set_MIRQ(2, false);

	for (volatile int i = 0; i < 100000000; i++)
		;
	printf("counter = %lu\n", counter);
	pause();
}

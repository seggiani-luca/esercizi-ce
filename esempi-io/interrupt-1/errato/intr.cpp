#include <libce.h>

namespace kbd {

	const ioaddr iCMR = 0x64;
	const ioaddr iTBR = 0x60;

	void enable_intr()
	{
		outputb(0x60, iCMR);
		outputb(0x61, iTBR);
	}

	void disable_intr()
	{
		outputb(0x60, iCMR);
		outputb(0x60, iTBR);
	}

}

namespace vid {

	volatile natw* video = reinterpret_cast<volatile natw*>(0xb8000);
	natw attr;

}

bool fine = false;

void tastiera()
{
	natb c = kbd::get_code();
	if (c == 0x01)	// make code di ESC
		fine = true;
	for (int i = 0; i < 8; i++) {
		vid::char_write('0' + !!(c & 0x80));
		c <<= 1;
	}
	vid::char_write('\n');
}

const natb KBD_VECT = 0x40;
void main()
{
	char spinner[] = { '|', '/', '-', '\\' };

	vid::clear(0x0f);
	apic::set_VECT(1, KBD_VECT);
	gate_init(KBD_VECT, tastiera);
	apic::set_MIRQ(1, false);
	kbd::enable_intr();

	natq spinpos = 0;
	while (!fine) {
		vid::video[12*80+40] = vid::attr | spinner[spinpos % 4];
		spinpos++;
	}
}

#include <libce.h>

namespace kbd {

	const ioaddr iSTR = 0x64;
	const ioaddr iRBR = 0x60;

	natb get_code()
	{
		natb c;
		do
			c = inputb(iSTR);
		while (!(c & 0x01));
		return inputb(iRBR);
	}

}

void main()
{
	natb c;
	for (;;) {
		c = kbd::get_code();
		if (c == 0x01)
			break;	// make code di ESC
		for (int i = 0; i < 8; i++) {
			if (c & 0x80)	// bit piu' significativo di c
				vid::char_write('1');
			else
				vid::char_write('0');
			c <<= 1;
		}
		vid::char_write('\n');
	}
}

#include <libce.h>

namespace serial {

	const ioaddr iRBR = 0x03F8;
	const ioaddr iTHR = 0x03F8;
	const ioaddr iLSR = 0x03FD;
	const ioaddr iLCR = 0x03FB;
	const ioaddr iDLR_LSB = 0x03F8;
	const ioaddr iDLR_MSB = 0x03F9;
	const ioaddr iIER = 0x03F9;
	const ioaddr iMCR = 0x03FC;
	const ioaddr iIIR = 0x03FA;

	void init1()
	{
		natw CBITR = 0x000C;		// 9600 bit/sec.
		outputb(0x80, iLCR);		// DLAB 1
		outputb(CBITR, iDLR_LSB);
		outputb(CBITR >> 8, iDLR_MSB);
		outputb(0x03, iLCR);		// 1 bit STOP, 8 bit/car, parità dis, DLAB 0
		outputb(0x00, iIER);		// richieste di interruzione disabilitate
		inputb(iRBR);			// svuotamento buffer RBR
	}

	natb in1()
	{
		natb s;
		do
			s = inputb(iLSR);
		while (! (s & 0x01));
		return inputb(iRBR);
	}

};

void main()
{
	vid::clear(0x40);
	serial::init1();
	for (;;) {
		natb c = serial::in1();
		if (c == 0x1B)
			break;			// carattere ASCII esc
		vid::char_write(c);
	}
}

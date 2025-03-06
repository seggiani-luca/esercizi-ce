#include <libce.h>

namespace hd {

	const ioaddr iBR = 0x01F0;
	const ioaddr iCNL = 0x01F4;
	const ioaddr iCNH = 0x01F5;
	const ioaddr iSNR = 0x01F3;
	const ioaddr iHND = 0x01F6;
	const ioaddr iSCR = 0x01F2;
	const ioaddr iERR = 0x01F1;
	const ioaddr iCMD = 0x01F7;
	const ioaddr iSTS = 0x01F7;
	const ioaddr iDCR = 0x03F6;

	void wait_for_br()
	{
		natb s;
		do
			s = inputb(iSTS);
		while ((s & 0x88) != 0x08);
	}

	void input_sect(natb vetto[])
	{
		wait_for_br();
		inputbw(iBR, reinterpret_cast<natw*>(vetto), 256);
	}

}

void leggisett(natl lba, natb quanti, natb vetti[])
{
	hd::start_cmd(lba, quanti, hd::READ_SECT);

	for (int i = 0; i < quanti; i++) {
		hd::input_sect(&vetti[i * 512]);
	}
}

static const int MAX_BUF = 8;

natb buff[MAX_BUF * 512];
void main()
{
	natl lba = 1;
	natb quanti = 2;			// massimo MAX_BUF
	leggisett(lba, quanti, buff);
	for (int i = 0; i < quanti*512; i++)
		vid::char_write(buff[i]);
	vid::char_write('\n');
	pause();
	// ...
}

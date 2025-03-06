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

	void set_lba(natl lba)
	{
		natb lba_0 = lba,
		     lba_1 = lba >> 8,
		     lba_2 = lba >> 16,
		     lba_3 = lba >> 24;

		outputb(lba_0, iSNR);
		outputb(lba_1, iCNL);
		outputb(lba_2, iCNH);
		natb hnd = (lba_3 & 0x0F) | 0xE0;
		outputb(hnd, iHND);
	}

	void start_cmd(natl lba, natb quanti, natb cmd)
	{
		set_lba(lba);
		outputb(quanti, iSCR);
		outputb(cmd, iCMD);
	}

	void wait_for_br()
	{
		natb s;
		do
			s = inputb(iSTS);
		while ((s & 0x88) != 0x08);
	}

	void output_sect(natb vetto[])
	{
		wait_for_br();
		outputbw(reinterpret_cast<natw*>(vetto), 256, iBR);
	}

}

static const int MAX_BUF = 8;

natb buff[MAX_BUF * 512];
void main()
{
	natl lba = 1;
	natb quanti = 2;					// massimo MAX_BUF
	for (int i = 0; i < quanti * 512; i++)
		buff[i] = 'f';

	hd::start_cmd(lba, quanti, hd::WRITE_SECT);
	for (int i = 0; i < quanti; i++)
		hd::output_sect(&buff[i * 512]);

	vid::str_write("OK\n");
	pause();
	// ...
}

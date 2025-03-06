#include <libce.h>

namespace hd {

	const ioaddr iSTS = 0x01F7;
	const ioaddr iDCR = 0x03F6;

	void enable_intr()
	{
		outputb(0x00, iDCR);
	}

	void ack()
	{
		inputb(iSTS);
	}

}

namespace bm {

	ioaddr iBMCMD;
	ioaddr iBMSTR;
	ioaddr iBMDTPR;

	bool find(natb& bus, natb& dev, natb& fun)
	{
		natb code[] = { 0xff, 0x01, 0x01 };

		do {
			if (pci::find_class(bus, dev, fun, code) && (code[0] & (1U << 7)))
				return true;
		} while (pci::next(bus, dev, fun));
		return false;
	}

	void init(natb bus, natb dev, natb fun)
	{
		natl base = pci::read_confl(bus, dev, fun, 0x20);
		base &= ~0x1;
		iBMCMD  = static_cast<ioaddr>(base + 0);
		iBMSTR  = static_cast<ioaddr>(base + 2);
		iBMDTPR = static_cast<ioaddr>(base + 4);
		natw cmd = pci::read_confw(bus, dev, fun, 4);
		pci::write_confw(bus, dev, fun, 4, cmd | 0b101);
	}

	void prepare(natq prd, bool write)
	{
		outputl(prd, iBMDTPR);
		natb work = inputb(iBMCMD);
		if (write) {
			work &= ~0x8;
		} else {
			work |= 0x8;
		}
		outputb(work, iBMCMD);
		work = inputb(iBMSTR);
		work |= 0x6;
		outputb(work, iBMSTR);
	}

	void start()
	{
		natb work = inputb(iBMCMD);
		work |= 1;
		outputb(work, iBMCMD);
	}

	void ack()
	{
		natb work = inputb(iBMCMD);
		work &= 0xFE;
		outputb(work, iBMCMD);
		inputb(iBMSTR);
	}

}

volatile bool done = false;
extern char vv[];
const natl BUFSIZE = 65536;
extern natl prd[];

extern "C" void a_bmide();
extern "C" void c_bmide()
{
	done = true;
	bm::ack();
	hd::ack();
	apic::send_EOI();
}

const natw HD_VECT = 0x60;

void main()
{
	natb nn = BUFSIZE / 512;
	natb lba = 0;
	natb bus = 0, dev = 0, fun = 0;

	vid::clear(0x0f);

	if (!bm::find(bus, dev, fun))
		printf("bm non trovato!\n");
	printf("PCI-ATA at %02x:%02x.%1x\n", bus, dev, fun);
	bm::init(bus, dev, fun);

	apic::set_VECT(14, HD_VECT);
	gate_init(HD_VECT, a_bmide);
	apic::set_MIRQ(14, false);

	for (natl i = 0; i < BUFSIZE; i++)
		vv[i] = '-';

	printf("primi 80 caratteri di vv:\n");
	for (int i = 0; i < 80; i++)
		vid::char_write(vv[i]);
	printf("ultimi 80 caratteri di vv:\n");
	for (natl i = BUFSIZE - 80; i < BUFSIZE; i++)
		vid::char_write(vv[i]);

	//char *buf = (char *)((natq)&vv & 0xFFFFFFFFFFFF0000);
	//printf("80 byte all'indirizzo %p\n", buf);
	//for (int i = 0; i < 80; i++)
	//	vid::char_write(buf[i]);

	prd[0] = reinterpret_cast<natq>(vv);
	prd[1] = 0x80000000 | ((nn * 512) & 0xFFFF);
	bm::prepare(reinterpret_cast<natq>(prd), false);

	hd::enable_intr();
	hd::start_cmd(lba, nn, hd::READ_DMA);
	bm::start();

	printf("aspetto l'interrupt...\n");
	while (!done)
		;

	printf("primi 80 caratteri di vv:\n");
	for (int i = 0; i < 80; i++)
		vid::char_write(vv[i]);

	printf("ultimi 80 caratteri di vv:\n");
	for (natl i = BUFSIZE - 80; i < BUFSIZE; i++)
		vid::char_write(vv[i]);

	//printf("80 byte all'indirizzo %p\n", buf);
	//for (int i = 0; i < 80; i++)
	//	vid::char_write(buf[i]);
	pause();
}

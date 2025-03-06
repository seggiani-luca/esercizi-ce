#include <libce.h>

namespace timmer {

	const ioaddr iSPR = 0x61;
	const ioaddr iCWR = 0x43;
	const ioaddr iCTR2_LSB = 0x42;
	const ioaddr iCTR2_MSB = 0x42;

	void start2(natw N)
	{
		outputb(0xB6, iCWR);		// contatore 2, modo 3
		outputb(N, iCTR2_LSB);
		outputb(N >> 8,  iCTR2_MSB);
	}

	void enable_spk()
	{
		outputb(3, iSPR);
	}

	void disable_spk()
	{
		outputb(0, iSPR);
	}
};

void main()
{
	timer::start2(1190); 		// costante 1000 Hz
	timer::enable_spk();
	pause();
	timer::disable_spk();
}

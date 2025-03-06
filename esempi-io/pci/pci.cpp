#include <libce.h>

void main()
{
	natb bus = 0;
	for (natb dev = 0; dev < 32; dev++) {
		natw vendorID, deviceID;
		natb class_code;

		vendorID = pci::read_confw(bus, dev, 0, 0);
		if (vendorID == 0xFFFF) continue;
		deviceID = pci::read_confw(bus, dev, 0, 2);
		class_code = pci::read_confb(bus, dev, 0, 11);
		printf("%02x:%02x.%1d   %04x:%04x [%s]\n",
				bus, dev, 0,
				vendorID, deviceID,
				pci::decode_class(class_code));
		for (natb fun = 1; fun < 8; fun++) {
			vendorID = pci::read_confw(bus, dev, fun, 0);
			if (vendorID == 0xFFFF) continue;
			deviceID = pci::read_confw(bus, dev, fun, 2);
			class_code = pci::read_confb(bus, dev, fun, 11);
			printf("%02x:%02x.%1d   %04x:%04x [%s]\n",
					bus, dev, fun,
					vendorID, deviceID,
					pci::decode_class(class_code));
		}
	}
	pause();
}

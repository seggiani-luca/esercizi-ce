#include <libce.h>

namespace vid {

	natw* video = reinterpret_cast<natw*>(0xb8000);	// array di 2000 word;
				
}

void main()
{
	for(int i = 0; i < 2000; i++)
		vid::video[i] = 0x4B00 | 'a';	// sfondo rosso (0100), simbolo azzurro chiaro (1011)
	for (;;) {
		char c = kbd::char_read();
		if (c == 0x1B)
			break;			// carattere ASCII esc
	}
}

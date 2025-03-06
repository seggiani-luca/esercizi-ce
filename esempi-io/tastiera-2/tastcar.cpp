#include <libce.h>

namespace kbd {

const natl MAX_CODE = 29;
bool shift = false;
natb tab[MAX_CODE] = {  // tasti lettere (26), spazio, enter, esc
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x39, 0x1C, 0x01
};
natb tabmin[MAX_CODE] = {
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ' ', '\n', 0x1B
};
natb tabmai[MAX_CODE] = {
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', '\r', 0x1B
};


char conv(natb c)
{
	natb cc;
	natl pos = 0;

	while (pos < MAX_CODE && tab[pos] != c)
		pos++;
	if (pos == MAX_CODE)
		return 0;
	if (shift)
		cc = tabmai[pos];
	else
		cc = tabmin[pos];
	return cc;
}

char char_read()
{
	natb c;
	char a;

	do {
		c = get_code();
		if (c == 0x2A) // left shift make code
			shift = true;
		else if (c == 0xAA) // left shift break code
			shift = false;
	} while (c >= 0x80 || c == 0x2A); // make code;
	a = conv(c);	// conv() puo' restituire 0
	return a;	// 0 se tasto non riconosciuto
}

}

void main()
{
	char c;

	for (;;) {
		c = kbd::char_read(); // puo' restituire 0
		if (c == 0x1B) // carattere ASCII esc
			break;
		vid::char_write(c);	// non effettua azioni se c vale 0
	}
}

#include <libce.h>

const natl ROWS = vid::rows();
const natl COLS = vid::cols();

namespace vid {

	const ioaddr IND = 0x03D4;
	const ioaddr DAT = 0x03D5;
	const natl VIDEO_SIZE = COLS*ROWS;
	const natb CUR_HIGH = 0x0e;
	const natb CUR_LOW = 0x0f;

	natw attr;					// attributo colore
	natb x, y;					// coordinate x e y inizialmente a 0
	volatile natw* video = reinterpret_cast<natw*>(0xb8000);
		// array di 80*25 = 2000 posizioni, ciascuna di una parola

	void cursor()
	{
		natw pos = COLS * y + x;
		outputb(CUR_HIGH, IND);
		outputb(pos >> 0x8, DAT);
		outputb(CUR_LOW, IND);
		outputb(pos, DAT);
	}

	void clear(natb col)
	{
		attr = static_cast<natw>(col) << 8;
		for (natl i = 0; i < VIDEO_SIZE; i++)
			video[i] = attr | ' ';
		x = 0;
		y = 0;
		cursor();
	}

	void scroll()
	{
		for (natl i = 0; i < VIDEO_SIZE - COLS; i++)
			video[i] = video[i + COLS];
		for (natl i = 0; i < COLS; i++)
			video[VIDEO_SIZE - COLS + i] = attr | ' ';
		y--;
	}

	void char_write(char c)
	{
		switch (c) {
		case 0:
			break;
		case '\n': case '\r':
			x = 0;
			y++;
			if (y >= ROWS)
				scroll();
			break;
		default:
			video[y * COLS + x] = attr | c;
			x++;
			if (x >= COLS) {
				x = 0;
				y++;
			}
			if (y >= ROWS)
				scroll();
			break;
		}
		cursor();
	}

}

void main()
{
	natb c;
	vid::clear(0x4B);
	for (;;) {
		c = kbd::char_read();
		if (c == 0x1B)
			break;				// carattere ASCII esc
		vid::char_write(c);
	}
}

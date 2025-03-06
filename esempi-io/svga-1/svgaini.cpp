#include <libce.h>

const natl COLS = 1280;
const natl ROWS = 1024;

volatile natb* framebuffer;

void main()
{
	framebuffer = svga::config(COLS,ROWS);

	for (natl i = 0;  i <COLS; i++)
		for (natl j = 0; j < ROWS; j++)
			framebuffer[j * COLS + i] = 0x04;
	char c;
	do {
		c = kbd::char_read();
	} while (c != 0x1B);
}

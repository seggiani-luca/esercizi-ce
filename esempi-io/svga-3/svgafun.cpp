#include <libce.h>

const natl COLS = 1280;
const natl ROWS = 1024;

volatile natb* framebuffer;

const natl x0 = (COLS - 1) / 2;
const natl y0 = (ROWS - 1) / 2;

void put_pixel(natl a, natl b, natb c)
{
	framebuffer[COLS * b + a] = c;
}

void sfondo(natb c)
{
	for (natl i = 0; i < COLS; i++)
		for (natl j = 0; j < ROWS; j++)
			put_pixel(i, j, c);
}

void assi(char c)
{
	for (natl i = 0; i < COLS; i++)
		put_pixel(i, y0, c);
	for (natl j = 0; j < ROWS; j++)
		put_pixel(x0, j, c);
}

void main()
{
	framebuffer = svga::config(COLS,ROWS);
	long x, y, xc, yc;
	sfondo(0x36);									// giallo
	assi(0x01);
	// retta yc = xc/2 - 100
	for (x = 0; x < COLS; x++)
	{
		xc = x - x0;
		yc = xc / 2 - 100;
		y  = y0 - yc;
		if (y >=0 && y < ROWS)
			put_pixel(x, y, 0x02);				// verde scuro
	}
	// parabola yc = xc*xc/200 - xc/2 - 100
	for (x = 0; x < COLS; x++)
	{
		xc = x - x0;
		yc = xc * xc / 200 - xc * 2 - 100;
		y = y0 - yc;
		if (y >= 0 && y < ROWS)
			put_pixel(x, y, 0x04);				// rosso scuro
	}
	char c;
	do
		c = kbd::char_read();
	while (c != 0x1B);
}

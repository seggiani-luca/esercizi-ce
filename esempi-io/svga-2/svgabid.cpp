#include <libce.h>

const natl COLS = 1280;
const natl ROWS = 1024;

const natl x0 = (COLS - 1) / 2;
const natl y0 = (ROWS - 1) / 2;

volatile natb* framebuffer;

void sfondo(natb c)
{
	for (natl i = 0; i < COLS; i++)
		for (natl j = 0; j < ROWS; j++)
			framebuffer[j * COLS + i] = c;
}

void rettangolo(natb c)
{
	for (natl i = 0; i < COLS; i++)
		framebuffer[i] = c;			// linea in alto
	for (natl j = 0; j < ROWS; j++)
		framebuffer[COLS * j + (COLS - 1)] = c;	// linea a destra
	for (natl i = 0; i < COLS; i++)
		framebuffer[COLS * (ROWS - 1) + i] = c;	// linea in basso
	for (natl j = 0; j < ROWS; j++)
		framebuffer[COLS * j] = c;		// linea a sinistra
}
void assi(natb c)
{
	for (natl i = 0; i < COLS; i++)
		framebuffer[COLS * y0 + i] = c;		// asse cartesiano x
	for (natl j = 0; j < ROWS; j++)
		framebuffer[COLS * j + x0] = c;		// asse cartesiano y
}

void main()
{
	framebuffer = svga::config(COLS, ROWS);
	sfondo(0x36);							// giallo
	rettangolo(0x04);						// rosso scuro
	assi(0x01);							// blu scuro
	char c;
	do
		c = kbd::char_read();
	while (c != 0x1B);
}

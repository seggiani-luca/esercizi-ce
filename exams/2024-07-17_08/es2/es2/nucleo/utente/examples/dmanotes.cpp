#include <all.h>

static const int MAX_NAME = 16;
static const int MAX_BLOCKS = 10;
static const int MAX_INPUT = 10;

int atoi(const char *in)
{
	int rv = 0;
	const char *sc = in;
	while ((sc - in) < MAX_INPUT && *sc != '\0' && *sc >= '0' && *sc <= '9') {
		rv = (rv * 10) + (*sc - '0');
		sc++;
	}
	return rv;
}

struct dir_entry {
	natl block;
	natl dim;
	char name[MAX_NAME];
};

char dir[DIM_BLOCK] alignas(alignof(dir_entry));
char buf[DIM_BLOCK * MAX_BLOCKS];
char c;
char input[MAX_INPUT];
bool get_input(const char* prompt, char* buf, natq size);
void get_contents(char *buf, natq size);
void main()
{
	dmareadhd_n(dir, 0, 1);
	bool cont = true;
	natl max = 1;
	int sel;
	while (cont) {
		int i = 0;
		dir_entry* e = reinterpret_cast<dir_entry*>(dir);
		dir_entry* list = e;
		printf("List of notes:\n");
		while (e->block) {
			i++;
			printf("%d) %.*s\n", i, MAX_NAME, e->name);
			if (e->block + e->dim > max)
				max = e->block + e->dim;
			e++;
		}
		printf("c - create note; r - read note; w - write notes on disk; q - quit\n");
		printf("> ");
		readconsole(&c, 1);
		printf("\n");
		switch (c) {
		case 'c':
			if (!get_input("name", e->name, MAX_NAME))
				break;
			e->block = max;
			if (!get_input("dim", input, MAX_INPUT))
				break;
			e->dim = atoi(input);
			if (e->dim > MAX_BLOCKS) {
				printf("too large\n");
				e->block = 0;
				break;
			}
			printf("Write note contents. Terminate with an empty line\n");
			get_contents(buf, e->dim * DIM_BLOCK);
			dmawritehd_n(buf, e->block, e->dim);
			break;
		case 'r':
			if (!get_input("id", input, MAX_INPUT))
				break;
			sel = atoi(input);
			if (sel > i) {
				printf("no such note\n");
				break;
			}
			printf("contents of note %d:\n", sel);
			dmareadhd_n(buf, list[sel - 1].block, list[sel - 1].dim);
			printf("-------------------\n");
			printf("%s\n", buf);
			printf("-------------------\n");
			break;
		case 'w':
			dmawritehd_n(dir, 0, 1);
			printf("OK\n");
			break;
		case 'q':
			cont = false;
			break;
		}
	}

	terminate_p();
}

bool get_input(const char* prompt, char *buf, natq size)
{
	for (;;) {
		printf("%s? ", prompt);
		if (readconsole(buf, size) < size)
			return true;
		printf("\nToo large. a: abort; anything else: retry\n> ");
		readconsole(&c, 1);
		if (c == 'a')
			return false;
	}
}

void get_contents(char *buf, natq size)
{
	while (size) {
		natq recv = readconsole(buf, size);
		if (recv == size) {
			buf[recv - 1] = '\0';
			printf("\nToo large: truncated\n");
			break;
		}
		if (!recv)
			break;
		buf[recv] = '\n';
		recv++;
		buf += recv;
		size -= recv;
	}
}

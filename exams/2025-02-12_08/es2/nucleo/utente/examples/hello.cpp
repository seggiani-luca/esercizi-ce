#include <all.h>


const natq DIM_NOME = 80;
char nome[DIM_NOME + 1];
void main(natq a)
{
	for (;;) {
		printf("Ciao, come ti chiami? ");
		if (readconsole(nome, DIM_NOME + 1) == DIM_NOME + 1) {
			printf("Troppo lungo! Max %lu caratteri\n", DIM_NOME);
			continue;
		}
		break;
	}
	printf("Ciao %s, piacere di conoscerti!\n", nome);
	pause();

	terminate_p();
}

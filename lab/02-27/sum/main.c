#include <stdio.h>

int sum(int a, int b);

int main() {
	int x1 = 1;
	int x2 = 2;
	int x3 = sum(x1, x2);
	printf("%i + %i = %i\n", x1, x2, x3);
}

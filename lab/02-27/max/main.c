#include <stdio.h>

int max(int a, int b);
// int max(int a, int b) {
//	 if(a > b) return a;
//	 else return b;
// }

int main() {
	int a = 2;
	int b = 3;

	printf("max(%d, %d) = %d\n", a, b, max(a, b));
}

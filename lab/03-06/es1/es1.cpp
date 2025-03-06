#include <iostream>

// int sum(int x, int y) {
//	 return x + y;
// }

extern int sum(int x, int y);

int main() {
	int a = 6;
	int b = 9;
	std::cout << a << " + " << b << " = " << sum(a, b) << std::endl;

	std::cout << sizeof(a) << std::endl;
}

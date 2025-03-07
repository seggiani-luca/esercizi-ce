#include <iostream>

struct st {
	int a;
	int b;
};

// int sum(st+ s) {
//	 return s->a + s->b;
// }

extern int sum(st* s);

int main() {
	st s;
	s.a = 6;
	s.b = 9;

	std::cout << sum(&s) << std::endl;
}

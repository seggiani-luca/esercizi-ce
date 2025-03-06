#include <iostream>

struct st {
	int a;
	int b;
};

// void sum(st* r, st st1, st st2) {
// 	r->a = st1.a + st2.a;
// 	r->b = st1.b + st2.b;
// }

extern int sum(st*, st, st);

int main() {
	st s1 = {a: 1, b: 2};
	st s2 = {a: 3, b: 4};

	st r;

	sum(&r, s1, s2);
	
	std::cout << "a: " << r.a << ", b: " << r.b << std::endl;
}

#include <cstdint>
#include <iomanip>
#include <iostream>

typedef uint64_t vaddr;

uint64_t dim_region(int liv)
{
	uint64_t v = 1ULL << (liv * 9 + 12);
	return v;
}

vaddr base(vaddr v, int liv)
{
	uint64_t mask = dim_region(liv) - 1;
	return v & ~mask;
}

vaddr limit(vaddr v, int liv)
{
	uint64_t dr = dim_region(liv);
	uint64_t mask = dr - 1;
	return (v + dr - 1) & ~mask;
}

void prt_vaddr(vaddr v) {
	std::cout << std::setw(16) << std::setfill('0') << std::hex << v;
}

int main() {
	vaddr x = 0x00001000;
	vaddr y = 0x00002000;

	vaddr l = base(x, 0);
	vaddr u = limit(y, 0);

	std::cout << "r_beg:\t"; prt_vaddr(x);
	std::cout << "\nr_end:\t"; prt_vaddr(y);
	std::cout << "\nf_beg:\t"; prt_vaddr(l);
	std::cout << "\nf_end:\t"; prt_vaddr(u);
	std::cout << std::endl;

	return 0;
}

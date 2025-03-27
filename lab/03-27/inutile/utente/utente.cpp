#include <all.h>

void main() {
	flog(LOG_INFO, "chiamo inutile");

	natq ret = inutile(10, 20);

	flog(LOG_INFO, "ho ottenuto %lu", ret);
	
	terminate_p();
}

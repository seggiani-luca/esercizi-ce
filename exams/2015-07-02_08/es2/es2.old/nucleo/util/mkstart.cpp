#include <fstream>
#include <iomanip>
#include <costanti.h>
#include <libce.h>
#include <vm.h>

// Questo programma viene compilato ed eseguito per primo.  Produce i file
// conf.sh, start.mk e const.gdb nella directory conf.  Serve a pasare le
// informazioni contenute in costanti.h al Makefile (start.mk), allo script
// boot (conf.sh) e al debugger (const.gdb) 

using namespace std;

int main()
{
	static const natq PART_SIZE = dim_region(MAX_LIV - 1);
	const vaddr start_io = norm(I_MIO_C * PART_SIZE);
	const vaddr start_utente = norm(I_UTN_C * PART_SIZE);

	ofstream confsh("conf/conf.sh");
	confsh << "MEM=" << MEM_TOT/MiB << "\n";
	confsh.close();

	ofstream startmk("conf/start.mk");
	startmk << hex;
	startmk << "START_IO=0x"      << start_io << "\n";
	startmk << "START_UTENTE=0x"  << start_utente << "\n";
	startmk.close();

	ofstream startgdb("conf/const.gdb");
	startgdb << "set $MAX_LIV="      	<< MAX_LIV << "\n";
	startgdb << "set $MAX_SEM="      	<< MAX_SEM << "\n";
	startgdb << "set $SEL_CODICE_SISTEMA="  << SEL_CODICE_SISTEMA << "\n";
	startgdb << "set $SEL_CODICE_UTENTE="   << SEL_CODICE_UTENTE << "\n";
	startgdb << "set $SEL_DATI_UTENTE="     << SEL_DATI_UTENTE << "\n";
	startgdb << "set $MAX_PROC="     	<< MAX_PROC << "\n";
	startgdb << "set $MAX_PRIORITY="     	<< MAX_PRIORITY << "\n";
	startgdb << "set $MIN_PRIORITY="     	<< MIN_PRIORITY << "\n";
	startgdb << "set $I_SIS_C="		<< I_SIS_C << "\n";
	startgdb << "set $I_SIS_P="		<< I_SIS_P << "\n";
	startgdb << "set $I_MIO_C="		<< I_MIO_C << "\n";
	startgdb << "set $I_UTN_C="		<< I_UTN_C << "\n";
	startgdb << "set $I_UTN_P="		<< I_UTN_P << "\n";
	startgdb.close();

	return 0;
}

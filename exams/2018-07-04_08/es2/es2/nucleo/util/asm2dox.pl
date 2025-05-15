#!/usr/bin/perl
#

open IN, "<$ARGV[0]" || die $!;
MAIN: while (<IN>) {
	if (/^([_a-zA-Z]\w*):/) {
		$name = $1;
		s/.*:\s*(\/\/\s*\()?/void $name(/;
		s/\)?\s*$/)\n/;
	}
	s/\.cfi_startproc/{/;
	s/\.cfi_endproc/}/;
	s/\.global.*//;
	s/\.extern.*//;
	if (/.macro/) {
		s/\s+/,/g;
		s/\.macro\s*,(\w+)\s*,?/#define $1(/;
		s/,?$/)\n/;
		print $_;
		while (<IN>) {
			s/^/\/\//;
			print $_;
			if (/\.endm/) {
				next MAIN;
			}
		}
	}
	if (/^\s*carica_gate/) {
		s/carica_gate\s+(\w+)\s+(\w+)\s+(\w+)/carica_gate($1, , $3);/;
	}
	if (m{^\s*//\s*\(\s*ESAME\s+....-..-..}) {
		$drop = <IN>;
		print '/// @addtogroup esame ESAME' . "\n";
		print '/// @{' . "\n";
		next MAIN;
	}
	if (m{^\s*//\s*ESAME.*\)}) {
		print '/// @}' . "\n";
		next MAIN;
	}
	if (m{^\s*//\s*\(\s*SOLUZIONE\s+....-..-..}) {
		print '/// @cond' . "\n";
		next MAIN;
	}
	if (m{^\s*//\s*SOLUZIONE.*\)}) {
		print '/// @endcond' . "\n";
		next MAIN;
	}
	print $_;
}

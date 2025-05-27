#!/usr/bin/perl

open IN, "<$ARGV[0]" || die $!;
MAIN: while (<IN>) {
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

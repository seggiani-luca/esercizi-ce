#!/usr/bin/perl -n

BEGIN {
	use File::Basename;
	use File::Path qw/make_path/;
}


sub redir() {
	if ($cur eq $open) {
		return;
	}
	$path = $cur;
	if ($path !~ m{^[/<]}) {
		if (!$default_dir) {
			return;
		}
		$path ="$default_dir$path";
	}
	$path = "decpp/$path";
	close OUT;
	if (!$created{$cur}) {
		make_path(dirname($path));
		open OUT, ">", $path or die "$path: $!";
		$created{$cur} = 1;
	} else {
		open OUT, ">>", $path or die "$path: $!";
	}
	$open = $cur;
}

if (/^# \d+ "(.*)"/) {
	$tmp = $1;
	if ($tmp =~ m{/$}) {
		$default_dir = $tmp;
	} else {
		$cur = $tmp;
	}
} elsif (!/^$/) {
	&redir;
	print OUT;
}

END {
	close OUT;
}

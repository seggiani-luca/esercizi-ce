use JSON::XS;
use Data::Dumper;
use IO::Handle;

($dev = $0) =~ s/\.pl$//;

open $in, "<", "$dev.out";
open $out, ">", "$dev.in";

$out->autoflush(1);

$json = JSON::XS->new;

$config = {
		bars => [
			{
				name => 'ce-io',
				type => 'io',
				size => 4,
			}
		],
		'interrupt-pin' => 2
	};


my $CTL;
my $pend_intr;

sub dbg {
	my $arg = shift;
	if ($debug) {
		print($arg);
	}
}

sub send {
	my $msg = shift;
	my $r = $json->encode($msg);
	&dbg("### $0: sending '$r' on $ch\n");
	print $out "$r\n";
}

sub receive {
	while (<$in>) {
	       	if ($obj = $json->incr_parse($_)) {
			&dbg(Dumper($obj));
			return $obj;
		}
	}
	exit;
}

sub send_ret {
	my $arg = shift || 0;
	&send({ return => int($arg)});
}

sub send_intr {
	return unless ($CTL & 1);

	if (!$pend_intr) {
		&send({ "raise-irq" => 1 });
		$pend_intr = 1;
	}
}

sub clear_intr {
	if ($pend_intr) {
		&send({ "lower-irq" => 1 });
		$pend_intr = 0;
	}
}

while (1) {
	$obj = &receive;
	if ($$obj{'get-config'}) {
		&send($config);
		&send_ret;
	} elsif ($i = $$obj{'write'}) {
		$a = $i->{'addr'};
		$s = $i->{'size'};
		$v = $i->{'val'};
		&dbg("### $0: writing $v at $a, size $s\n");
		&send_ret if  ($s != 1);
		if ($a == 0) {
			$CTL = $v;
			&send_intr;
		}
		&send_ret;
	} elsif ($i = $$obj{'read'}) {
		$a = $i->{'addr'};
		&send_ret if  ($a % 4 || $a >= 12);
		$s = $i->{'size'};
		&send_ret if  ($s != 1);
		&dbg("### $0: reading from $a\n");
		if ($a == 0) {
			&clear_intr;
			&send_ret($CTL);
		}
	}			
}

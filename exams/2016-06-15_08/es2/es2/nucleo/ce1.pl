use JSON::XS;
use Data::Dumper;
use IO::Handle;

($dev = $0) =~ s/\.pl$//;

open $in, "<", "$dev.out";
open $out, ">", "$dev.in";
open $async, ">", "${dev}a.in";

$out->autoflush(1);
$async->autoflush(1);

$json = JSON::XS->new;

#$debug = 1;

$config = {
		bars => [
			{
				name => 'ce-io',
				type => 'io',
				size => 16,
			}
		],
		'interrupt-pin' => 2
	};


# registri visibili al programmatore
my $BMPTR;	# puntatore al buffer
my $BMLEN;	# lunghezza del buffer
my $CMD;	# comando (1: inizia scrittura)
my $STS;	# stato; la lettura risponde all'interrupt

# stato interno
my $pend_intr;	# 1 se e' stato inviato un interrupt e non si e' ancora
		# ricevuta risposta

sub dbg {
	my $msg = shift;
	print "$0: $msg" if $debug;
}

sub send {
	my ($ch, $msg) = @_;
	my $r = $json->encode($msg);
	&dbg("### $0: sending '$r' on $ch\n");
	if ($ch eq "sync") {
		print $out "$r\n";
	} else {
		print $async "$r\n";
	}
}

sub receive {
	while (<$in>) {
	       	if ($obj = $json->incr_parse($_)) {
			&dbg("<-" . Dumper($obj));
			return $obj;
		}
	}
	exit;
}


sub send_ret {
	my $arg = shift || 0;
	&send("sync", { return => int($arg)});
}

sub send_intr {
	if (!$pend_intr) {
		&send("async", { "raise-irq" => 1 });
		$pend_intr = 1;
	}
}

sub clear_intr {
	if ($pend_intr) {
		&send("async", { "lower-irq" => 1 });
		$pend_intr = 0;
	}
}

my $chr = 0;
sub start_dma {
	my $addr = int($BMPTR);
	my $len = int($BMLEN);
	&send("sync", {
		"dma-write" => {
			addr => $addr,
			len  => $len,
			bytes => [map { ($_ % 26) + 65 } $chr..($chr+$len-1)],
		}
	});
	$chr += $len;
	&receive;
	&send_intr;
}

while (1) {
	$obj = &receive;
	if ($$obj{'get-config'}) {
		&send("sync", $config);
		&send_ret;
	} elsif ($i = $$obj{'write'}) {
		$a = $i->{'addr'};
		$s = $i->{'size'};
		$v = $i->{'val'};
		&send_ret if  ($s != 4);
		&dbg("### $0: writing $v at $a\n");
		if ($a == 0) {
			$BMPTR = $v;
		} elsif ($a == 4) {
			$BMLEN = $v;
		} elsif ($a == 8) {
			$CMD = $v;
			if ($CMD == 1) {
				&start_dma;
			}
		} else {
			&dbg("scrittura di $v su STS ignorata\n");
		}
		&send_ret;
	} elsif ($i = $$obj{'read'}) {
		$a = $i->{'addr'};
		&send_ret if  ($a % 4);
		$s = $i->{'size'};
		&send_ret if  ($s != 4);
		&dbg("### $0: reading from $a\n");
		if ($a == 0) {
			&send_ret($BMPTR);
		} elsif ($a == 4) {
			&send_ret($BMLEN);
		} elsif ($a == 8) {
			&send_ret($CMD);
		} else {
			&clear_intr;
			&send_ret($STS);
		}
	}			
}


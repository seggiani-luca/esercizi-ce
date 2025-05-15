use JSON::XS;
use Data::Dumper;
use IO::Handle;

$debug = 1;
$dev = "ce1";

open $in, "<", "$dev.out";
open $out, ">", "$dev.in";

$out->autoflush(1);

$json = JSON::XS->new->allow_nonref;

#$debug = 1;

$config = {
		bars => [
			{
				name => 'ce-io',
				type => 'io',
				size => 32,
			}
		],
		'interrupt-pin' => 1,
	};

my $HEAD = int(0);
my $TAIL = int(0);
my $RING = int(0);
my $ERROR = int(0);
my $pend_intr;
my @messages = ();
my $sent = 0;

open M, "<utente/messages" or die $!;
while (<M>) {
	if (/^\s*MSG\((.*?)\s*,\s*(.*?)\s*,\s*"(.*?)"\s*\)/) {
		my ($src, $dst, $payload) = ($1, $2, $3);
		&dbg("src $src dst $dst payload $payload\n");
		push @messages, pack('L3 A*', hex($src), hex($dst), length($payload), $payload);
	}
}
close M;

sub dbg {
	my $msg = shift;
	if ($debug) {
		printf "$dev: $msg", @_;
	}
}

sub send {
	my $msg = shift;
	my $r = $json->encode($msg);
	&dbg("### $0: sending '$r' on sync\n");
	print $out "$r\n";
	&dbg("### $0: done sending\n");
}

sub receive {
	&dbg("### $0: receiving from sync\n");
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
	&send({ return => int($arg)});
}

sub send_intr {
	if (!$pend_intr) {
		&send({ "raise-irq" => 1 });
		&receive;
		$pend_intr = 1;
	}
}

sub clear_intr {
	if ($pend_intr) {
		&send({ "lower-irq" => 1 });
		&receive;
		$pend_intr = 0;
	}
}

sub get_natl {
	my $rv = int(0);
	for (my $i = 3; $i >= 0; $i--) {
		$rv <<= 8;
		$rv |= @_[$i];
	}
	return $rv;
}

sub get_des {
	my $i = shift;
	my $a = $RING + $i * 8;
	&send({ 'dma-read' => { addr => $a, len => 8 } });
	my $obj = &receive;
	my @b = @{$obj->{'bytes'}};
	my ($pa, $pl) = (&get_natl(@b[0..3]), &get_natl(@b[4..7]));
	return ($pa, $pl);
}

sub put_packet {
	my ($pa, $pl) = @_;
	my @bytes = unpack('(C)*', $messages[$sent % @messages]);
	my $al = @bytes;
	if ($al > $pl) {
		$al = $pl;
	}
	@bytes = @bytes[0..$al-1];
	&send({ 'dma-write' => { addr => $pa, len => $al, bytes => \@bytes } });
	&receive;
}

sub ring_next {
	my $i = shift;
	return ($i + 1) % 8;
}

my @torecv = ( 2, 0, 3, 5, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0);
my $curproc = 0;
sub proc_packets {
	my $n = $torecv[$curproc];
	&dbg("receiving $n packets\n");
	for (my $i = 0 ; $i < $n; $i++) {
		if ($HEAD != $TAIL) {
			my ($pa, $pl) = &get_des($HEAD);
			&dbg(sprintf("desc %d: %x %d\n", $HEAD, $pa, $pl));
			if ($pl > 0) {
				&put_packet($pa, $pl);
			} else {
				$ERROR++;
			}
			$HEAD = &ring_next($HEAD);
		}
		$sent++;
	}
	$curproc++;
	&send_intr;
}

while (1) {
	$obj = &receive;
	if ($$obj{'get-config'}) {
		&send($config);
		&send_ret;
	} elsif ($i = $$obj{'write'}) {
		$a = $i->{'addr'};
		if  ($a % 4) {
			&send_ret;
			next;
		}
		$s = $i->{'size'};
		if  ($s != 4) {
			&send_ret;
			next;
		}
		$v = $i->{'val'};
		&dbg("### writing $v at $a\n");
		if ($a == 0) {
			# read only
		} elsif ($a == 4) {
			$TAIL = $v;
			&dbg("TAIL $v curproc $curproc\n");
			&proc_packets;
		} elsif ($a == 8) {
			$RING = $v;
		} 
		&send_ret;
	} elsif ($i = $$obj{'read'}) {
		$a = $i->{'addr'};
		if  ($a % 4) {
			&send_ret;
			next;
		}
		$s = $i->{'size'};
		if  ($s != 4) {
			&send_ret;
			next;
		}
		&dbg("### reading from $a\n");
		if ($a == 0) {
			&clear_intr;
			&send_ret($HEAD);
		} elsif ($a == 4) {
			&send_ret($TAIL);
		} elsif ($a == 8) {
			&send_ret($RING);
		} elsif ($a == 12) {
			&send_ret($ERROR + (@messages - $recvd));
		}
	}			
}

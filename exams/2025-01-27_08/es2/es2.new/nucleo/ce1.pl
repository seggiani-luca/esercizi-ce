use JSON::XS;
use Data::Dumper;
use IO::Handle;
use Time::HiRes qw(ualarm);

#$debug = 1;

($dev = $0) =~ s/\.pl$//;

open $in, "<", "$dev.out";
open $out, ">", "$dev.in";
open $aout, ">", "${dev}a.in";
open $ain, "<", "${dev}a.out";

$out->autoflush(1);
$aout->autoflush(1);

$json = JSON::XS->new;

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


my $TBR = 0;		# 0
my $STS = 0;		# 4
my $DBG1 = 0;		# 8
my $DBG2 = 0;		# 12
my $pend_intr = 0;
my %seq = {};

sub dbg {
	my $msg = shift;
	print "$0: $msg" if $debug;
}

sub send {
	my $msg = shift;
	my $r = $json->encode($msg);
	&dbg("### $0: sending '$r' on sync\n");
	print $out "$r\n";
	&dbg("### $0: done sending on sync\n");
}

sub send_async {
	my $msg = shift;
	my $r = $json->encode($msg);
	&dbg("### $0: sending '$r' on async\n");
	print $aout "$r\n";
	&dbg("### $0: done sending on async\n");
}

sub receive {
	&dbg("### $0: receiving\n");
	while (<$in>) {
	       	if ($obj = $json->incr_parse($_)) {
			&dbg("SYNC<-" . Dumper($obj));
			return $obj;
		}
	}
	exit;
}

sub receive_async {
	&dbg("### $0: receiving from async\n");
	while (<$ain>) {
	       	if ($obj = $json->incr_parse($_)) {
			&dbg("ASYCN<-" . Dumper($obj));
			return $obj;
		}
	}
	exit;
}

sub send_ret {
	my $arg = shift || 0;
	&send({ return => int($arg)});
}

$SIG{ALRM} = sub { &send_async({ "raise-irq" => 1 }); &receive_async; $STS = 0; };

sub send_intr {
	&dbg("### $0: sending interrupt (pending: $pend_intr, STS: $STS)\n");
	if (!$pend_intr && $STS) {
		$pend_intr = 1;
		$done_sending = 0;
		&ualarm(10000);
	}
}

sub clear_intr {
	&dbg("### $0: clearing interrupt (pending: $pend_intr)\n");
	if ($pend_intr) {
		&send({ "lower-irq" => 1 });
		&receive;
		$pend_intr = 0;
	}
}

while (1) {
	$obj = &receive;
	if ($$obj{'get-config'}) {
		&send($config);
		&send_ret;
		&dbg("### CONFIGURATION SENT\n");
	} elsif ($i = $$obj{'write'}) {
		$a = $i->{'addr'};
		$s = $i->{'size'};
		$v = $i->{'val'};
		&send_ret if  ($s != 1 || ($a != 0 && $a != 8));
		if ($a == 0) {
			&clear_intr;
			if (!$STS) {
				&dbg("### ADDING $v TO TEST ".(int($v/16))."\n");
				push @{$seq{int($v / 16)}}, $v;
				$STS = 1;
				&send_intr;
			}
			&send_ret;
		} else {
			&dbg("### selected test: $v\n");
			$DBG1 = $v;
			&send_ret;
		}
	} elsif ($i = $$obj{'read'}) {
		$a = $i->{'addr'};
		&send_ret if  ($a % 4 || $a > 12);
		$s = $i->{'size'};
		&send_ret if  ($s != 1);
		&dbg("### $0: reading from $a\n");
		if ($a == 0) {
			&send_ret($TBR);
		} elsif ($a == 4) {
			&send_ret($STS);
		} elsif ($a == 8) {
			&send_ret(int(@{$seq{$DBG1}}));
		} elsif ($a == 12) {
			&send_ret(int(shift @{$seq{$DBG1}}));
		}
	}
}

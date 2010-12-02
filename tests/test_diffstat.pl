#
# pepper - SCM statistics report generator
# Copyright (C) 2010 Jonas Gehring
#
# Test group for diffstat testing
#

my @cases = glob("$ARGV[0]/*.pat");
foreach $case (@cases) {
	my $refpath = $case;
	$refpath =~ s/.pat$/.ref/;
	open(FILE, "$refpath") or die "Unable to open file: $!";
	my $reference = <FILE>;
   	close(FILE);	
	my $output = `pdiffstat < $case`;
	if ($output != $reference) {
		print("Output differes from reference for $case:\n");
		print("  out: $output");
		print("  ref: $reference");
		exit(1);
	} else {
		print("$case ok\n");
	}
}

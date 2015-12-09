#
# pepper - SCM statistics report generator
# Copyright (C) 2010-present Jonas Gehring
#
# Released under the GNU General Public License, version 3.
# Please see the COPYING file in the source distribution for license
# terms and conditions, or see http://www.gnu.org/licenses/.
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

#!/usr/bin/perl 

# makeclassdoc.pl - generate a libdoc file from ctalk.texi.
#
#  Usage: makeclassdoc.pl ctalk.texi
#
# The output is an updated libdoc file, which
# normally needs to be installed with, 
# "make install."
#
# Note that we don't use the '-w' option with Perl, because
# method names that contain a '%' character get interpolated
# when printing and that generates a warning.
#
# Note 2: The build procedure does not run this program
# automatically, so you should run it manually whenever
# making a change to the class libary sections of ctalk.texi.

my $docstartexpr=qr/^\@c CLASS-DOC-START-COOKIE/;
my $docendexpr=qr/^\@c CLASS-DOC-END-COOKIE/;
my $indoc = 0;
my $inex = 0;
my $inmethods = 0;
my $tmpname = "/tmp/doc.tmp";
my $docname = "classlibdoc";
my $rh;
my $wh;
my $methodname;
my $args;
my $tablelevel = 0;
my $levelindent = "";
my $inexample = 0;
my $classname = "";

open ($rh, ">", $tmpname) or die "$tmpname > : $!";

while (<>) {
    if ($_ =~ $docendexpr) {
	$indoc = 0;
    }
    if ($indoc) {
	print $rh "$_";
    }
    if ($_ =~ $docstartexpr) {
	$indoc = 1;
    }
}

close $rh;

open ($rh, "<", $tmpname) or die "$tmpname < : $!";
open ($wh, ">", $docname) or die "$docname > : $!";
while (<$rh>) {

    if ($_ =~ /\@node (\w+),/) {
	$classname = $1;
	$inmethod = 0;
	next;
    } 

    # skip over index keywords.
    if (($_ =~ /\@cindex/) || ($_ =~ /\@idxlibfn/) || ($_ =~ /\@idxfncite/) ||
	($_ =~ /\@anchor/)) {
	next;
    }

    # skip comments
    if ($_ =~ /\@c /) {
	next;
    }

    # Handle table indentation.
    if ($_ =~ /\@table/) {
	$tablelevel++;
	next;
    }
    if ($_ =~ /\@end table/) {
	$tablelevel--;
	next;
    }

    if (($_ =~ /\@subsubheading Instance Methods/)  ||
	($_ =~ /\@subheading Instance Methods/)) {
	$inmethod = 1;
	next;
    }

    if (!$inmethod) {
	next;
    }

    # Add fences for examples
    if ($_ =~ /\@example/) {
	print $wh "---------------------------------------------------------\n";
	$inexample = 1;
	next;
    }
    if ($_ =~ /\@end example/) {
	print $wh "---------------------------------------------------------\n";
	$inexample =  0;
	next;
    }
    if ($_ =~ /\@smallexample/) {
	print $wh "---------------------------------------------------------\n";
	$inexample = 1;
	next;
    }
    if ($_ =~ /\@end smallexample/) {
	print $wh "---------------------------------------------------------\n";
	$inexample =  0;
	next;
    }

    if ($inexample) {
	$_ =~ s/\@\{/\{/g;
	$_ =~ s/\@\}/\}/g;
	$_ =~ s/\%/\%\%/g;
    }

    # Remove markup tags.  Note the non-greedy match.  
    # Then we can later add our own markup
    # by enclosing the backreference.
    $_ =~ s/\@code\{(.*?)\}/$1/g;
    $_ =~ s/\@var\{(.*?)\}/$1/g;
    $_ =~ s/\@cite\{(.*?)\}/$1/g;
    $_ =~ s/\@samp\{(.*?)\}/'$1'/g;
    $_ =~ s/\@mnm\{(.*?)\}/$1/g;
    $_ =~ s/\@emph\{(.*?)\}/$1/g;
    $_ =~ s/\@flnm\{(.*?)\}/$1/g;
    $_ =~ s/\@xref\{.*?\}[.,]//g;
    $_ =~ s/\@b\{(.*?)\}/$1/g;

    if (m/\@item/) {

	$methodname = "";

	# m/\@item\s+(\S+)\s+\(/;
	m/\@item\s+(\S+)\s+/;
	$methodname = $1;

	if (m/(\(.*\))/) {
	    $args = $1;
	    if (m/\@code/) {
		$args =~ s/\@code\{(.*?)\}/$1/g;
	    }
	    if (m/\@var/) {
		$args =~ s/\@code\{(.*?)\}/$1/g;
	    }
	} else {
	    $args = "";
	}

	if (length $args == 0) {
	    if (!defined $methodname || length $methodname == 0) {
		printf $wh "|\n|\n-----------------------------------\n";
		printf $wh $_;
		printf $wh "-----------------------------------\n|\n|\n";
		printf $wh ">>>" . $classname . "::\n";
	    } else {
		printf $wh ">>>" . $classname . "::" . $methodname . "\n";
	    }
	} else {
	    printf $wh ">>>" . $classname . "::" . $methodname . 
		" " . $args . "\n";
	}
	next;
    }

    $levelindent = "";
    for ($i = 1; $i <  $tablelevel; $i++) {
	$levelindent = $levelindent . "  ";
    }
    if ($inexample) {
	$levelindent .= "  ";
    }
    printf $wh $levelindent . $_;

}
close $rh;
close $wh;

unlink $tmpname;

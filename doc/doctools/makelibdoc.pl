#!/usr/bin/perl -w

# makelibdoc.pl - generate a libdoc file from ctalk.texi.
#
#  Usage: makelibdoc.pl ctalk.texi
#
# The output is an updated libdoc file, which
# normally needs to be installed with, 
# "make install."
#
# Note: The build procedure does not run this program
# automatically, so you should run it whenever making
# a change to the libary API section of ctalk.texi.
#

my $docstartexpr=qr/^\@c LIB-DOC-START-COOKIE/;
my $docendexpr=qr/^\@c LIB-DOC-END-COOKIE/;
my $indoc = 0;
my $inex = 0;
my $tmpname = "/tmp/doc.tmp";
my $docname = "libdoc";
my $rh;
my $wh;
my $fnproto;
my $tablelevel = 0;
my $levelindent = "";
my $inexample = 0;
my $inproto = 0;
my $protoline = "";

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
read_a_line: while (<$rh>) {

    # skip over index keywords.
    if (($_ =~ /\@cindex/) || ($_ =~ /\@idxlibfn/) || ($_ =~ /\@idxfncite/) ||
	($_ =~ /\@anchor/)) {
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

    if ($inexample) {
	$_ =~ s/\@//g;
    }

    # Remove markup tags.  Note the non-greedy match.  
    # Then we can later add our own markup
    # by enclosing the backreference.
    $_ =~ s/\@code\{(.*?)\}/$1/g;
    $_ =~ s/\@var\{(.*?)\}/$1/g;
    $_ =~ s/\@cite\{(.*?)\}/$1/g;
    $_ =~ s/\@samp\{(.*?)\}/$1/g;
    $_ =~ s/\@mnm\{(.*?)\}/$1/g;
    $_ =~ s/\@emph\{(.*?)\}/$1/g;
    $_ =~ s/\@flnm\{(.*?)\}/$1/g;
    $_ =~ s/\@xref\{.*?\}[.,]//g;

    # make the search keys.
    if ($tablelevel == 0) {
	if (m/\@item/) {
	    $_ =~ /\@item\s*([_a-zA-Z0-9]*)(\s*)(\(.*\))/;
	    if (!defined $1) {
		print ">>>>>> NO FN MATCH";
		print "$_";
	    }
	    $fnproto = $1 . $2 . $3;
	    if ($inproto == 0) {
		$protoline = $fnproto;
		$inproto = 1;
	    } else {
		$protoline = $protoline . ":" . $fnproto;
	    }
	    goto read_a_line;
	}
    }

    if ($inproto == 1) {
	print $wh ">>>" . $protoline . "\n";
	$inproto = 0;
	$protoline = "";
    }

    if ($tablelevel == 0) {
	$_ =~ s/\@item //;
    } else {
	$_ =~ s/\@item /- /;
    }

    $levelindent = "";
    for ($i = 0; $i <  $tablelevel; $i++) {
	$levelindent = $levelindent . "  ";
    }
    printf $wh $levelindent . $_;
}
close $rh;
close $wh;

unlink $tmpname;

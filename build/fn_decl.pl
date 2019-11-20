#
#  Find function definitions in MacOS include files.  So far,
#  all the definitions have the opening brace as the first
#  character of the line after the fn declararator and param
#  list.
#
#!/usr/bin/perl -w

my $fn_name;
my $need_bracket;
my $after_paramlist;
my @keywords = (qw/for if while do/);

$need_bracket = 0;

loop: while (<>) {

    if ($need_bracket) {
	$need_bracket = 0;
	if ($_ =~ /\{/) {
#	    foreach my $m (@keywords) {
#		if ($fn_name eq $m) {
#		    goto loop;
#		}
#	    }
	    
	    foreach my $m (@keywords) {
		if ($fn_name eq $m) {
		    goto loop;
		}
	    }
	    # Output in quotes and trailing comma, suitable for
	    # a char * element in an array declaration.
	    print "\"$fn_name\",\n";

	} else {
	    goto loop;
	}
    }

    if ($_ =~ /^(\w+)\s*\(.*\)\R/) {
	$fn_name = $1;
	$need_bracket = 1;
    }
}


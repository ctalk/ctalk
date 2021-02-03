#!/bin/bash

#
# output multi-part HTML docs write the parts to the subdirectory,
# "doc/CtalkLanguageRef," and to the single file,
# "doc/CtalkLanguageRef.html."
#
# Run from the doc subdirectory.
#
makeinfo --html --output=CtalkLanguageRef \
	 --css-include=doctools/refstyle.css $@

# output single HTML file.
makeinfo --html --no-split --output=CtalkLanguageRef.html \
	 --css-include=doctools/refstyle.css $@
	 

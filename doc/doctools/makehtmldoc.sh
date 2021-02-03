#!/bin/bash

# output multi-part HTML docs write the parts to the subdirectory,
# "ctalk."
makeinfo --html --output=CtalkLanguageRef \
	 --css-include=doctools/refstyle.css $@

# output single HTML file.
makeinfo --html --no-split --output=CtalkLanguageRef.html \
	 --css-include=doctools/refstyle.css $@
	 

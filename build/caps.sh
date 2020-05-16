#!/bin/sh
# $Id: caps.sh,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $
#
# This is how we will get system capabilities into classes/ctalklib.
# Instead of writing a library function for every capability (although
# it's possible to do that also), 
#
# Note that this is not well tested, so we also keep a basic ctalklib 
# (with these capabilities defined as 0) so that if the caps.h script 
# doesn't work right, it won't break any builds.
#

# First, start with ctalklib with no capabilities set.
cat classes/ctalklib.in > classes/ctalklib

# Set the HAVE_GNU_READLINE #define if the machine has GNU 
# readline.
cap=`grep 'HAVE_GNU_READLINE 1' config.h`
if [ "$cap" != "" ]; then
    sed 's|// #include <readline/readline.h>|#include <readline/readline.h>|' classes/ctalklib >classes/ctalklib.out
     sed 's/#define HAVE_GNU_READLINE 0/#define HAVE_GNU_READLINE 1/' classes/ctalklib.out >classes/ctalklib.out.2
mv classes/ctalklib.out.2 classes/ctalklib
fi

# Do the same with HAVE_X_H
cap=`grep 'HAVE_X_H 1' config.h`
if [ "$cap" != "" ]; then
    sed 's|// #include <X11/Xlib.h>|#include <X11/Xlib.h>|' classes/ctalklib >classes/ctalklib.out
    sed 's/#define HAVE_X_H 0/#define HAVE_X_H 1/' classes/ctalklib.out >classes/ctalklib.out.2
mv classes/ctalklib.out.2 classes/ctalklib
fi

# Do the same with HAVE_STDBOOL_H...
cap=`grep 'HAVE_STDBOOL_H 1' config.h`
if [ "$cap" != "" ]; then
    sed 's/#define HAVE_STDBOOL_H 0/#define HAVE_STDBOOL_H 1/' classes/ctalklib >classes/ctalklib.out
mv classes/ctalklib.out classes/ctalklib
fi

# ...and HAVE_XFT_H ...  (Used only for testing.)
cap=`grep 'HAVE_XFT_H 1' config.h`
if [ "$cap" != "" ]; then
    sed 's/#define HAVE_XFT_H 0/#define HAVE_XFT_H 1/' classes/ctalklib >classes/ctalklib.out
mv classes/ctalklib.out classes/ctalklib
fi


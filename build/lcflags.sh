#!/bin/sh

help () {
    echo "Usage: lcflags \"CTALKFLAGS\" \"CFLAGS\" \"OPTIMIZINGFLAGS\" \"LDFLAGS\" \"X_CFLAGS\" \"X_LDFLAGS\""
    echo "   Configures default compile and link options for ctcc(1) and ctdb(1)."
    exit 1;
}

if [ $# -ne 6 ] ; then help; fi

ctalkflags=`echo $1 | sed s/\"//g | sed s/-/\\\-/g`
cflags=`echo $2 | sed s/\"//g`
optflags=`echo $3 | sed s/\"//g`
ldflags=`echo $4 | sed s/\"//g`
ct_x_cflags=`echo $5 | sed s/\"//g`
x_ldflags=`echo $6 | sed s/\"//g`

cat src/ctcc.in | sed -e "s|@ac_ctalkflags@|$ctalkflags|" | \
    sed "s|@ac_cflags@|$cflags|" | \
    sed "s|@ac_optflags@|$optflags|" | \
    sed "s|@ac_ldflags@|$ldflags|" | \
    sed "s|@ct_x_cflags@|$ct_x_cflags|" | \
    sed "s|@ac_xldflags@|$x_ldflags|" > src/ctcc

cat src/ctdb.in | sed -e "s|@ac_ctalkflags@|$ctalkflags|" | \
    sed "s|@ac_cflags@|$cflags|" | \
    sed "s|@ac_ldflags@|$ldflags|" | \
    sed "s|@ct_x_cflags@|$ct_x_cflags|" | \
    sed "s|@ac_xldflags@|$x_ldflags|" > src/ctdb

# $cflags and $x_ldflags aren't needed for the test programs,
# but try substituting here anyway.
cat test/expect/cttest.in | sed -e "s|@ac_ctalkflags@|$ctalkflags|" | \
    sed "s|@ac_cflags@|$cflags|" | \
    sed "s|@ac_ldflags@|$ldflags|" | \
    sed "s|@ac_xldflags@|$x_ldflags|" > test/expect/cttest
chmod +x test/expect/cttest


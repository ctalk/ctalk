#!/bin/bash

GCC=`which gcc`

cat <<EOF >gcctest.c
#include <stdlib.h>
int main () {exit (0);}
EOF

$GCC -Wno-long-double gcctest.c -o gcctest >gcctest.out 2>&1

RET=$?

# Remember, this script is run from the top-level build directory.
if [ $RET -ne 0 ]; then 
    touch build/.gccnolongdouble
else
    touch build/.gcchavelongdouble
fi

$GCC -framework Cocoa gcctest.c -o gcctest >gcctest.out 2>&1

RET=$?

if [ $RET -ne 0 ]; then 
    touch build/.gccnoframeworkopt
else
    touch build/.gcchaveframeworkopt
fi

$GCC -Wno-format-overflow gcctest.c -o gcctest >gcctest.out 2>&1

grep '\-Wno-format-overflow' gcctest.out  >>gcctest.out 2>&1
RET=$?

if [ $RET -eq 0 ]; then 
    touch build/.gccdonthavenoformatoverflowopt
else
    touch build/.gcchavenoformatoverflowopt
fi

rm -f gcctest gcctest.c gcctest.out

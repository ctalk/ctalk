#!/bin/bash

gcc --version > ./gccver.out 2>&1
grep 'LLVM' ./gccver.out  >/dev/null 2>&1
retval="$?"

if [ "x$retval" = "x0" ]; then
    touch ./build/.llvm
fi
rm -f ./gccver.out


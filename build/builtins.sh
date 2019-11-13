#!/bin/sh

# Compile and run make_builtins.  
# The directory paths are not absolute here. The script is 
# meant to be run from the top-level build directory.

BUILTINS_FILE=include/builtins.h

echo Creating $BUILTINS_FILE

gcc -g build/make_builtins.c -o build/make_builtins

build/make_builtins >$BUILTINS_FILE
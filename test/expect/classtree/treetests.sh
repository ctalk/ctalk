#!/bin/bash

#
#  Builds all of the class?.c programs in this directory, 
#  then executes them and compares the output.
#

SRCS=class1.c class2.c class3.c class4.c class5.c class6.c class7.c
BINS=class1 class2 class3 class4 class5 class6 class7

for i in [ $SRCS ]; do ctcc -I . --nopreload $i -o `basename $i .c`; done

for i in [ $BINS ]; do ./$i -q > $i.out ; done

diff -q class1.out class2.out
diff -q class2.out class3.out
diff -q class3.out class4.out
diff -q class4.out class5.out
diff -q class5.out class6.out
diff -q class6.out class7.out

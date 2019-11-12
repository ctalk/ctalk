#
# This appears to be the only universally valid way of checking for the
# -pthread option in GCC -- Eech.
#
#!/bin/sh

PTHREAD_FN=build/.pthread_gcc_option

rm -f $PTHREAD_FN

if [ ! -z "$CC" ]; then
    my_cc=$CC;
else
    my_cc=`which gcc`
fi

if [ -z "$my_cc" ] ; then
    rm -f $PTHREAD_FN >/dev/null 2>&1
    exit 1;
fi

if [ ! -f "$my_cc" ] ; then
    rm -f $PTHREAD_FN >/dev/null 2>&1
    exit 1;
fi

strings $my_cc | grep '\-pthread' >/dev/null 2>&1

if [ $? = 0 ]; then
    touch $PTHREAD_FN
else
    rm -f $PTHREAD_FN >/dev/null 2>&1
fi
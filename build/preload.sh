#!/bin/bash

# Paths are configured for the script to be run from the top 
# Ctalk build directory.

CONF_USER=`cat build/.confname`

CTALK=`which ctalk`

CTALKDIR=`cat build/.confuserhomedir`/.ctalk
CTALKCACHEDIR=$CTALKDIR/libcache

CLASSDIR=`cat ./build/.includedir`

if [ -f ./build/.x_h_path ]; then

XINCLUDEDIR="-I "`cat ./build/.x_h_path`
    
else

XINCLUDEDIR=""    
    
fi    


# if [ ! -z $LOGNAME ]; then
#    ACTUAL_USER=$LOGNAME
#else if [ ! -z $EUID ]; then
#    ACTUAL_USER=$EUID
#fi
#fi

# 
# These are the classes that are used by preload.ca.  If rewriting
# preload.ca, then also check this list.  We want to replace only
# the base classes when installing.
#
PRELOADCLASSES="Application ArgumentList Array Boolean Character \
	Collection Event Exception FileStream Float Integer \
	Key List LongInteger Magnitude Method Object \
	ObjectInspector ReadFileStream SignalEvent SignalHandler \
    Stream String Symbol SystemErrnoException WriteFileStream"


if [ -f build/.nopreload ]; then
    echo
    echo Skipping base class preloading.
    echo
    exit 0
fi


echo 
echo Preloading base classes.
echo 

if [ -z $CONF_USER ]; then
    echo
    echo Warning: Cannot determine the actual user name. You
    echo can still use Ctalk without preloading during installation, 
    echo but it will run more slowly the first few times you compile 
    echo programs.
    echo
    exit 1
fi

if [ -d $CTALKCACHEDIR ]; then
    for i in $PRELOADCLASSES; do
	rm -f $CTALKCACHEDIR/$i*;
    done
fi

#
# Preload the classes and set some reasonable args.
#
if [ $UID = 0 ]; then
    su $CONF_USER -c "$CTALK --clearpreload"
    su $CONF_USER -c "$CTALK --progress $XINCLUDEDIR -I$CLASSDIR build/preload.ca -o build/preload.i"
    su $CONF_USER -c "echo -n -I$CLASSDIR >$CTALKDIR/args"
else
    $CTALK --clearpreload
    $CTALK --progress $XINCLUDEDIR -I$CLASSDIR build/preload.ca -o build/preload.i
    echo -n -I$CLASSDIR >$CTALKDIR/args
fi

echo
echo

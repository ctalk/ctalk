#!/bin/sh

SEARCHDIRS_FILE=include/searchdirs.h
echo Creating $SEARCHDIRS_FILE
echo "/* This is a machine generated file.  Do not edit! */" > $SEARCHDIRS_FILE
echo >>$SEARCHDIRS_FILE
echo char *searchdirs[] = { >> $SEARCHDIRS_FILE

CPPOUT=`echo ' ' | cpp -E -v - 2>&1`
EXPR=`which expr`

if [ -z '$CPPOUT' ]; then
    echo 'include_dirs.sh: Cannot run "cpp -E -v:" See cpp(1). Exiting'
    exit 1
fi

if [ -z $EXPR ]; then
    echo 'include_dirs.sh: Cannot find "expr:" See expr(1). Exiting.'
    exit 1
fi

N_PATHS=0

for i in $CPPOUT; do
    if [ -d $i ]; then 
	echo \"$i\", >>$SEARCHDIRS_FILE;
	N_PATHS=$(expr $N_PATHS + 1)
    fi 
done

# Darwin still places the X include files (like X11/Xlib.h) under
# /usr/X11R6/include.  Ctalk always includes at least X11/Xlib.h,
# so add it automagically.
echo $OSTYPE | grep 'darwin' > /dev/null 2>&1
if [ "x$?" = "x0" ]; then
  echo "\"/usr/X11R6/include\"," >> $SEARCHDIRS_FILE
fi
N_PATHS=$(expr $N_PATHS + 1)

echo '(void *)0' >>$SEARCHDIRS_FILE
echo '};' >>$SEARCHDIRS_FILE
echo '#define __have_preloaded_searchdirs' >> $SEARCHDIRS_FILE
echo '#define N_PATHS '$N_PATHS >>$SEARCHDIRS_FILE


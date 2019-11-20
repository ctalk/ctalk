#!/bin/sh

# The sed expressions are as portable as possible.  The script is
# complicated by the fact that a header line can contain any number of
# builtin labels.

PERL=`which perl`

function extract_builtins () {
    cat $1 | sed 's/ /\n/' | \
    sed 's/	/\n/' | \
	sed 's/=/\n/' | \
	sed 's/;/\n/' | \
	sed 's/{/\n/' | \
	sed 's/}/\n/' | \
	sed 's/>/\n/' | \
	sed 's/</\n/' | \
	sed 's/\\(/\n/'  | \
	grep '__builtin_' | \
	sed 's/.*\(__builtin_[a-zA-Z]*\).*/\1/g' | \
	sed 's/\(.*\)/\"\1\"\,/g'  
}

for i in /usr/include/architecture/ppc/*; do \
    if [ -f /usr/include/architecture/ppc/math.h ]; then
      extract_builtins /usr/include/architecture/ppc/math.h \
      >build/builtin_names
    fi
done
for i in /usr/include/*.h; do 
    extract_builtins $i >>build/builtin_names;
    if [ ! -z $PERL ]; then 
	cat $i | perl build/fn_decl.pl >> build/builtin_names ;
    fi ;
done
for i in /usr/include/sys/*.h; do \
    extract_builtins $i >>build/builtin_names
    if [ ! -z $PERL ]; then 
	cat $i | perl build/fn_decl.pl >> build/builtin_names ;
    fi ;
done

echo "/* This is a machine generated file.  Please do not edit! */" >include/osx_ppc_builtins.h 
echo 'char *__osx_ppc_math_builtins[] = {' >>include/osx_ppc_builtins.h
cat build/builtin_names | sort | uniq >> include/osx_ppc_builtins.h
echo 'NULL, NULL };' >>include/osx_ppc_builtins.h 

echo 'char *__osx_ppc_libkern_builtins[] = {' >>include/osx_ppc_builtins.h
if [ -f /usr/include/libkern/ppc/OSByteOrder.h ]; then
    cat /usr/include/libkern/ppc/OSByteOrder.h | \
	sed '/^#/d' | \
	sed '/__asm__[ \t]*/,/);/d' | \
	sed '/{/,/}/d'  | \
	sed '/return/d' | \
	sed 's/(/\
 (/'   | \
	sed '/(/,/)/d' | \
	sed '/^[^a-zA-Z0-9_]/d' | \
	sort | \
	uniq -c | \
	awk '{ if($1 == '1') print $2 }' | \
	sed 's/\(.*\)/\"\1\"\,/g' >>include/osx_ppc_builtins.h
fi
echo 'NULL, NULL };' >>include/osx_ppc_builtins.h

if [ -f /usr/include/architecture/i386/math.h ]; then
    extract_builtins /usr/include/architecture/i386/math.h \
	>build/builtin_names
fi

echo "/* This is a machine generated file.  Please do not edit! */" >include/osx_i386_builtins.h
echo 'char *__osx_i386_math_builtins[] = {' >>include/osx_i386_builtins.h
cat build/builtin_names | sort | uniq >> include/osx_i386_builtins.h
echo 'NULL, NULL };' >>include/osx_i386_builtins.h

echo 'char *__osx_i386_libkern_builtins[] = {' >>include/osx_i386_builtins.h
if [ -f /usr/include/libkern/i386/OSByteOrder.h ]; then
    cat /usr/include/libkern/i386/OSByteOrder.h | \
	sed '/^#/d' | \
	sed '/__asm__[ \t]*/,/);/d' | \
	sed '/{/,/}/d'  | \
	sed '/return/d' | \
	sed 's/(/\
 (/'   | \
	sed '/(/,/)/d' | \
	sed '/^[^a-zA-Z0-9_]/d' | \
	sort | \
	uniq -c | \
	awk '{ if($1 == '1') print $2 }' | \
	sed 's/\(.*\)/\"\1\"\,/g' >>include/osx_i386_builtins.h
fi
if [ -f /usr/include/libkern/i386/_OSByteOrder.h ]; then
    cat /usr/include/libkern/i386/_OSByteOrder.h | \
	sed '/^#/d' | \
	sed '/__asm__[ \t]*/,/);/d' | \
	sed '/{/,/}/d'  | \
	sed '/return/d' | \
	sed 's/(/\
 (/'   | \
	sed '/(/,/)/d' | \
	sed '/^[^a-zA-Z0-9_]/d' | \
	sort | \
	uniq -c | \
	awk '{ if($1 == '1') print $2 }' | \
	sed 's/\(.*\)/\"\1\"\,/g' >>include/osx_i386_builtins.h
fi
echo 'NULL, NULL };' >>include/osx_i386_builtins.h

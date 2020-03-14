#!/bin/bash

# remember that this gets called after lcflags.sh, which makes the first copy
# of the actual cttest.

if [ ! -f test/expect/cttest.in ]; then
   exit 0;
fi

echo Configuring GCC capabilities in test/expect/ctest.

if [ -f build/.cttest_no_pointer_to_int_cast ]; then
    cat test/expect/cttest | sed s/@gcc_no_pointer_to_int_cast@/-Wno-pointer-to-int-cast/ > test/expect/cttest.2
else
    cat test/expect/cttest | sed s/@gcc_no_pointer_to_int_cast@// > test/expect/cttest.2
fi    

if [ -f build/.cttest_no_format_overflow ]; then
    cat test/expect/cttest.2 | sed s/@gcc_no_format_overflow@/-Wno-format-overflow/ > test/expect/cttest.3
else
    cat test/expect/cttest.2 | sed s/@gcc_no_format_overflow@// > test/expect/cttest.3
fi    

if [ -f build/.cttest_no_address_of_packed_member ]; then
    cat test/expect/cttest.3 | sed s/@gcc_no_address_of_packed_member@/-Wno-address-of-packed-member/ > test/expect/cttest
else
    cat test/expect/cttest.3 | sed s/@gcc_no_address_of_packed_member@// > test/expect/cttest
fi    

rm -f test/expect/cttest.2 test/expect/cttest.3



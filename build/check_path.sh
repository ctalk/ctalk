#!/bin/sh

echo $PATH | grep /usr/local/bin > /dev/null

if [ $? != 0 ]; then

cat <<EOF
------------------------------------------------------
Warning : Your search path does not contain the Ctalk
Warning : installation directory, /usr/local/bin.
Warning : To add the directory to the search path,
Warning : use the following shell command.

Warning :   set PATH=/usr/local/bin:\$PATH; export PATH

Warning : To set the search path on startup, refer to
Warning : the system's documentation.
------------------------------------------------------
EOF
fi
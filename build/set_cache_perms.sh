#!/bin/sh

INSTUSER=`cat .confname`
INSTGROUP=`cat .confgroup`
INSTUSERHOMEDIR=`cat .confuserhomedir`
CTALKDIR=`cat .confuserhomedir`/.ctalk

if [ -d $CTALKDIR ]; then
    cd $INSTUSERHOMEDIR && chown -R $INSTUSER .ctalk
    cd $INSTUSERHOMEDIR && chgrp -R $INSTGROUP .ctalk
fi

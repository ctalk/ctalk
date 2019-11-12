#!/bin/sh

INSTUSER=`cat .confname`
INSTGROUP=`cat .confgroup`
INSTUSERHOMEDIR=`cat .confuserhomedir`

cd $INSTUSERHOMEDIR && chown -R $INSTUSER .ctalk
cd $INSTUSERHOMEDIR && chgrp -R $INSTGROUP .ctalk

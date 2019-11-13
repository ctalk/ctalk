#!/bin/bash

CONF_USER=`cat build/.confname`
CONF_GROUP=`cat build/.confgroup`
CTALKDIR=`cat build/.confuserhomedir`/.ctalk

if [ -d $CTALKDIR ]; then
    chown -R $CONF_USER $CTALKDIR
    chgrp -R $CONF_GROUP $CTALKDIR
fi

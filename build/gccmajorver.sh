#!/bin/sh
case `uname -a` in 
    SunOS*5.8*)
gcc -print-libgcc-file-name | /opt/sfw/bin/gsed -e 's/\/[^/]*$//' | \
    /opt/sfw/bin/gsed -e 's/\(\/[^/]*\)*\/\([0-9]\)/\2/' | \
    /usr/bin/cut -f 1 -d .
    ;;
    *)
gcc -print-libgcc-file-name | sed -e 's/\/[^/]*$//' | \
    sed -e 's/\(\/[^/]*\)*\/\([0-9]\)/\2/' | cut -f 1 -d .
    ;;
esac

    


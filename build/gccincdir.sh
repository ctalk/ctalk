#!/bin/sh
gcc -print-libgcc-file-name | sed -e 's/\/[^/]*$/\/include/'


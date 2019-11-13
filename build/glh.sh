#!/bin/bash

# The first argument is a user-supplied include directory.
# Then check the standard locations.

incdirs="/usr/X11R6/include /usr/include /usr/local/include"

if [ "$1" = "no" ]; then
    touch build/.no_gl_h;
    touch build/.no_glx_h;
    touch build/.no_glew_h;
    exit 0;
fi
    

if [ ! -z $1 ]; then
    if [ -f "$1/GL/gl.h" ]; then
	echo "$1" > build/.gl_h_path
    else
	touch build/.no_gl_h
    fi

    if [ -f "$1/GL/glx.h" ]; then
	echo "$1" > build/.glx_h_path
    else
	touch build/.no_glx_h
    fi

    if [ -f "$1/GL/glew.h" ]; then
	echo "$1" > build/.glew_h_path
    else
	touch build/.no_glew_h
    fi
	
fi

for dir  in $incdirs ; do
    if [ ! -f build/.gl_h_path ] && [ -f "$dir/GL/gl.h" ]; then
	rm -f build/.no_gl_h;
	echo "$dir" > build/.gl_h_path;
    fi
    if [ ! -f build/.glx_h_path ] && [ -f "$dir/GL/glx.h" ]; then
	rm -f build/.no_glx_h;
	echo "$dir" > build/.glx_h_path
    fi
    if [ ! -f build/.glew_h_path ] && [ -f "$dir/GL/glew.h" ]; then
	rm -f build/.no_glew_h;
	echo "$dir" > build/.glew_h_path
    fi
done

if [ ! -f build/.gl_h_path ]; then touch build/.no_gl_h; fi
if [ ! -f build/.glx_h_path ]; then touch build/.no_glx_h; fi
if [ ! -f build/.glew_h_path ]; then touch build/.no_glew_h; fi

exit 0

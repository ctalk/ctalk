# $Id: Makefile.am,v 1.2 2019/10/29 20:38:55 rkiesling Exp $

SUBDIRS = 
EXTRA_DIST=Makefile.djgpp

libctpp_la_SOURCES=ansitime.c bintodec.c bnamecmp.c error_out.c errorloc.c \
	hash.c lex.c lextype.c lineinfo.c list.c keyword.c message.c \
	radixof.c read.c rtinfo.c rt_error.c sformat.c statfile.c \
	strcatx.c \
	substrcpy.c tempname.c tempio.c trimstr.c val.c xalloc.c

libctpp_la_LTLIBOBJS=ansitime.lo bintodec.lo bnamecmp.lo error_out.lo \
	errorloc.lo \
	hash.lo lex.lo lextype.lo lineinfo.lo list.lo keyword.lo message.lo \
	radixof.lo read.c rtinfo.lo rt_error.lo sformat.lo statfile.lo \
	strcatx.lo \
	substrcpy.lo tempname.lo tempio.lo trimstr.lo val.lo xalloc.lo

lib_LTLIBRARIES=libctpp.la

libctpp_la_LDFLAGS=-version-info $(LIB_MAJOR_VERSION):$(LIB_MINOR_VERSION)

AM_CFLAGS=-g -Wall 
AM_CPPFLAGS=-I$(top_builddir)/include
DEFS=-DPKGNAME=\"$(PACKAGE_NAME)\" -DCLASSLIBDIR=\"$(pkgincludedir)\" -DVERSION=\"$(PACKAGE_VERSION)\" -DLIB_BUILD @have_stdbool_def@ @use_pthreads@ @without_signed_hex_octal_constant_warnings_def@ @no_restrict_warning@ @no_stringop_overflow_warning@ @no_unknown_warning_warning@

distclean-local:
	rm -f $(top_builddir)/include/searchdirs.h 
	rm -f $(top_builddir)/include/builtins.h
	rm -f $(top_builddir)/tools/make_builtins


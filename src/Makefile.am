# $Id: Makefile.am,v 1.2 2019/10/29 20:30:43 rkiesling Exp $

SUBDIRS=
LIBEXT=@lib_ext@

EXTRA_DIST=$(top_builddir)/src/Makefile.djgpp

bin_PROGRAMS=ctalk

bin_SCRIPTS=ctdb ctcc

ctalk_SOURCES=argblk.c arg.c argexprchk.c ccompat.c class.c collection.c \
	complexmethd.c constrcvr.c control.c cparse.c ctoobj.c cvars.c \
	cvartab.c c_rval.c enum.c error.c errmsgs.c eval_arg.c extern.c \
	fnbuf.c fninit.c fn_param.c fn_tmpl.c \
	frame.c ifexpr.c include.c libdeps.c loop.c main.c math.c \
	mcct.c methodbuf.c \
	method.c methodgt.c mthdrep.c mthdrf.c mthdret.c objderef.c \
	object.c objtoc.c \
	op.c output.c parser.c pexcept.c prefixop.c preclass.c premethod.c \
	primitives.c rcvrexpr.c reg_cvar.c resolve.c refobj.c return.c rexpr.c \
	rt_expr.c stack.c subexpr.c subscr.c symbol.c typecast_expr.c \
	typecheck.c ufntmpl.c 

ctalk_INCLUDES=argblk.h break.h bufmthd.h chash.h classlib.h ctalk.h 
	ctrlblk.h cvar.h defcls.h djgpp.h except.h fnstate.h fndeclstate.h \
	fntmpl.h frame.h lex.h lextab.h list.h message.h \
	method_decl_states.h object.h objtoc.h option.h paramstate.h \
	parser.h pending.h rt_expr.h rtinfo.h stdargcall.h strs.h \
	symbol.h typecaststate.h typeof.h ufntmpl.h uproto.h \
	vartag.h x11defs.h xftfont.h xlibfont.h xrender.h

USE_PTHREADS=@use_pthreads@

AM_CFLAGS=$(PARSER_AM_CFLAGS) @no_strict_overflow_warning@
AM_CPPFLAGS=-I$(top_builddir)/include $(CTALK_X_CFLAGS)
LIBS=$(top_builddir)/lib/.libs/libctalk$(LIBEXT) $(LIB_AM_LDFLAGS)
DEFS=-DPKGNAME=\"$(PACKAGE)\" -DCLASSLIBDIR=\"$(pkgincludedir)\" -DVERSION=\"$(VERSION)\" $(X_LIBS) $(USE_PTHREADS) @no_restrict_warning@ @no_stringop_overflow_warning@ @stringop_truncation_warning@ @address_of_packed_member_warning@ @have_glx_h_def@ @no_unknown_warning_warning@

ctdb:
	if [ $$EUID == 0 ]; then \
		install $(top_srcdir)/src/ctdb $(bindir); \
	fi
ctcc:
	if [ $$EUID == 0 ]; then \
		install $(top_srcdir)/src/ctcc $(bindir); \
	fi

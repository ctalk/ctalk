# $Id: Makefile.am,v 1.2 2019/10/29 20:38:54 rkiesling Exp $

SUBDIRS= 
LIBEXT=@lib_ext@
EXTRA_DIST=Makefile.djgpp $(top_builddir)/ctpp/config/*.h

bin_PROGRAMS=ctpp

ctpp_SOURCES=ansisymbols.c assert.c a_opt.c builtins.c ccompat.c cexpr.c \
	chkstate.c d_opt.c error.c escline.c include.c is_fn.c is_methd.c \
	i_opt.c lineinfo.c macprint.c main.c m_opt.c pcvars.c pmath.c \
	ppexcept.c ppop.c pragma.c preprocess.c psubexpr.c type_of.c u_opt.c
ctpp_INCLUDES=ctpp.h djgpp.h pexcept.h phash.h plex.h list.h pmessage.h \
	pparser.h prtinfo.h typeof.h pval.h \
	$(top_builddir)/ctpp/config/*.h


USE_PTHREADS=@use_pthreads@
GENERIC_PATH=@generic_path@
CTPP_GNUC_VERSION=@gnuc_version@
AM_CFLAGS=-g -Wall $(CONF_INCLUDE_PATH) $(GENERIC_PATH) \
	$(INLINE_FUNCTION_OPT) $(PARSER_OPT_FLAGS) $(HAVE_OFF_T) \
	$(GCC_STDINT_WRAPPER) $(WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS) \
	$(NO_BUILTINS) $(USE_OLD_BUILTINS) $(MIT_POSIX_PATH) \
	@have_stdbool_def@ @no_fmt_trunc_warning@ @no_restrict_warning@ \
	@no_stringop_overflow_warning@ @no_unknown_warning_warning@
AM_CPPFLAGS=-I$(top_builddir)/include
LIBS=$(top_builddir)/libctpp/.libs/libctpp$(LIBEXT)
DEFS=-DPKGNAME=\"$(PACKAGE_NAME)\" -DVERSION=\"$(PACKAGE_VERSION)\" -DCLASSLIBDIR=\"$(pkgincludedir)\" $(USE_PTHREADS) $(CTPP_GNUC_VERSION)



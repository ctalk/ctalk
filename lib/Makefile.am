# $Id: Makefile.am,v 1.4 2019/12/06 22:46:11 rkiesling Exp $

SUBDIRS = 

EXTRA_DIST=Makefile.djgpp

libctalk_la_SOURCES=ansitime.c become.c bintodec.c bitmap.c bnamecmp.c \
	cclasses.c \
	chash.c clibtmpl.c ctatoll.c ctdtoa.c ctitoa.c ctlltoa.c ctltoa.c \
	ctoobj.c ctsleep.c cttmpl.c dblib.c edittext.c errorloc.c error_out.c \
	escstr.c event.c except.c exec_bin.c fileglob.c fmtargtype.c \
	font.c fsecure.c ftlib.c glewlib.c glutlib.c glxlib.c \
	guiclearrectangle.c guiclearwindow.c \
	guidrawline.c guidrawpoint.c guidrawrectangle.c guiputstr.c \
	guiputxstr.c guirefresh.c guisetbackground.c \
	guisetforeground.c guitext.c guixpm.c htoa.c infiles.c ismethd.c \
	keyword.c lex.c lextype.c lineinfo.c list.c localtime.c lsort.c \
	message.c objtoc.c optiondb.c \
	panelib.c param.c pattern.c pattypes.c process.c radixof.c read.c \
	re_lex.c rt_argblk.c rt_args.c \
	rt_call.c rtclslib.c rt_cvar.c rt_error.c rt_expr.c rtinfo.c \
	rt_methd.c rtnewobj.c rtnwmthd.c rt_obj.c rtobjref.c rtobjvar.c \
	rt_op.c rt_prton.c rt_rcvr.c rt_refobj.c rt_return.c rt_signal.c \
	rt_stdarg.c rt_time.c rttmpobj.c rttrace.c rt_vmthd.c rtxalloc.c \
	sconvchk.c sformat.c \
	shmem.c signame.c spawn_bin.c ssearch.c statfile.c strcatx.c \
	strdupx.c streq.c strsecure.c substrcpy.c tag.c \
	tempio.c tempname.c termsize.c textcx.c trimstr.c typecast.c \
	unixsock.c val.c \
	wregex.c x11ksym.c x11lib.c xcircle.c xcopypixmap.c xftlib.c \
	xgeometry.c xlibfont.c xrender.c xresource.c


libctalk_la_LTLIBOBJS=ansitime.lo become.lo bintodec.lo bitmap.lo \
	bnamecmp.lo cclasses.lo \
	chash.lo clibtmpl.lo ctatoll.lo ctdtoa.lo ctitoa.lo ctlltoa.lo \
	ctltoa.lo ctoobj.lo ctsleep.lo cttmpl.lo dblib.lo edittext.lo \
	errorloc.lo error_out.lo escstr.lo event.lo except.lo exec_bin.lo \
	fileglob.lo fmtargtype.lo font.lo fsecure.lo ftlib.lo glewlib.lo \
	glutlib.lo glxlib.lo guiclearrectangle.lo guiclearwindow.lo \
	guidrawline.lo guidrawpoint.lo guidrawrectangle.lo \
	guiputstr.lo guiputxstr.lo guirefresh.lo guisetbackground.lo \
	guisetforeground.lo guitext.lo guixpm.lo htoa.lo infiles.lo \
	ismethd.lo keyword.lo lex.lo lextype.lo lineinfo.lo \
	list.lo localtime.lo lsort.lo message.lo objtoc.lo optiondb.lo \
	panelib.lo \
	param.lo pattern.lo pattypes.lo process.lo radixof.lo read.lo \
	re_lex.lo rt_argblk.lo \
	rt_args.lo rt_call.lo rtclslib.lo rt_cvar.lo rt_error.lo rt_expr.lo \
	rtinfo.lo rt_methd.lo rtnewobj.lo rtnwmthd.lo rt_obj.lo rtobjref.lo \
	rtobjvar.lo rt_op.lo rt_prton.lo rt_rcvr.lo rt_refobj.lo rt_return.lo \
	rt_signal.lo rt_stdarg.lo rt_time.lo rttmpobj.lo rttrace.lo \
	rt_vmthd.lo rtxalloc.lo sconvchk.lo sformat.lo shmem.lo signame.lo \
	spawn_bin.lo ssearch.lo statfile.lo strcatx.lo strdupx.lo streq.lo \
	strsecure.lo substrcpy.lo tempio.lo tag.lo tempname.lo termsize.lo \
	textcx.lo trimstr.lo typecast.lo unixsock.lo val.lo wregex.lo \
	x11ksym.lo \
	x11lib.lo xcircle.lo xcopypixmap.lo xftlib.lo xgeometry.lo \
	xlibfont.lo xrender.lo xresource.lo

lib_LTLIBRARIES=libctalk.la

libctalk_la_LDFLAGS=-version-info $(LIB_MAJOR_VERSION):$(LIB_MINOR_VERSION) $(LIB_AM_LDFLAGS)

USE_PTHREADS=@use_pthreads@

AM_CFLAGS=$(LIB_AM_CFLAGS)
AM_CPPFLAGS=-I$(top_builddir)/include
DEFS=-DPKGNAME=\"$(PACKAGE)\" -DINSTALLPREFIX=\"$(prefix)\" -DCLASSLIBDIR=\"$(pkgincludedir)\" -DVERSION=\"$(VERSION)\" -DDOCDIR=\"$(pkgdatadir)\" -DLIB_BUILD $(X_LIBS) $(USE_PTHREADS) @have_gl_h_def@ @have_glx_h_def@ @have_glew_h_def@ @no_restrict_warning@ @no_stringop_overflow_warning@ @stringop_truncation_warning@ @address_of_packed_member_warning@ @no_unknown_warning_warning@


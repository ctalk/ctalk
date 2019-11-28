# $Id: Makefile.am,v 1.8 2019/11/28 17:32:52 rkiesling Exp $

SUBDIRS=classes doc libctpp ctpp lib src 
HOST_OS=@host_os@
HOST_CPU=@host_cpu@
ACLOCAL_AMFLAGS=-I m4
BINDIR=@bindir@
SU=@su_path@

noinst_HEADERS=$(top_builddir)/include/*.h

EXTRA_DIST=$(top_builddir)/build/gccincdir.sh \
	$(top_builddir)/test/* \
	$(top_builddir)/test/expect/* $(top_builddir)/demos/* \
	$(top_builddir)/src/ctdb.in $(top_builddir)/src/ctcc.in \
	$(top_builddir)/Makefile.djgpp \
	$(top_builddir)/config.h.in \
	$(top_builddir)/build/caps.sh \
	$(top_builddir)/build/osxbuiltins.sh \
	$(top_builddir)/build/include_dirs.sh \
	$(top_builddir)/classes/Makefile.djgpp \
	$(top_builddir)/doc/Makefile.djgpp \
	$(top_builddir)/build/builtins.sh \
	$(top_builddir)/build/make_builtins.c \
	$(top_builddir)/build/lcflags.sh \
	$(top_builddir)/build/preload.ca \
	$(top_builddir)/build/preload.sh \
	$(top_builddir)/build/gccmajorver.sh \
	$(top_builddir)/build/check_path.sh \
	$(top_builddir)/build/set_cache_perms.sh \
	$(top_builddir)/build/inst_group.sh \
	$(top_builddir)/build/pthread.sh \
	$(top_builddir)/build/gcccaps.sh \
	$(top_builddir)/build/chcacheperms.sh \
	$(top_builddir)/build/conftest.c \
	$(top_builddir)/build/glh.sh \
	$(top_builddir)/build/have_llvm.sh \
	$(top_builddir)/build/fn_decl.pl \
	$(top_builddir)/m4/* \
	$(top_builddir)/ChangeLog \
	$(top_builddir)/tools/langtools \
	$(top_builddir)/tools/methods/methods.ca \
	$(top_builddir)/tools/methods/Makefile \
	$(top_builddir)/tools/ctxlogo/ctxlogo.ca \
	$(top_builddir)/tools/ctxlogo/Makefile \
	$(top_builddir)/tools/ctxlogo/ctalk-logo.xpm \
	$(top_builddir)/tools/classes/Makefile \
	$(top_builddir)/tools/classes/classes.ca \
	$(top_builddir)/tools/methodsource/methodsource.ca \
	$(top_builddir)/tools/methodsource/Makefile \
	$(top_builddir)/tools/classdoc/classdoc.ca \
	$(top_builddir)/tools/classdoc/Makefile \
	$(top_builddir)/tools/membervars/membervars.ca \
	$(top_builddir)/tools/membervars/Makefile \
	$(top_builddir)/tools/template/template.ca \
	$(top_builddir)/tools/template/Makefile \
	$(top_builddir)/tools/searchlib/searchlib.ca \
	$(top_builddir)/tools/searchlib/Makefile \
	$(top_builddir)/doc/libdoc \
	$(top_builddir)/doc/classlibdoc \
	$(top_builddir)/doc/doctools/*


install-data-hook:
	@$(SHELL) $(top_builddir)/build/preload.sh ; \
	$(SHELL) -c "cd $(top_builddir)/build && ./set_cache_perms.sh" ; \
	$(SHELL) -c "echo " ; \
	if [ -f build/.nolangtools ]; then \
	  $(SHELL) -c "echo " ; \
	  $(SHELL) -c "echo Not building language tools. " ; \
          $(SHELL) -c "echo " ; \
	else \
	  $(SHELL) -c "echo Building language tools." ; \
	  $(SHELL) -c "echo " ; \
	  for t in `cat $(top_builddir)/tools/langtools`; do \
	    $(SHELL) -c "export CTBINDIR=$(BINDIR) && cd $(top_builddir)/tools/$$t && make install"; \
          done; \
	fi ; \
	$(SHELL) $(top_builddir)/build/chcacheperms.sh

distclean-local:
	rm -f $(top_builddir)/build/.nopreload \
	  $(top_builddir)/build/.nolangtools \
	  $(top_builddir)/build/.confname $(top_builddir)/build/.includedir \
	  $(top_builddir)/build/.confgroup \
	  $(top_builddir)/build/.confuserhomedir \
	  $(top_builddir)/build/.gcchavelongdouble \
	  $(top_builddir)/build/.gccnolongdouble \
	  $(top_builddir)/build/.no_x_support \
	  $(top_builddir)/build/.gccnoframeworkopt \
	  $(top_builddir)/build/.gcchaveframeworkopt \
	  $(top_builddir)/build/.pthread_gcc_option \
          $(top_builddir)/build/.gccdonthavenoformatoverflowopt \
          $(top_builddir)/build/.gcchavenoformatoverflowopt \
	  $(top_builddir)/build/.no_gl_h \
	  $(top_builddir)/build/.no_glx_h \
          $(top_builddir)/build/.no_glew_h \
          $(top_builddir)/build/.gl_h_path \
	  $(top_builddir)/build/.glx_h_path \
	  $(top_builddir)/build/.glew_h_path \
	  $(top_builddir)/build/.llvm \
	  $(top_builddir)/build/preload.i \
	  $(top_builddir)/build/.x_h_path \
	  $(top_builddir)/build/make_builtins \
	  $(top_builddir)/build/builtin_names \
	  $(top_builddir)/build/conftest ; \
	  for t in `cat $(top_builddir)/tools/langtools`; do \
	    $(SHELL) -c "rm -f $(top_builddir)/tools/$$t/$$t $(top_builddir)/tools/$$t/$$t.i $(top_builddir)/tools/$$t/gmon.out $(top_builddir)/tools/$$t/*.err "; \
          done; 

check:
	if [ ! -f $(top_builddir)/test/Makefile ] ; then \
	$(ECHO) -----------------------------------------------------------------------------; \
	$(ECHO) The Ctalk tests are normally in the test subdirectory. ; \
	$(ECHO) They are part of the complete Ctalk distribution.; \
	$(ECHO) The file test/README contains further information. ; \
	$(ECHO) -----------------------------------------------------------------------------; \
	else \
	 cd $(top_builddir)/test && make ; \
	fi

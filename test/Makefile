# $Id: Makefile,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $

.SILENT:
CTALK=`which ctalk`
CLASSES=../classes
LD_LIBRARY_OPT=`if [ ${LD_LIBRARY_PATH} ]; then echo ${LD_LIBRARY_PATH} | sed 's/:$$//' | sed 's/^/-L/' | sed 's/:/ -L/g'; fi`

#
# You might need to edit this if you set ./configure's options different
# than the defaults.
#
LDFLAGS=-lreadline

CC=gcc

# You might need to turn off these warnings with old compilers.... 
CFLAGS=-g -O2 -Wno-uninitialized -Wno-unused

BIN_EXT=`if [ $(OSTYPE) = 'cygwin'] ; then printf ".exe"; else printf ""; fi;`

BINARIES=ctypes ctypes-sunos basecls ctrlstrc basiclist assocarray externs
CYGWIN_BINARIES=ctypes.exe basecls.exe ctrlstrc.exe basiclist.exe externs.exe


test: cvariables baseclasses ctrlstructs basiclist assocarray externs

all: test

#
#  Use -Wall only when rewriting the test modules.  The use of
#  -Wall should only generate, "<var> defined but not used," 
#  warnings.
#
cvariables:
	echo "============================="; \
	echo "C variables and expressions."; \
	echo "============================="; \
	case `uname -a` in \
	Linux*) \
		$(CTALK) -I$(CLASSES) ctypes.c -o ctypes.i  && \
		$(CC) $(CFLAGS) ctypes.i -o ctypes \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		-lctalk && ./ctypes && \
		rm -f ctypes.i ctypes \
	;; \
	SunOS*) \
		$(CTALK) -I$(CLASSES) ctypes-sunos.c -o ctypes-sunos.i  && \
		$(CC) $(CFLAGS) ctypes-sunos.i -o ctypes-sunos \
		$(LD_LIBRARY_OPT) $(LDFLAGS) -lctalk && ./ctypes-sunos && \
		rm -f ctypes-sunos.i ctypes-sunos \
	;; \
	CYGWIN*) \
		$(CTALK) -I$(CLASSES) ctypes-cygwin.c -o ctypes-cygwin.i  && \
		$(CC) $(CFLAGS) ctypes-cygwin.i -o ctypes-cygwin$(_EXT) \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		-lctalk && ./ctypes-cygwin$(_EXT) && \
		rm -f ctypes-cygwin.i ctypes-cygwin$(_EXT) \
	;; \
	Darwin*) \
		$(CTALK) -I$(CLASSES) ctypes-macosx.c -o ctypes-macosx.i  && \
		$(CC) ctypes-macosx.i -o ctypes-macosx \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		-lctalk && ./ctypes-macosx$ && \
		rm -f ctypes-macosx.i ctypes-macosx \
	;; \
	*) \
	echo "***Not supported on this OS.***"; \
	;; \
	esac


baseclasses:
	case `uname -a` in \
	Darwin*) \
		echo "============================="; \
		echo "Base classes."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) basecls.c -o basecls.i && \
		$(CC) basecls.i -o basecls -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./basecls && \
		rm -f basecls.i basecls$(_EXT) \
	;; \
	*) \
		echo "============================="; \
		echo "Base classes."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) basecls.c -o basecls.i && \
		$(CC) $(CFLAGS) -Wall basecls.i -o basecls -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./basecls && \
		rm -f basecls.i basecls$(_EXT) \
	;; \
	esac

ctrlstructs:
	case `uname -a` in \
	CYGWIN*) \
		echo "============================="; \
		echo "Control structures."; \
		echo "(Expect a warning message.)" ; \
		echo "============================="; \
		$(CTALK) ctrlstrc.c -o ctrlstrc.i && \
		$(CC) $(CFLAGS) -Wall ctrlstrc.i -o ctrlstrc -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./ctrlstrc$(_EXT) \
		rm -f ctrlstrc.i ctrlstrc$(_EXT) \
		;; \
	Darwin*) \
		echo "============================="; \
		echo "Control structures."; \
		echo "============================="; \
		$(CTALK) ctrlstrc.c -o ctrlstrc.i && \
		$(CC) ctrlstrc.i -o ctrlstrc -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./ctrlstrc$(_EXT) \
		rm -f ctrlstrc.i ctrlstrc$(_EXT) \
		;; \
	*) \
		echo "============================="; \
		echo "Control structures."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) ctrlstrc.c -o ctrlstrc.i && \
		$(CC) $(CFLAGS) -Wall ctrlstrc.i -o ctrlstrc -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./ctrlstrc && \
		rm -f ctrlstrc.i ctrlstrc$(_EXT) \
		;; \
	esac

basiclist:
	case `uname -a` in \
	Darwin*) \
		echo "============================="; \
		echo "Basic lists."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) basiclist.c -o basiclist.i && \
		$(CC) basiclist.i -o basiclist -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./basiclist && \
		rm -f basiclist.i basiclist$(_EXT) \
	;; \
	*) \
		echo "============================="; \
		echo "Basic lists."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) basiclist.c -o basiclist.i && \
		$(CC) $(CFLAGS) -Wall basiclist.i -o basiclist -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./basiclist && \
		rm -f basiclist.i basiclist$(_EXT) \
	;; \
	esac

assocarray:
	case `uname -a` in \
	Darwin*) \
		echo "============================="; \
		echo "AssociativeArray."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) assocarray.c -o assocarray.i && \
		$(CC) assocarray.i -o assocarray -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./assocarray && \
		rm -f assocarray.i assocarray$(_EXT) \
	;; \
	*) \
		echo "============================="; \
		echo "AssociativeArray."; \
		echo "============================="; \
		$(CTALK) -I $(CLASSES) assocarray.c -o assocarray.i && \
		$(CC) $(CFLAGS) -Wall assocarray.i -o assocarray -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./assocarray && \
		rm -f assocarray.i assocarray$(_EXT) \
	;; \
	esac

externs:
	case `uname -a` in \
	Darwin*) \
		echo "============================="; \
		echo "Externs."; \
		echo "============================="; \
		$(CTALK) -I . -I $(CLASSES) externs.c testfunc.c testmethod.c -o externs.i && \
		$(CC) externs.i -o externs -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./externs && \
		rm -f externs.i externs$(_EXT) \
	;; \
	*) \
		echo "============================="; \
		echo "Externs."; \
		echo "============================="; \
		$(CTALK) -I . -I $(CLASSES) externs.c testfunc.c testmethod.c -o externs.i && \
		$(CC) $(CFLAGS) -Wall externs.i -o externs -lctalk \
		$(LD_LIBRARY_OPT) $(LDFLAGS) \
		&& ./externs && \
		rm -f externs.i externs$(_EXT) \
	;; \
	esac

clean:  
	rm -f *.i *~ *.tmp ; \
	case `uname -a` in \
	CYGWIN*) \
		rm -f $(CYGWIN_BINARIES);  \
	;; \
	*) \
		rm -f $(BINARIES) ; \
	;; \
	esac


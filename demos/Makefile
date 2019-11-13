# $Id: Makefile,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $

BINARIES=hello chaser ctecho ctpath ctwc ctrep mdt mdt2 lclock sls

CTALK=`which ctalk`
CLASSES=../classes

CC=gcc
CCFLAGS=-O2

LD_LIBRARY_OPT=`if [ ${LD_LIBRARY_PATH} ]; then echo ${LD_LIBRARY_PATH} | sed 's/:$$//' | sed 's/^/-L/' | sed 's/:/ -L/g'; fi`

LIBS=-lctalk

##
## Comment out this line if the system doesn't have the GNU 
## readline libraries installed.
##
EXTRALIBS=-lreadline -lhistory
EXTRALIBS_DARWIN=-lreadline
XLIBS=-lX11 -lXext

programs: $(BINARIES)

hello: hello.c
	@echo hello.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) hello.c -o hello.i && \
		$(CC) $(CCFLAGS) hello.i -o hello $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f hello.i; \
	else \
		$(CTALK) hello.c -o hello.i && \
		$(CC) $(CCFLAGS) hello.i -o hello $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f hello.i; \
	fi

chaser: chaser.c
	@echo chaser.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) chaser.c -o chaser.i && \
		$(CC) $(CCFLAGS) chaser.i -o chaser $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f chaser.i; \
	else \
		$(CTALK) chaser.c -o chaser.i && \
		$(CC) $(CCFLAGS) chaser.i -o chaser $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f chaser.i; \
	fi

ctecho: ctecho.c
	@echo ctecho.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) ctecho.c -o ctecho.i && \
		$(CC) $(CCFLAGS) ctecho.i -o ctecho $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f ctecho.i; \
	else \
		$(CTALK) ctecho.c -o ctecho.i && \
		$(CC) $(CCFLAGS) ctecho.i -o ctecho $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f ctecho.i; \
	fi

ctpath: ctpath.c
	@echo ctpath.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) ctpath.c -o ctpath.i && \
		$(CC) $(CCFLAGS) ctpath.i -o ctpath $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f ctpath.i; \
	else \
		$(CTALK) ctpath.c -o ctpath.i && \
		$(CC) $(CCFLAGS) ctpath.i -o ctpath $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f ctpath.i; \
	fi

ctwc: ctwc.c
	@echo ctwc.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) ctwc.c -o ctwc.i && \
		$(CC) $(CCFLAGS) ctwc.i -o ctwc $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f ctwc.i; \
	else \
		$(CTALK) ctwc.c -o ctwc.i && \
		$(CC) $(CCFLAGS) ctwc.i -o ctwc $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f ctwc.i; \
	fi

ctrep: ctrep.c
	@echo ctrep.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) ctrep.c -o ctrep.i && \
		$(CC) $(CCFLAGS) ctrep.i -o ctrep $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f ctrep.i; \
	else \
		$(CTALK) ctrep.c -o ctrep.i && \
		$(CC) $(CCFLAGS) ctrep.i -o ctrep $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f ctrep.i; \
	fi

mdt: mdt.c
	@echo mdt.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) mdt.c -o mdt.i && \
		$(CC) $(CCFLAGS) mdt.i -o mdt $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f mdt.i; \
	else \
		$(CTALK) mdt.c -o mdt.i && \
		$(CC) $(CCFLAGS) mdt.i -o mdt $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f mdt.i; \
	fi

mdt2: mdt2.c
	@echo mdt2.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) mdt2.c -o mdt2.i && \
		$(CC) $(CCFLAGS) mdt2.i -o mdt2 $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f mdt2.i; \
	else \
		$(CTALK) mdt2.c -o mdt2.i && \
		$(CC) $(CCFLAGS) mdt2.i -o mdt2 $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f mdt2.i; \
	fi

lclock: lclock.c
	@echo lclock.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) lclock.c -o lclock.i && \
		$(CC) $(CCFLAGS) lclock.i -o lclock $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f lclock.i; \
	else \
		$(CTALK) lclock.c -o lclock.i && \
		$(CC) $(CCFLAGS) lclock.i -o lclock $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f lclock.i; \
	fi

sls: sls.c
	@echo sls.c; \
	if [ `uname -s` = 'Darwin' ]; then \
		$(CTALK) sls.c -o sls.i && \
		$(CC) $(CCFLAGS) sls.i -o sls $(LIBS) $(EXTRALIBS_DARWIN) $(LD_LIBRARY_OPT) && \
		rm -f sls.i; \
	else \
		$(CTALK) sls.c -o sls.i && \
		$(CC) $(CCFLAGS) sls.i -o sls $(LIBS) $(EXTRALIBS) $(LD_LIBRARY_OPT) && \
		rm -f sls.i; \
	fi

clean:  
	rm -f *.i *~ *.tmp $(BINARIES)



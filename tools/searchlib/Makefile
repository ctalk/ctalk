# Makefile for searchlib.  This is the quick version.

SRC=searchlib.ca
BIN=searchlib

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh searchlib $$CTBINDIR"


searchlib: searchlib.ca
	@echo 'ctcc searchlib.ca -o searchlib >searchlib.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc searchlib.ca -o searchlib >searchlib.err 2>&1 ;\
	else \
	  ctcc searchlib.ca -o searchlib >searchlib.err 2>&1 ;\
	fi 

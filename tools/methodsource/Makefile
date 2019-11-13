# Makefile for methodsource.  Another quick version.

SRC=methodsource.ca
BIN=methodsource

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh methodsource $$CTBINDIR"


methodsource: methodsource.ca
	@echo 'ctcc methodsource.ca -o methodsource >methodsource.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc methodsource.ca -o methodsource >methodsource.err 2>&1 ;\
	else \
	  ctcc methodsource.ca -o methodsource >methodsource.err 2>&1 ;\
	fi 

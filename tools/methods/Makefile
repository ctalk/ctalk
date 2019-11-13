# Makefile for methods.  This is the quick version.

SRC=methods.ca
BIN=methods

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh methods $$CTBINDIR"


methods: methods.ca
	@echo 'ctcc methods.ca -o methods >methods.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc methods.ca -o methods >methods.err 2>&1 ;\
	else \
	  ctcc methods.ca -o methods >methods.err 2>&1 ;\
	fi 

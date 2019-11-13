# Makefile for classdoc.

SRC=classdoc.ca
BIN=classdoc

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh classdoc $$CTBINDIR"


classdoc: classdoc.ca
	@echo 'ctcc classdoc.ca -o classdoc >classdoc.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc classdoc.ca -o classdoc >classdoc.err 2>&1 ;\
	else \
	  ctcc classdoc.ca -o classdoc >classdoc.err 2>&1 ;\
	fi 

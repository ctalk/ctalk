# Makefile for membervars.  This is the quick version.

SRC=membervars.ca
BIN=membervars

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh membervars $$CTBINDIR"


membervars: membervars.ca
	@echo 'ctcc membervars.ca -o membervars >membervars.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc membervars.ca -o membervars >membervars.err 2>&1 ;\
	else \
	  ctcc membervars.ca -o membervars >membervars.err 2>&1 ;\
	fi 

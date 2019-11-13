# Makefile for classes.  Again, this is the quick version.

SRC=classes.ca
BIN=classes

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh classes $$CTBINDIR"


classes: classes.ca
	@echo 'ctcc classes.ca -o classes >classes.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc classes.ca -o classes >classes.err 2>&1 ;\
	else \
	  ctcc classes.ca -o classes >classes.err 2>&1 ;\
	fi 

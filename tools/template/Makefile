# Makefile for template.  This is the quick version.

SRC=template.ca
BIN=template

all: $(BIN)

install: $(BIN)
	$(SHELL) -c "../../install-sh template $$CTBINDIR"


template: template.ca
	@echo 'ctcc template.ca -o template >template.err 2>&1' ;\
	if [ -f ../../build/.x_h_path ]; then \
	  xinc="-I "`cat ../../build/.x_h_path` ;\
	  ctcc $$xinc template.ca -o template >template.err 2>&1 ;\
	else \
	  ctcc template.ca -o template >template.err 2>&1 ;\
	fi 

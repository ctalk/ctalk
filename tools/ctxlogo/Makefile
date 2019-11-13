# Makefile for ctxlogo.  This is the quick version.

SRC=ctxlogo.ca
BIN=ctxlogo


all: $(BIN)

install: ctxlogo.ca
	if [ ! -f ../../build/.no_x_support ]; then \
	  if [ ! -f $(BIN) ]; then \
	    ctcc -x ctxlogo.ca -o ctxlogo > ctxlogo.err 2>&1 ; \
	  fi; \
	  $(SHELL) -c "../../install-sh ctxlogo $$CTBINDIR"; \
	fi
#
# Doesn't need ../../build/.x_h_path because we want to use
# ctcc's -x option as usual.
#
$(BIN): ctxlogo.ca
	@if [ ! -f ../../build/.no_x_support ]; then \
	    ctcc -x ctxlogo.ca -o ctxlogo > ctxlogo.err 2>&1 ; \
	fi

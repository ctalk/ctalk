
#
#  Makefile for the validatedentry program.
#

all: validatedentry

validatedentry: X11ValidatedTextEntryBox validatedentry.ca
	ctdb -k -x -I . validatedentry.ca -o validatedentry

clean:
	rm -f validatedentry validatedentry.i validatedentry.o *.~ core


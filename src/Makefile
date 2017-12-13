#########################################################################
CC= gcc
COMCFLAGS= $(INCLUDES) -DDEBUG $(ZLIB_OPT) \
			-DGNULINUXDEFS -DYACC_ALLOWED
CFLAGS=-c -g -Og $(COMCFLAGS)
EDGCFLAGS= -O
LDFLAGS=
BIN=$(HOME)/bin
LEX=flex

# We need our own patched yacc
YACC=$(ROOTDIR)/byacc-1.9/yacc

ROOTDIR=..

#########################################################################


include Makefile.common
include Makefile.dependencies


#########################################################################
#
# Trigger running YACC on possibly updated grammar files

yacc: $(YACC)
	$(MAKE) YACCING=yes

$(YACC):
	$(MAKE) -C $(ROOTDIR)/byacc-1.9

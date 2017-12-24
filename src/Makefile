#########################################################################
CC= gcc
COMCFLAGS= $(INCLUDES) -DDEBUG $(ZLIB_OPT) -DGNULINUXDEFS
CFLAGS=-c -g -Og $(COMCFLAGS)
LDFLAGS=
LEX=flex

ROOTDIR=..

# We need our own patched yacc
YACC=$(ROOTDIR)/byacc-1.9/yacc

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

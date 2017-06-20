#########################################################################
CC= gcc
COMCFLAGS= $(INCLUDES) -DDEBUG $(ZLIB_OPT) \
			-DBIN_RELEASE -DPRESERVE_C_ARGS -D__mygnulinux__ -DYACC_ALLOWED
CFLAGS= -c -O $(COMCFLAGS)
EDGCFLAGS= -O
LDFLAGS=
BIN=$(HOME)/bin
LEX=flex
YACC=$(ROOTDIR)/byacc-1.9/yacc
RANLIB=echo
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

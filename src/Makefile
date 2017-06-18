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

$(YACC):
	cd $(ROOTDIR)/byacc-1.9; make

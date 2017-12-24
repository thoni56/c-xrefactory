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
#
# If our yacc is built we can rebuild from yacc source always

ifneq ("$(wildcard $(YACC))", "")
YACCING=yes
endif

#########################################################################
#
# The heavy lifting is here

include Makefile.common
include Makefile.dependencies

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist and run it on
# possibly updated grammar files

yacc: $(YACC)
	$(MAKE) YACCING=yes

$(YACC):
	$(MAKE) -C $(ROOTDIR)/byacc-1.9

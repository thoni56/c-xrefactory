#########################################################################
CC = gcc

WARNINGS = -Wall -Wno-char-subscripts

# CFLAGS for bootstrap compilation
BS_CFLAGS = -g $(INCLUDES) -DDEBUG $(ZLIB_OPT) -DGNULINUXDEFS -MMD

# Real CFLAGS
CFLAGS = $(BS_CFLAGS) -funsigned-char $(WARNINGS) $(COVERAGE)

LDFLAGS = $(COVERAGE)

LEX = flex

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

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist and run it on
# possibly updated grammar files

yacc: $(YACC)
	$(MAKE) YACCING=yes

$(YACC):
	$(MAKE) -C $(ROOTDIR)/byacc-1.9

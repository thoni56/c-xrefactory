# Sources and link settings common to bootstrap (Makefile.bs) and the
# real target build (Makefile.common)

SRCS = cgram.c main.c globals.c misc.c semact.c commons.c generate.c \
	   enumTxt.c complete.c cxref.c cxfile.c lex.c yylex.c cexp.c options.c \
	   caching.c javagram.c jsemact.c cfread.c cct.c init.c \
	   editor.c refactory.c protocol.c log.c \
	   yaccgram.c html.c extract.c classh.c jslsemact.c \
	   filetab.c matab.c olcxtab.c editorbuffertab.c symtab.c javafqttab.c \
	   jsltypetab.c reftab.c memmac.c utils.c

OBJDIR = .objects

# Put the following into comments if you wish to build without zlib
# library, i.e. without possibility to read compressed .jar files
ZLIB_OPT=-DUSE_LIBZ
ZLIB_LIB= -lz

# If you are using your systems zlib use these
INCLUDES=$(ZLIB_INCLUDE)
LIBS=$(ZLIB_LIB)
# Else build the local, included, version and use these
# ZLIB_LIB=$(ROOTDIR)/lib/zlib/libz.a
# ZLIB_INCLUDE=-I$(ROOTDIR)/lib/zlib

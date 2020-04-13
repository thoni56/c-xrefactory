# Sources and link settings common to bootstrap (Makefile.bs) and the
# real target build (Makefile.common)

SRCS = main.c globals.c misc.c semact.c commons.c generate.c \
	   enumTxt.c complete.c cxref.c cxfile.c lex.c yylex.c options.c \
	   caching.c jsemact.c classfilereader.c cct.c init.c \
	   editor.c refactory.c protocol.c log.c \
	   html.c extract.c classhierarchy.c jslsemact.c \
	   filetab.c macroargumenttable.c olcxtab.c editorbuffertab.c symboltable.c javafqttab.c \
	   jsltypetab.c reftab.c memory.c utils.c characterbuffer.c hash.c symbol.c \
	   c_parser.tab.c cexp_parser.tab.c java_parser.tab.c yacc_parser.tab.c \
	   fileitem.c filedescriptor.c typemodifier.c position.c id.c

OBJDIR = .objects
OBJS = $(addprefix $(OBJDIR)/,${SRCS:.c=.o})

-include $(OBJDIR)/*.d

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(OUTPUT_OPTION) -MMD -c $<

$(OBJDIR):
	@mkdir $(OBJDIR)

$(ZLIB_LIB):
	make -C $(ROOTDIR)/lib/zlib libz.a

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

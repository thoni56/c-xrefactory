MODULES =  main globals misc semact commons \
		complete cxref cxfile lexer lexembuffer yylex options \
		caching jsemact classfilereader classcaster init \
		editor refactory protocol log type usage storage \
		extract classhierarchy jslsemact \
		filetable macroargumenttable session editorbuffertab symboltable \
		javafqttab jsltypetab reftab memory utils characterreader hash \
		symbol c_parser.tab cexp_parser.tab java_parser.tab yacc_parser.tab \
		filedescriptor typemodifier position id parsers fileio stringlist ppc \
		server

modules:
	@echo MODULES=$(MODULES)
	@echo SRCS=$(SRCS)
	@echo OBJS=$(OBJS)

OBJDIR = .objects
SRCS = ${MODULES:=.c}
OBJS = $(addprefix $(OBJDIR)/,${SRCS:.c=.o})
DEPS = $(addprefix $(OBJDIR)/,${SRCS:.c=.d})

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(OUTPUT_OPTION) -MMD -MP -c $<

$(OBJDIR):
	-mkdir $(OBJDIR)

-include $(OBJDIR)/*.d

$(OPTIONAL_ZLIB_LIB):
	make -C $(ROOTDIR)/lib/zlib libz.a

# If your system has zlib use this:

LIBS += -lz
CFLAGS += -DHAVE_ZLIB

# else you can either

# 0) install zlib dev package using your pacakge manager, like:
#
# $ sudo apt install zlib1g-dev

# 1) build a local version of zlib
# see http://zlib.net and https://github.com/madler/zlib and
# set:
# OPTIONAL_ZLIB_LIB=<path-to-zlib>
# INCLUDES += -I<path-to-zlib>

# 2) compile c-xrefactory without support for compressed jars by
# commenting out the above LIBS and CFLAGS definitions

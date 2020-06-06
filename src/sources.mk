MODULES =  main globals misc semact commons generate \
		enumTxt complete cxref cxfile lexer yylex options \
		caching jsemact classfilereader classcaster init \
		editor refactory protocol log type \
		html extract classhierarchy jslsemact \
		filetab macroargumenttable olcxtab editorbuffertab symboltable \
		javafqttab jsltypetab reftab memory utils characterreader hash \
		symbol c_parser.tab cexp_parser.tab java_parser.tab yacc_parser.tab \
		fileitem filedescriptor typemodifier position id parsers fileio

modules:
	@echo $(MODULES)
	@echo $(SRCS)
	@echo $(OBJS)

OBJDIR = .objects
SRCS = ${MODULES:=.c}
OBJS = $(addprefix $(OBJDIR)/,${SRCS:.c=.o})
DEPS = $(addprefix $(OBJDIR)/,${SRCS:.c=.d})

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(OUTPUT_OPTION) -MMD -MP -c $<

$(OBJDIR):
	mkdir $(OBJDIR)

-include $(OBJDIR)/*.d

$(OPTIONAL_ZLIB_LIB):
	make -C $(ROOTDIR)/lib/zlib libz.a

# If you are using your systems zlib use this
LIBS+=-lz
# Else build the local, included, version and use these
# OPTIONAL_ZLIB_LIB=$(ROOTDIR)/lib/zlib/libz.a
# INCLUDES += -I$(ROOTDIR)/lib/zlib

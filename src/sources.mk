MODULES = c_parser.tab cppexp_parser.tab characterreader commandlogger commons complete	\
completion counters cxfile cxref editor editorbuffer editorbuffertable editormarker	\
encoding extract filedescriptor fileio filetable globals hash id init lexemstream	\
json_utils lexem lexembuffer lexer log lsp lsp_adapter lsp_dispatcher lsp_handler	\
lsp_utils lsp_sender macroargumenttable main memory browsermenu misc options parsers	\
position ppc progress protocol refactorings refactory reference referencesstack reftab	\
semact server session stackmemory storage stringlist symbol symboltable type		\
typemodifier undo usage xref yacc_parser.tab yylex

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

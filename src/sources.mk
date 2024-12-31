MODULES = main globals misc semact commons complete cxref cxfile	\
		lexer lexembuffer yylex options caching init editor	\
		refactory protocol log type usage storage extract	\
		filetable macroargumenttable session			\
		editorbuffertable symboltable reftab memory		\
		characterreader hash symbol c_parser.tab		\
		cexp_parser.tab yacc_parser.tab filedescriptor		\
		typemodifier position id parsers fileio stringlist	\
		ppc server reference xref refactorings progress input	\
		commandlogger completion menu lexem editorbuffer undo	\
		counters stackmemory encoding json_utils                \
		lsp lsp_dispatcher lsp_handler lsp_sender

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

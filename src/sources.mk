MODULES = browsermenu c_parser.tab cppexp_parser.tab characterreader	\
commandlogger commons complete counters cxfile cxref editor				\
editorbuffer editorbuffertable editormarker encoding extract			\
filedescriptor fileio filetable globals hash id init lexemstream		\
json_utils lexem lexembuffer lexer log lsp lsp_adapter lsp_dispatcher	\
lsp_handler lsp_utils lsp_sender macroargumenttable main match memory	\
misc options move_function organize_includes parsing parsers position	\
ppc progress protocol refactorings refactory reference					\
referenceableitem referenceableitemtable reference_database search		\
semact server session stackmemory startup storage stringlist symbol		\
symboltable type typemodifier undo usage xref yacc_parser.tab yylex

modules:
	@echo MODULES=$(MODULES)
	@echo SRCS=$(SRCS)
	@echo OBJS=$(OBJS)

OBJDIR = .objects

CJSON_SRC := $(CJSON_DIR)/cJSON.c
CJSON_OBJ := $(OBJDIR)/cJSON.o
CJSON_DEP := $(OBJDIR)/cJSON.d

$(CJSON_OBJ): $(CJSON_SRC) | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

SRCS = ${MODULES:=.c}
OBJS = $(addprefix $(OBJDIR)/,${SRCS:.c=.o}) $(CJSON_OBJ)
DEPS = $(addprefix $(OBJDIR)/,${SRCS:.c=.d}) $(CJSON_DEP)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR):
	-mkdir $(OBJDIR)

-include $(DEPS)

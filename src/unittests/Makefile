vpath %.c ..

CFLAGS = -g -Og -MMD -Wall -Wno-char-subscripts -fPIC -I.. $(COVERAGE) -I/usr/local/include

OBJDIR=.objs

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

CGREEN = -L/usr/local/lib -lcgreen

debug: COVERAGE=--coverage
all debug: $(UNITTESTS)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^

clean:
	-rm $(OBJDIR)/*.[od] *.so $(OBJDIR)/*.gc*

cleaner: clean
	-rm -rf $(OBJDIR)

# These link the modules .o and only tests the external behaviour
symboltable_tests.so: $(OBJDIR)/symboltable_tests.o $(OBJDIR)/symboltable.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

jslsemact_tests.so: $(OBJDIR)/jslsemact_tests.o $(OBJDIR)/jslsemact.o $(OBJDIR)/symbol.o $(OBJDIR)/log.o $(OBJDIR)/id.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

olcxtab_tests.so: $(OBJDIR)/olcxtab_tests.o $(OBJDIR)/olcxtab.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

filetab_tests.so: $(OBJDIR)/filetab_tests.o $(OBJDIR)/filetab.o $(OBJDIR)/hash.o $(OBJDIR)/fileitem.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

symbol_tests.so: $(OBJDIR)/symbol_tests.o $(OBJDIR)/symbol.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

misc_tests.so: $(OBJDIR)/misc.o $(OBJDIR)/protocol.o $(OBJDIR)/enumTxt.o $(OBJDIR)/misc_tests.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

editor_tests.so: $(OBJDIR)/editor_tests.o $(OBJDIR)/editor.o $(OBJDIR)/position.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

cxref_tests.so: $(OBJDIR)/cxref_tests.o $(OBJDIR)/cxref.o $(OBJDIR)/protocol.o $(OBJDIR)/enumTxt.o $(OBJDIR)/hash.o $(OBJDIR)/position.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

OPTIONS_DEPENDENCIES = protocol log position memory
OPTIONS_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(OPTIONS_DEPENDENCIES))
options_tests.so: $(OBJDIR)/options_tests.o $(OBJDIR)/options.o $(OPTIONS_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

YYLEX_DEPENDENCIES = protocol log position memory filedescriptor id filetab hash fileitem symboltable macroargumenttable symbol
YYLEX_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(YYLEX_DEPENDENCIES))
yylex_tests.so: $(OBJDIR)/yylex_tests.o $(OBJDIR)/yylex.o $(YYLEX_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

# These includes the modules .c to be able to test internal functions or macros
LEX_DEPENDENCIES = log position
LEX_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(LEX_DEPENDENCIES))
lex_tests.so: $(OBJDIR)/lex_tests.o $(LEX_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

CXFILE_DEPENDENCIES = protocol log position
CXFILE_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(CXFILE_DEPENDENCIES))
cxfile_tests.so:$(OBJDIR)/cxfile_tests.o $(CXFILE_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

EXTRACT_DEPENDENCIES = protocol log position memory id
EXTRACT_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(EXTRACT_DEPENDENCIES))
extract_tests.so:$(OBJDIR)/extract_tests.o $(EXTRACT_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)



# A test lib for a module is built from its .o and the _test.o (and some extras)
#%_tests.so: %.o %_tests.o $(EXTRA_OBJS)
#	gcc -shared -o $@ $(sort $(EXTRA_OBJS) $^) $(LDLIBS) -lcgreen

$(OBJDIR)/%_tests.o: %_tests.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/:
	-mkdir $@

-include $(OBJDIR)/*.d

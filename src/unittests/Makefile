vpath %.c ..

CFLAGS = -g -O0 -MMD -Wall -Wno-char-subscripts -fPIC -I.. $(COVERAGE) -I/usr/local/include

OBJDIR=.objects

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

CGREEN = -L/usr/local/lib -lcgreen

debug: COVERAGE=--coverage
all debug: $(UNITTESTS)
	lcov -q -z -d $(OBJDIR)
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

filetable_tests.so: $(OBJDIR)/filetable_tests.o $(OBJDIR)/filetable.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

symbol_tests.so: $(OBJDIR)/symbol_tests.o $(OBJDIR)/symbol.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

semact_tests.so:  $(OBJDIR)/semact_tests.o $(OBJDIR)/semact.o $(OBJDIR)/protocol.o $(OBJDIR)/hash.o $(OBJDIR)/position.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

misc_tests.so:  $(OBJDIR)/misc_tests.o $(OBJDIR)/misc.o $(OBJDIR)/protocol.o $(OBJDIR)/type.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

commons_tests.so: $(OBJDIR)/commons_tests.o $(OBJDIR)/commons.o $(OBJDIR)/protocol.o $(OBJDIR)/log.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

editor_tests.so: $(OBJDIR)/editor_tests.o $(OBJDIR)/editor.o $(OBJDIR)/position.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

cxref_tests.so: $(OBJDIR)/cxref_tests.o $(OBJDIR)/cxref.o $(OBJDIR)/protocol.o $(OBJDIR)/hash.o $(OBJDIR)/position.o $(OBJDIR)/type.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

OPTIONS_DEPENDENCIES = protocol log position
OPTIONS_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(OPTIONS_DEPENDENCIES))
options_tests.so: $(OBJDIR)/options_tests.o $(OBJDIR)/options.o $(OPTIONS_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

YYLEX_DEPENDENCIES = protocol log position filedescriptor id filetable hash symboltable macroargumenttable symbol
YYLEX_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(YYLEX_DEPENDENCIES))
yylex_tests.so: $(OBJDIR)/yylex_tests.o $(OBJDIR)/yylex.o $(YYLEX_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

memory_tests.so: $(OBJDIR)/memory_tests.o $(OBJDIR)/memory.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

lexembuffer_tests.so: $(OBJDIR)/lexembuffer_tests.o $(OBJDIR)/lexembuffer.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

# These includes the modules .c to be able to test internal functions or macros
LEX_DEPENDENCIES = log position characterreader
LEX_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(LEX_DEPENDENCIES))
lexer_tests.so: $(OBJDIR)/lexer_tests.o $(LEX_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

CXFILE_DEPENDENCIES = protocol log position type usage
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

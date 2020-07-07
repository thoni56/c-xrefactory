vpath %.c ..

WARNINGS = -Wall -Wno-char-subscripts
CFLAGS = -g -O0 -MMD -funsigned-char $(WARNINGS) -fPIC -I.. $(COVERAGE)

OBJDIR=.objects

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

CGREEN = -lcgreen

debug: COVERAGE=--coverage
all debug: $(UNITTESTS)
	lcov -q -z -d $(OBJDIR)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^

clean:
	-rm $(OBJDIR)/*.[od] *.so $(OBJDIR)/*.gc*

cleaner: clean
	-rm -rf $(OBJDIR)

*.c: Makefile

# These link the modules .o and only tests the external behaviour
symboltable_tests.so: Makefile $(OBJDIR)/symboltable_tests.o $(OBJDIR)/symboltable.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

jslsemact_tests.so: Makefile $(OBJDIR)/jslsemact_tests.o $(OBJDIR)/jslsemact.o $(OBJDIR)/symbol.o $(OBJDIR)/log.o $(OBJDIR)/id.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

olcxtab_tests.so: Makefile $(OBJDIR)/olcxtab_tests.o $(OBJDIR)/olcxtab.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

filetable_tests.so: Makefile $(OBJDIR)/filetable_tests.o $(OBJDIR)/filetable.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

symbol_tests.so: Makefile $(OBJDIR)/symbol_tests.o $(OBJDIR)/symbol.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

semact_tests.so: Makefile  $(OBJDIR)/semact_tests.o $(OBJDIR)/semact.o $(OBJDIR)/protocol.o $(OBJDIR)/hash.o $(OBJDIR)/position.o $(OBJDIR)/log.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

misc_tests.so: Makefile  $(OBJDIR)/misc_tests.o $(OBJDIR)/misc.o $(OBJDIR)/protocol.o $(OBJDIR)/type.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

commons_tests.so: Makefile $(OBJDIR)/commons_tests.o $(OBJDIR)/commons.o $(OBJDIR)/protocol.o $(OBJDIR)/log.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

editor_tests.so: Makefile $(OBJDIR)/editor_tests.o $(OBJDIR)/editor.o $(OBJDIR)/position.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

cxref_tests.so: Makefile $(OBJDIR)/cxref_tests.o $(OBJDIR)/cxref.o $(OBJDIR)/protocol.o $(OBJDIR)/hash.o $(OBJDIR)/position.o $(OBJDIR)/type.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

OPTIONS_DEPENDENCIES = protocol log position
OPTIONS_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(OPTIONS_DEPENDENCIES))
options_tests.so: Makefile $(OBJDIR)/options_tests.o $(OBJDIR)/options.o $(OPTIONS_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

YYLEX_DEPENDENCIES = protocol log position filedescriptor id filetable hash symboltable macroargumenttable symbol lexembuffer
YYLEX_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(YYLEX_DEPENDENCIES))
yylex_tests.so: Makefile $(OBJDIR)/yylex_tests.o $(OBJDIR)/yylex.o $(YYLEX_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

LEXEMBUFFER_DEPENDENCIES = characterreader position
LEXEMBUFFER_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(LEXEMBUFFER_DEPENDENCIES))
lexembuffer_tests.so: Makefile $(OBJDIR)/lexembuffer_tests.o $(OBJDIR)/lexembuffer.o $(LEXEMBUFFER_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

memory_tests.so: Makefile $(OBJDIR)/memory_tests.o $(OBJDIR)/memory.o $(OBJDIR)/log.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

position_tests.so: Makefile $(OBJDIR)/position_tests.o $(OBJDIR)/position.o $(OBJDIR)/log.o
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)


# These includes the modules .c to be able to test internal functions or macros
LEXER_DEPENDENCIES = log position lexembuffer characterreader
LEXER_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(LEXER_DEPENDENCIES))
lexer_tests.so: Makefile $(OBJDIR)/lexer_tests.o $(LEXER_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

CXFILE_DEPENDENCIES = protocol log position type usage
CXFILE_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(CXFILE_DEPENDENCIES))
cxfile_tests.so: Makefile $(OBJDIR)/cxfile_tests.o $(CXFILE_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

EXTRACT_DEPENDENCIES = protocol log position memory id
EXTRACT_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(EXTRACT_DEPENDENCIES))
extract_tests.so: Makefile $(OBJDIR)/extract_tests.o $(EXTRACT_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)



# A test lib for a module is built from its .o and the _test.o (and some extras)
#%_tests.so: Makefile %.o %_tests.o $(EXTRA_OBJS)
#	gcc -shared -o $@ $(sort $(EXTRA_OBJS) $^) $(LDLIBS) -lcgreen

$(OBJDIR)/%_tests.o: %_tests.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/:
	-mkdir $@

-include $(OBJDIR)/*.d

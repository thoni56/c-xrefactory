vpath %.c ..

CFLAGS = -g -MMD -Wall -fPIC -I.. $(COVERAGE) -I/usr/local/include

OBJDIR=.objs

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

CGREEN = -L/usr/local/lib -lcgreen

all: $(UNITTESTS)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^

clean:
	-rm $(OBJDIR)/*.o *.so

cleaner: clean
	-rm -rf $(OBJDIR)

coverage: COVERAGE=--coverage
coverage: clean all
	lcov -d . -c -o coverage.info
	genhtml -o coverage coverage.info


# These link the modules .o and only tests the external behaviour
symboltable_tests.so: $(OBJDIR)/symboltable_tests.o $(OBJDIR)/symboltable.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

jslsemact_tests.so: $(OBJDIR)/jslsemact_tests.o $(OBJDIR)/jslsemact.o $(OBJDIR)/symbol.o $(OBJDIR)/log.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

olcxtab_tests.so: $(OBJDIR)/olcxtab_tests.o $(OBJDIR)/olcxtab.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

filetab_tests.so: $(OBJDIR)/filetab_tests.o $(OBJDIR)/filetab.o $(OBJDIR)/hash.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

symbol_tests.so: $(OBJDIR)/symbol_tests.o $(OBJDIR)/symbol.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

editor_tests.so: $(OBJDIR)/editor_tests.o $(OBJDIR)/editor.o $(OBJDIR)/position.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

cxref_tests.so: $(OBJDIR)/cxref_tests.o $(OBJDIR)/cxref.o $(OBJDIR)/protocol.o $(OBJDIR)/enumTxt.o $(OBJDIR)/hash.o $(OBJDIR)/position.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)


# These includes the modules .c to be able to test static functions
LEX_DEPENDENCIES = log position
lex_tests.so: $(OBJDIR)/lex_tests.o $(patsubst %,$(OBJDIR)/%.o,$(LEX_DEPENDENCIES))
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

CXFILE_DEPENDENCIES = protocol log position
CXFILE_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(CXFILE_DEPENDENCIES))
cxfile_tests.so:$(OBJDIR)/cxfile_tests.o $(CXFILE_DEPS)
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

EXTRACT_DEPENDENCIES = protocol log position
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

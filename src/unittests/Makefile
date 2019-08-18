vpath %.c ..

CFLAGS = -g -MMD -MT .d -fPIC -I.. $(COVERAGE)

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

lex_tests.so: $(OBJDIR)/lex_tests.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

symtab_tests.so: $(OBJDIR)/symtab_tests.o $(OBJDIR)/symtab.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

olcxtab_tests.so: $(OBJDIR)/olcxtab_tests.o $(OBJDIR)/olcxtab.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

filetab_tests.so: $(OBJDIR)/filetab_tests.o $(OBJDIR)/filetab.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

CXFILE_DEPENDENCIES = protocol #enumTxt globals commons enumTxt reftab cgram semact jslsemact complete cxref olcxtab cfread symtab editor editorbuffertab jsltypetab
cxfile_tests.so:$(OBJDIR)/cxfile_tests.o $(patsubst %,$(OBJDIR)/%.o,$(CXFILE_DEPENDENCIES))
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)


# A test lib for a module is built from its .o and the _test.o (and some extras)
%_tests.so: %.o %_tests.o $(EXTRA_OBJS)
	gcc -shared -o $@ $(sort $(EXTRA_OBJS) $^) $(LDLIBS) -lcgreen

$(OBJDIR)/%_tests.o: %_tests.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/:
	-mkdir $@

-include $(OBJDIR)/*.d

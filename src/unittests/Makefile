vpath %.c ..

CFLAGS = -g -MMD -MT .d -fPIC -I.. $(COVERAGE)

OBJDIR=objs

UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

CGREEN = -L/usr/local/lib -lcgreen

all: $(UNITTESTS)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^


symtab_tests.so: $(OBJDIR)/symtab_tests.o $(OBJDIR)/symtab.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

olcxtab_tests.so: $(OBJDIR)/olcxtab_tests.o $(OBJDIR)/olcxtab.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

filetab_tests.so: $(OBJDIR)/filetab_tests.o $(OBJDIR)/filetab.o
	gcc -shared -o $@ $^ $(COVERAGE) $(CGREEN)

$(OBJDIR)/%_tests.o: %_tests.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/:
	-mkdir $@

-include $(OBJDIR)/*.d

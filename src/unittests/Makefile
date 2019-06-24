CFLAGS = -g -MMD -MT .d -fPIC -I..

OBJDIR=objs

UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

all: $(UNITTESTS)
	cgreen-runner --suite unittests $^


symtab_tests.so: $(OBJDIR)/symtab_tests.o $(OBJDIR)/symtab.o
	gcc -shared -o $@ $^ -lcgreen

olcxtab_tests.so: $(OBJDIR)/olcxtab_tests.o $(OBJDIR)/olcxtab.o
	gcc -shared -o $@ $^ -lcgreen

$(OBJDIR)/%_tests.o: %_tests.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)/
	gcc -o $@ $(CFLAGS) -c $<

$(OBJDIR)/:
	-mkdir $@

-include $(OBJDIR)/*.d

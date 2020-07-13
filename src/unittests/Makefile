vpath %.c ..

WARNINGS = -Wall -Wno-char-subscripts
CFLAGS = -g -O0 -MMD -funsigned-char $(WARNINGS) -fPIC -I.. $(COVERAGE) -DHAVE_ZLIB

OBJDIR=.objects

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.so,$(wildcard *_tests.c))

# Assuming Cgreen is installed in "normal" place
CGREEN = -lcgreen

debug: COVERAGE=--coverage
all debug: $(UNITTESTS)
	lcov -q -z -d $(OBJDIR)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^

clean:
	-rm $(OBJDIR)/*.[od] *.so $(OBJDIR)/*.gc*

cleaner: clean
	-rm -rf $(OBJDIR)

.objs/*.o: Makefile

# Macro for unittests that link the SUT and a small number of dependent .o
define UNITTEST
$(1)_DEPENDENCIES = $(2)
$(1)_DEPS = $$(patsubst %,$(OBJDIR)/%.o,$$($(1)_DEPENDENCIES))
$(1)_tests.so: $(OBJDIR)/$(1)_tests.o $(OBJDIR)/$(1).o $$($(1)_DEPS)
	gcc -shared -o $$@ $$^ $$(COVERAGE) $$(CGREEN)
endef

$(eval $(call UNITTEST,options,protocol log position))
$(eval $(call UNITTEST,symboltable,hash))
$(eval $(call UNITTEST,jslsemact,symbol log id))
$(eval $(call UNITTEST,olcxtab,hash))
$(eval $(call UNITTEST,filetable,hash))
$(eval $(call UNITTEST,symbol))
$(eval $(call UNITTEST,semact,protocol hash position log))
$(eval $(call UNITTEST,misc,protocol type))
$(eval $(call UNITTEST,commons,protocol log))
$(eval $(call UNITTEST,editor,position))
$(eval $(call UNITTEST,cxref,protocol hash position type))
$(eval $(call UNITTEST,yylex,protocol log position filedescriptor id filetable hash symboltable macroargumenttable symbol lexembuffer))
$(eval $(call UNITTEST,lexembuffer,characterreader position))
$(eval $(call UNITTEST,memory,memory log))
$(eval $(call UNITTEST,position,log))

# These includes the modules .c to be able to test internal functions or macros
LEXER_DEPENDENCIES = log position lexembuffer characterreader
LEXER_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(LEXER_DEPENDENCIES))
lexer_tests.so: $(OBJDIR)/lexer_tests.o $(LEXER_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

CXFILE_DEPENDENCIES = protocol log position type usage
CXFILE_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(CXFILE_DEPENDENCIES))
cxfile_tests.so: $(OBJDIR)/cxfile_tests.o $(CXFILE_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)

EXTRACT_DEPENDENCIES = protocol log position memory id
EXTRACT_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(EXTRACT_DEPENDENCIES))
extract_tests.so: $(OBJDIR)/extract_tests.o $(EXTRACT_DEPS)
	gcc -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE) $(CGREEN)



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

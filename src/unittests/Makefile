vpath %.c ..
vpath %.dll.a /usr/local:/usr/local/lib
vpath %.so /usr/lib:/usr/local/lib
vpath %.dylib /usr/lib:/usr/local/lib


WARNINGS = -Wall -Wno-char-subscripts
CFLAGS = -g -O0 -MMD -funsigned-char $(WARNINGS) -fPIC -I.. -I/usr/include -I/usr/local/include $(COVERAGE) -DHAVE_ZLIB

OBJDIR=.objects

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.$(EXT),$(wildcard *_tests.c))

OS := $(shell uname)
ifeq ($(OS),Darwin)
	EXT=dylib
else
	EXT=so
endif

# Assuming make (>3.something, not Apple!) can use vpath to locate Cgreen library
CGREEN = -lcgreen

# We need to use compatible gcov (Darwin/Brew now have GCC v10)
ifeq ("$(CC)", "gcc-10")
	LCOV = lcov --gcov-tool gcov-10
else
	LCOV = lcov
endif

# NOTE: lcov >= 1.14 does not have --exclude
LCOV_EXCLUDES = --exclude 'log.c' --exclude '*.mock' --exclude '*.tab.c' --exclude '/usr/*' --exclude '/Library/*'
LCOV_IGNORE_MODULES = 'log.c' '*.mock' '*.tab.c' '/usr/*' '/Library/*'

all: $(UNITTESTS)
	@-[ -x "$$(command -v lcov)" ] && lcov -q -z -d $(OBJDIR)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^
	filecount=`ls -1 $(OBJDIR)/*.gcda 2>/dev/null | wc -l` ; \
	if [ $$filecount > 0 ] ; then \
		$(LCOV) -q -t unittests -d . -c -o unittests.info ; \
		$(LCOV) -q --remove unittests.info $(LCOV_IGNORE_MODULES) -o unittests.info ; \
	fi

clean:
	-rm $(OBJDIR)/*.[od] *.$(EXT) $(OBJDIR)/*.gc*

cleaner: clean
	-rm -rf $(OBJDIR)

.objs/*.o: Makefile

# Be silent unless VERBOSE is set to something (VERBOSE=1)
$(VERBOSE).SILENT:

# Macro for unittests that link the SUT and a small number of dependent .o
define UNITTEST
$(1)_DEPENDENCIES = $(2)
$(1)_DEPS = $$(patsubst %,$(OBJDIR)/%.o,$$($(1)_DEPENDENCIES))
$(1)_tests.$(EXT): $(OBJDIR)/$(1)_tests.o $(OBJDIR)/$(1).o $$($(1)_DEPS) $$(CGREEN)
	$(CC) -shared -o $$@ $$^ $$(COVERAGE)
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
$(eval $(call UNITTEST,id))

# These includes the modules .c to be able to test internal functions or macros
LEXER_DEPENDENCIES = log position lexembuffer characterreader
LEXER_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(LEXER_DEPENDENCIES))
lexer_tests.$(EXT): $(OBJDIR)/lexer_tests.o $(LEXER_DEPS) $(CGREEN)
	$(CC) -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE)

CXFILE_DEPENDENCIES = protocol log position type usage
CXFILE_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(CXFILE_DEPENDENCIES))
cxfile_tests.$(EXT): $(OBJDIR)/cxfile_tests.o $(CXFILE_DEPS) $(CGREEN)
	$(CC) -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE)

EXTRACT_DEPENDENCIES = protocol log position memory id
EXTRACT_DEPS = $(patsubst %,$(OBJDIR)/%.o,$(EXTRACT_DEPENDENCIES))
extract_tests.$(EXT): $(OBJDIR)/extract_tests.o $(EXTRACT_DEPS) $(CGREEN)
	$(CC) -shared -o $@ $(filter-out Makefile,$^) $(COVERAGE)



# A test lib for a module is built from its .o and the _test.o (and some extras)
#%_tests.$(EXT): %.o %_tests.o $(EXTRA_OBJS)
#	$(CC) -shared -o $@ $(sort $(EXTRA_OBJS) $^) $(LDLIBS) -lcgreen

$(OBJDIR)/%_tests.o: %_tests.c | $(OBJDIR)/
	$(CC) -o $@ $(CFLAGS) -c $<

$(OBJDIR)/%.o: ../%.c | $(OBJDIR)/
	$(CC) -o $@ $(CFLAGS) -c $<

$(OBJDIR)/:
	-mkdir $@

-include $(OBJDIR)/*.d

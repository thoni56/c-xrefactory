vpath %.c ..
vpath %.dll.a /usr/local:/usr/local/lib
vpath %.so /usr/lib:/usr/local/lib
vpath %.dylib /usr/lib:/usr/local/lib

WARNINGS = -Wall -Wno-char-subscripts
CFLAGS = -DUNITTESTING -g -O0 -MMD -funsigned-char $(WARNINGS) -fPIC -I.. -I/usr/include -I/usr/local/include $(COVERAGE) -DHAVE_ZLIB

OBJDIR=.objects

# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.$(EXT),$(wildcard *_tests.c))

OS := $(shell uname)
ifeq ($(OS),Darwin)
	EXT=dylib
else
	EXT=so
endif

# If you have a make >3.something (MacOS don't!) you can use vpath to locate Cgreen library
# as a dependency which will automatically be linked
# CGREEN_LIB = -lcgreen
# else you have to add explicit linkage directives
CGREEN_LIB = -L/usr/local/lib -lcgreen
# And for Homebrew on M1
# CGREEN_LIB = -L/opt/homebrew/lib -lcgreen

# We need to use compatible gcov (Darwin/Brew now have GCC v11)
ifeq ("$(CC)", "gcc-11")
	LCOV = lcov --gcov-tool gcov-11
else
	LCOV = lcov
endif

COVERAGE = --coverage

# NOTE: lcov >= 1.14 does not have --exclude
LCOV_EXCLUDES = --exclude 'log.c' --exclude '*.mock' --exclude '*.tab.c' --exclude '/usr/*' --exclude '/Library/*'
LCOV_IGNORE_COVERAGE = 'log.c' '*.mock' '*.tab.c' '/usr/*' '/Library/*'


ifeq ($(shell which cgreen-runner),)
all:
	@echo Cgreen not available, cannot run unittests
else
all: $(UNITTESTS)
	@-[ -x "$$(command -v lcov)" ] && lcov -q -z -d $(OBJDIR)
	cgreen-runner $(CGREEN_OUTPUT) --suite unittests $^
	filecount=`ls -1 $(OBJDIR)/*.gcda 2>/dev/null | wc -l` ; \
	if [ $$filecount -gt 0 ] ; then \
		$(LCOV) -q -t unittests -d . -c -o unittests.info ; \
		$(LCOV) -q --remove unittests.info $(LCOV_IGNORE_COVERAGE) -o unittests.info ; \
		if command -v info2gcov > /dev/null 2>&1 ; then \
			info2gcov -q unittests.info ; \
		fi ; \
	fi
endif

clean:
	-rm -r $(OBJDIR) *.$(EXT)

.objs/*.o: Makefile


# Macro for unittests that link the SUT and a small number of dependent .o
# Parameters: <module>, <dependent modules ...>
define UNITTEST
$(1)_DEPENDENCIES = $(2)
$(1)_DEPS = $$(patsubst %,$(OBJDIR)/%.o,$$($(1)_DEPENDENCIES))
$(1)_tests.$(EXT): $(OBJDIR)/$(1)_tests.o $(OBJDIR)/$(1).o $$($(1)_DEPS)
	$(CC) -shared -o $$@ $$^ $$(COVERAGE) $$(CGREEN_LIB)
endef

$(eval $(call UNITTEST,classfilereader,log memory position))
$(eval $(call UNITTEST,caching,log memory))
$(eval $(call UNITTEST,commons,log protocol stringlist))
$(eval $(call UNITTEST,complete,log memory protocol type))
$(eval $(call UNITTEST,classhierarchy,log memory protocol))
$(eval $(call UNITTEST,cxref,server protocol log hash position type storage usage olcxtab memory))
$(eval $(call UNITTEST,cxfile,log memory protocol position))
$(eval $(call UNITTEST,editor,position log memory))
$(eval $(call UNITTEST,extract,position log memory storage type id usage protocol))
$(eval $(call UNITTEST,fileio,log))
$(eval $(call UNITTEST,filetable,log memory hash))
$(eval $(call UNITTEST,id,log memory))
$(eval $(call UNITTEST,jslsemact,log memory symbol id))
$(eval $(call UNITTEST,jsemact,log memory position symbol id))
$(eval $(call UNITTEST,lexer,log lexembuffer characterreader position))
$(eval $(call UNITTEST,lexembuffer,characterreader position log))
$(eval $(call UNITTEST,main,memory log protocol position))
$(eval $(call UNITTEST,memory,log))
$(eval $(call UNITTEST,misc,protocol log type memory))
$(eval $(call UNITTEST,olcxtab,memory log hash))
$(eval $(call UNITTEST,options,protocol log position memory))
$(eval $(call UNITTEST,ppc,protocol))
$(eval $(call UNITTEST,position,log))
$(eval $(call UNITTEST,refactory,protocol position log memory))
$(eval $(call UNITTEST,semact,memory log protocol hash position storage type))
$(eval $(call UNITTEST,symbol,memory log))
$(eval $(call UNITTEST,symboltable,hash log memory))
$(eval $(call UNITTEST,yylex,memory protocol log position filedescriptor id filetable hash symboltable macroargumenttable symbol lexembuffer stringlist))


$(OBJDIR)/main.o: CFLAGS+=-DGIT_HASH=\"NOT_A_REAL_GIT_HASH\"


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

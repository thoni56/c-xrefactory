#######################################################################
# c-xrefactory Makefile
#
# This makefile supports the following use cases
#
# 1: production - simple straight forward build of c-xref for
#    production use when installing c-xref using el-get:
#
#	make prod
#
# 2: ci - build c-xref and run all tests with coverage and create
#    coverage report:
#
#	make ci
#
# 3: all (or no target) - build and run unittests & build the c-xref
# exe:
#
#	make
#
# 4: build and run unittests with coverage and produce gcov files for
#    cov-mode in Emacs:
#
#	make unit
#
# 5: build c-xref with coverage, run all "quick" tests and produce
# gcov:
#
#	make devel
#
# 6: build c-xref with coverage, run one test and produce gcov files:
#
#       make
#	make clean-coverage
#	<run test>
#	make coverage-report
#
# 6: run another test and produce aggregated gcov files:
#
#	<run another test>
#	make coverage-report
#
# 7: produce coverage report for current collected coverage:
#
#	make coverage-report
#
# 8: watch sources to build and run all tests:
#
#	make watch
#
# 9: watch sources to build and run unittests (use as compile command in Emacs!):
#
#	make watch-for-unittests
#
# 10: watch unitests completion and run all systemtests (use in a separate
#     terminal):
#
#	make watch-for-systemtests
#
#########################################################################

OS := $(shell uname)

# Figure out reasonable number of parallel jobs
# Set number of jobs based on operating system
ifeq ($(OS),Linux)
    JOBS := $(shell nproc)
endif
ifeq ($(OS),Darwin)
    JOBS := $(shell sysctl -n hw.physicalcpu)
endif
# Fallback if number of jobs is not set
ifeq ($(JOBS),)
    JOBS := 1
endif

# Always export following variables to environment of sub-makes (to use you need "-e")

# Build with coverage for all cases except "prod"
export COVERAGE = --coverage
export CC = cc

# Other variables

MAKEFLAGS += --no-print-directory

WARNINGS = -Wall -Wno-char-subscripts -Werror -Wimplicit-fallthrough

CFLAGS = -g -O0 $(INCLUDES) -fPIC -MMD -funsigned-char $(WARNINGS) $(COVERAGE) $(EXTRA_CFLAGS)

LDFLAGS = $(COVERAGE)

ROOTDIR=..

# We need our own yacc - although it is no longer patched for
# recursive parsing for Java, the grammars are not adapted to modern
# byacc and yacc/bison
YACC=$(ROOTDIR)/byacc-1.9/yacc

# On Darwin we can use gcc-13 if available
ifeq ($(OS),Darwin)
#	ifneq (, $(shell command -v gcc-13 2> /dev/null))
#		CC=gcc-13
#	endif
#	ifeq ($(shell arch),arm64)
#		# And for Homebrew on M1
#		CC=cc
#	endif
# But we'd rather go with Apple Clang and generate fat binaries
CFLAGS += -arch arm64 -arch x86_64
LDFLAGS += -arch arm64 -arch x86_64
endif

# We need to use compatible gcov (for Darwin/Brew with Homebrew gcc-13 we need to use GCC:s gcov)
ifeq ("$(CC)", "gcc-13")
	LCOV = lcov --gcov-tool gcov-13
	GCOV = gcov-13
	GCOV_TOOL = gcov-tool-13
else
	LCOV = lcov
	GCOV = gcov
	GCOV_TOOL = gcov-tool
endif

# We might not have gcov-tool available
ifneq (, $(shell command -v $(GCOV_TOOL) 2> /dev/null))
    HAVE_GCOV_TOOL=yes
else
    HAVE_GCOV_TOOL=no
endif

ifneq (, $(shell lcov --version | grep 'version 2'))
	HAVE_LCOV2=yes
else
	HAVE_LCOV2=no
endif

# If we don't then we can generate .gcov from the .info but we need:
ifneq (, $(shell command -v info2gcov 2> /dev/null))
    HAVE_INFO2GCOV=yes
else
    HAVE_INFO2GCOV=no
endif


all: unit build

#########################################################################
# For production - no smartness just compile, ensure clean production build
prod: COVERAGE =
prod:
	$(MAKE) clean
	$(MAKE) c-xref

#########################################################################
# For development - various ways to build
devel: CFLAGS += -DYYDEBUG
devel: check-tokens
	$(MAKE) -C ../editors/emacs
	$(MAKE) -e clean-coverage
	$(MAKE) -e devel-proper
	$(MAKE) coverage-report

devel-proper: unit build
	$(MAKE) -e test

slow:
	$(MAKE) -C ../tests slow
	$(MAKE) gcov

ci:
	$(MAKE) -e -j unit
	$(MAKE) -e -j build
	$(MAKE) -e -C ../tests/systemd init
	$(MAKE) -e -C ../tests/ffmpeg init
	$(MAKE) -e -j -C ../tests all
	for tc in ../tests/*/.coverage ; do $(GCOV_TOOL) merge $$tc $(OBJDIR) -o $(OBJDIR) ; done


yydebug: CFLAGS += -DYYDEBUG
yydebug: devel

chcheck:
	for m in `echo $(MODULES) | sed -e 's/[a-z]*_parser\.tab//g'` ; do \
		chcheck $$m ; \
	done

#########################################################################
include sources.mk

# Automatically generate a config header with GIT hash/tag info
options_config.h: options_config.h.in generate_options_config.sh
	./generate_options_config.sh
options.c: options_config.h

PHONY += build
build: build-proper

build-proper: c-xref
	$(MAKE) -C ../editors/emacs

PHONY += c-xref
c-xref: check-tokens $(OBJS) editor-defs
	$(CC) $(LDFLAGS) -o c-xref $(OBJS) $(LIBS)

editor-defs: refactorings.def
	$(MAKE) -C ../editors

refactorings.def: refactorings.h
	# Convert enums in refactorings.h into #defines that the editor extensions can use
	grep AVR refactorings.h | sed -e 's/ *\(AVR.*\) =/#define \1/' -e 's/,//' > refactorings.def


#########################################################################
test:
	@echo "Only running quick tests for now - 'cd ../test; make all' to run all"
	$(MAKE) -e -C ../tests quick

unit: check-tokens unittests

#########################################################################
# We generate parsers for C, Yacc and C expressions

PHONY += parsers
parsers: c_parser.tab.[ch] yacc_parser.tab.[ch] cexp_parser.tab.[ch]

# Note the file prefix (-b) and symbol prefix (-p)
# Also creates .rules files with only the rules in it

# Here's some Makefile magic to make two targets with one recepie,
# thanks to https://stackoverflow.com/a/10609434/204658

.INTERMEDIATE: c_parser.tab.intermediate
c_parser.tab.c c_parser.tab.h : c_parser.tab.intermediate ;
c_parser.tab.intermediate : c_parser.y $(YACC)
	$(YACC) -v -d -b c_parser -p c_yy c_parser.y
	sed -E '/^$$/q' c_parser.output > c_parser.rules
	awk '{$$1=""; print}' c_parser.rules > c_parser.tmp
	sed -n '/primary_expr/,$$p' c_parser.tmp > c_rules_from_c_parser.txt

# Extract the C rules in yacc format and merge them into yacc_parser.y
yacc_parser.y: c_parser.y
	@echo "Yacc grammar needs updating from c_parser.y ..."
	sed -n '/^%%$$/,/^%%$$/{/^%%$$/!{/^%%$$/!p;};}' c_parser.y > c_rules.txt
	sed '/NOW FOLLOWS THE COMPLETE C GRAMMAR/q' yacc_parser.y > yacc_parser.head
	sed '1,/^%%$$/d' yacc_parser.y | sed -n '/^%%$$/,$$p' > yacc_parser.tail
	cat yacc_parser.head c_rules.txt yacc_parser.tail > yacc_parser.y
	rm  yacc_parser.head c_rules.txt yacc_parser.tail

.INTERMEDIATE: cexp_parser.tab.intermediate
cexp_parser.tab.c cexp_parser.tab.h : cexp_parser.tab.intermediate ;
cexp_parser.tab.intermediate : cexp_parser.y $(YACC)
	$(YACC) -v -b cexp_parser -p cexp_yy cexp_parser.y
	sed -E '/^$$/q' cexp_parser.output > cexp_parser.rules

.INTERMEDIATE: yacc_parser.tab.intermediate
yacc_parser.tab.c yacc_parser.tab.h : yacc_parser.tab.intermediate ;
yacc_parser.tab.intermediate : yacc_parser.y $(YACC)
	$(YACC) -v -b yacc_parser -p yacc_yy yacc_parser.y
	sed -E '/^$$/q' yacc_parser.output > yacc_parser.rules
	awk '{$$1=""; print}' yacc_parser.rules > yacc_parser.tmp
	sed -n '/primary_expr/,$$p' yacc_parser.tmp > c_rules_from_yacc_parser.txt


# There is no include feature in grammars so we need to duplicate
# token definitons. Here we ensure they are in sync
# Extract all three token sections
EXTRACT_TOKEN_DEFINITIONS = sed -E '/END OF COMMON TOKEN DEFINITIONS/,$$d' | sed -E '/START OF COMMON TOKEN DEFINITIONS/,$$!d'
c_parser.tokens: c_parser.y
	cat c_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > c_parser.tokens

yacc_parser.tokens: yacc_parser.y
	cat yacc_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > yacc_parser.tokens

# Ensure they are extracted by making the parser object files dependent on them
$(OBJDIR)/c_parser.tab.o : c_parser.tokens
$(OBJDIR)/yacc_parser.tab.o : yacc_parser.tokens

# And compare the three token sections to ensure that they are exactly the same
PHONY += check-tokens
check-tokens: c_parser.tokens yacc_parser.tokens
	@echo Checking tokens across parsers ...
	@if ! diff -q c_parser.tokens yacc_parser.tokens ; then \
		echo "c_parser.tokens:1:1: ERROR: Parser token section must be identical:" ; \
		diff -c c_parser.tokens yacc_parser.tokens ; \
	fi

# Create an enum of the %token define's in c_parser.tab.h so that
# we don't need to use c_parser.tab.h to define tokens
lexem.h: c_parser.tab.h character_lexems.txt Makefile
	echo "#ifndef LEXEM_H_INCLUDED" > lexem.h.tmp
	echo "#define LEXEM_H_INCLUDED" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "/**** DO NOT EDIT - generated from c_parser.tab.h & character_lexems.txt by Makefile ****/" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "typedef enum LexemCode {" >> lexem.h.tmp
	cat character_lexems.txt >> lexem.h.tmp
	# Standard BYACC (which does not work yet) defines YYSTYPE_IS_DECLARED in .tab.h which we don't want
	grep define c_parser.tab.h | sed -e "s/#define \(.*\) \([0-9]*\)/\1 = \2,/" | grep -v YYSTYPE_IS_DECLARED >> lexem.h.tmp
	echo "} LexemCode;" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "extern char *lexemEnumNames[];" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "extern void initLexemEnumNames(void);" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "#endif" >> lexem.h.tmp
	# Sanity check for content, otherwise yacc errored out probably
	if grep " = " lexem.h.tmp > /dev/null ; then \
		mv lexem.h.tmp lexem.h ; \
	else \
		rm lexem.h.tmp ; \
	fi

# Generate an init function for lexem names from lexem.h
lexem.c: lexem.h Makefile
	echo "#include \"lexem.h\"" > lexem.c.tmp
	echo >> lexem.c.tmp
	echo "char *lexemEnumNames[LAST_TOKEN];" >> lexem.c.tmp
	echo >> lexem.c.tmp
	echo "void initLexemEnumNames(void) {" >> lexem.c.tmp
	grep = lexem.h | grep -v LAST_TOKEN | awk '{ printf "    lexemEnumNames[%s] = \"%s\";\n", $$3, $$1}' | sed 's/,]/]/g' >> lexem.c.tmp
	echo "}" >> lexem.c.tmp
	if ! cmp lexem.c lexem.c.tmp ; then \
		mv lexem.c.tmp lexem.c ; \
	else \
		rm lexem.c.tmp ; \
	fi


#########################################################################

clean:
	-$(MAKE) -C ../tests clean
	-rm -rf $(OBJDIR) c-xref *.gcov

########################################################################

PHONY += clean-coverage
clean-coverage:
	-rm ../coverage/total.info
	-if command -v $(LCOV) > /dev/null 2>&1 ; then \
		$(LCOV) -q -z -d .. ; \
	fi
	$(MAKE) gcov

# Create .gcov files from .gcda in $(OBJDIR)
gcov:
	-$(GCOV) $(OBJDIR)/*.o --object-directory $(OBJDIR) 2>&1 >/dev/null | grep -v "assuming not executed" && true

coverage-report:
# We create separate coverage files for each test to be able to run them in parallel (see ../tests/Makefile)
ifeq ($(HAVE_GCOV_TOOL),yes)
# Then we can merge all .gcda for all tests into the .gcda in OBJDIR using it, which is very fast!
	-for tc in ../tests/*/.coverage ; do $(GCOV_TOOL) merge $$tc $(OBJDIR) -o $(OBJDIR) ; done
# And then create .gcov for Emacs cov-mode
	$(MAKE) gcov
else
# We need to collect all coverage before we can generate gcov...
endif
	@echo Doing the actual reporting in the background...
	$(MAKE) the-actual-coverage-report &

LCOV_IGNORE_COVERAGE_FOR = 'log.c' '*.mock' '*.tab.c' '/usr/*' '/Library/*' '/Applications/*'

LCOV_TEST_DIRS = `ls -d ../tests/*/.coverage | xargs printf -- '-d %s\n'`
ifeq ($(OS),Darwin)
SYSTEM_TEST_COVERAGE_FILES = `find ../tests -name coverage.info | xargs stat -f "%N %z" | grep -v " 0" | awk '{print $$1}' | xargs printf -- '-a %s\n'`
else
SYSTEM_TEST_COVERAGE_FILES = `ls ../tests/*/.coverage/coverage.info | xargs printf -- '-a %s\n'`
endif

the-actual-coverage-report:
	@mkdir -p ../coverage
ifeq ($(HAVE_GCOV_TOOL),yes)
# Then we have already merged all test .gcda into OBJDIR and can just generate the total.info from .
	-$(LCOV) -d . -c -q -o ../coverage/total.info 2>&1 | grep -v "did not produce any data" | grep -v "assuming not executed"
else
# The run_test script in ../tests has created a coverage.info in each test directory
# So we can add each of the tests coverage.info to ../coverage/total.info using '-a ...'
	$(LCOV) $(SYSTEM_TEST_COVERAGE_FILES) -q -o ../coverage/total.info 2>&1
endif
	@echo "LCOV-ing done..."
	[ "$(HAVE_LCOV2)" = "yes" ] && lcovignore="--ignore-errors unused,unused" || true ; \
	$(LCOV) -q $$lcovignore --remove ../coverage/total.info $(LCOV_IGNORE_COVERAGE_FOR) -o ../coverage/total.info
	@echo "LCOV --remove done..."
ifeq ($(HAVE_GCOV_TOOL),no)
# Then we have to wait until now to generate the gcov's for Emacs cov-mode...
# And we need the info2gcov command (https://github.com/thoni56/info2gcov)
ifeq ($(HAVE_INFO2GCOV),yes)
	info2gcov -q ../coverage/total.info
else
# We have to do without the .gcov files, no Emacs coverage display sadly...
endif
endif
	genhtml -q -o ../coverage/$(COVERAGE_CASE) ../coverage/total.info
	@echo Coverage done: `grep -m 1 coverPer ../coverage/index.html | tr -dc '0-9.'`
ifneq ($(shell command -v cloc 2> /dev/null),)
	@cloc --quiet *.[chy] *.t? | grep SUM: | awk '{print "Lines:", $$5}'
endif

#########################################################################
watch:
	watchexec --no-vcs-ignore -i \*.tab.\[ch\] -i lexem.h -e c,h,y,mock,tc,th -- execnotify -t Development make -j$(JOBS) devel

watch-for-unittests:
	watchexec -i yacc_parser.y -i \*.tab.\[ch\] -i lexem.h -e c,h,y,mock,tc,th -- execnotify -t Unittests make -j$(JOBS) unit

watch-for-systemtests:
	watchexec --no-vcs-ignore -e done -- execnotify -t Systemtests make -j$(JOBS) system-tests

system-tests:
	$(MAKE) -e build
	$(MAKE) -e test
	$(MAKE) coverage-report

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist

$(YACC): $(ROOTDIR)/byacc-1.9/*.[ch]
	$(MAKE) -C $(ROOTDIR)/byacc-1.9


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

# vpath %.dll.a /usr/local:/usr/local/lib
# vpath %.so /usr/lib:/usr/local/lib
# vpath %.dylib /usr/lib:/usr/local/lib

# CGREEN_LIB = -lcgreen
# else you have to add explicit linkage and/or include directives
# CGREEN_INCLUDE = -I/usr/local/include
# CGREEN_LIB = -L/usr/local/lib -lcgreen

ifeq ($(OS),Darwin)
	ifeq ($(shell arch),arm64)
		# And for Homebrew on M1
		CGREEN_LIB = -L/opt/homebrew/lib -lcgreen
		DYLD_PATH = DYLD_LIBRARY_PATH=/opt/homebrew/lib
	else
		CGREEN_LIB = -L/usr/local/lib -lcgreen
		DYLD_PATH = DYLD_LIBRARY_PATH=/usr/local/lib
	endif
endif

PHONY += unittests
ifeq ($(shell command -v cgreen-runner 2> /dev/null),)
unittests: parsers
	@echo Cgreen not available, cannot run unittests
else
unittests: parsers $(OBJS) $(UNITTESTS)
	$(MAKE) -e clean-coverage
	$(DYLD_PATH) cgreen-runner -q $(CGREEN_OUTPUT) --suite unittests $(UNITTESTS)
	$(MAKE) gcov
	[ "$(HAVE_LCOV2)" = "yes" ] && lcovignore1="--ignore-errors gcov,gcov" || true ; \
	[ "$(HAVE_LCOV2)" = "yes" ] && lcovignore2="--ignore-errors unused,unused" || true ; \
	filecount=`ls -1 $(OBJDIR)/*.gcda 2>/dev/null | wc -l` ; \
	if [ $$filecount -gt 0 ] ; then \
		$(LCOV) -q $$lcovignore1  -t unittests -d . -c -o unittests.i 2>&1 | grep -v "did not produce any data"  | grep -v "assuming not executed" ; \
		$(LCOV) -q $$lcovignore2 --remove unittests.i $(LCOV_IGNORE_COVERAGE_FOR) -o unittests.i ; \
		mv unittests.i unittests.info ; \
	fi
	touch .unittests.done
endif

# A test lib for a module is built from its .o and the _test.o (and some extras)
#%_tests.$(EXT): %.o %_tests.o $(EXTRA_OBJS)
#	$(CC) -shared -o $@ $(sort $(EXTRA_OBJS) $^) $(LDLIBS) -lcgreen


# Macro for unittests that link the SUT and a small number of dependent .o
# Parameters: <module>, <dependent modules ...>
define UNITTEST
$(1)_DEPENDENCIES = $(2)
$(1)_DEPS = $$(patsubst %,$(OBJDIR)/%.o,$$($(1)_DEPENDENCIES))
$(OBJDIR)/$(1)_tests.o: CFLAGS += $(CGREEN_INCLUDE)
$(1)_tests.$(EXT): $(OBJDIR)/$(1)_tests.o $(OBJDIR)/$(1).o $$($(1)_DEPS)
	$(CC) $(LDFLAGS) -shared -o $$@ $$^ $$(COVERAGE) $$(CGREEN_LIB) -lz
endef


$(eval $(call UNITTEST,caching,log memory stackmemory))
$(eval $(call UNITTEST,characterreader,log))
$(eval $(call UNITTEST,classhierarchy,log memory protocol))
$(eval $(call UNITTEST,commandlogger))
$(eval $(call UNITTEST,commons,log protocol stringlist))
$(eval $(call UNITTEST,complete,log protocol type session memory stackmemory))
$(eval $(call UNITTEST,completion,log memory usage session))
$(eval $(call UNITTEST,counters))
$(eval $(call UNITTEST,cxfile,log memory usage protocol position hash session stackmemory))
$(eval $(call UNITTEST,cxref,server protocol log hash position type storage usage session memory stackmemory))
$(eval $(call UNITTEST,editor,position usage log memory stackmemory hash))
$(eval $(call UNITTEST,editorbuffer,position log memory stackmemory))
$(eval $(call UNITTEST,editorbuffertab,position log memory hash))
$(eval $(call UNITTEST,extract,position log memory stackmemory storage type id usage protocol))
$(eval $(call UNITTEST,fileio,log))
$(eval $(call UNITTEST,filetable,log memory stackmemory hash))
$(eval $(call UNITTEST,id,log memory stackmemory))
$(eval $(call UNITTEST,init,log))
$(eval $(call UNITTEST,lexembuffer,characterreader position memory stackmemory log))
$(eval $(call UNITTEST,lexer,log lexembuffer characterreader position memory stackmemory))
$(eval $(call UNITTEST,macroargumenttable,memory log hash))
$(eval $(call UNITTEST,main,memory log protocol position memory stackmemory session))
$(eval $(call UNITTEST,memory,log))
$(eval $(call UNITTEST,menu,log protocol position memory stackmemory usage))
$(eval $(call UNITTEST,misc,protocol log type memory stackmemory))
$(eval $(call UNITTEST,options,protocol log position memory stackmemory))
$(eval $(call UNITTEST,parsers,log))
$(eval $(call UNITTEST,position,log))
$(eval $(call UNITTEST,progress,protocol log))
$(eval $(call UNITTEST,refactorings,log))
$(eval $(call UNITTEST,refactory,protocol usage position log memory stackmemory session))
$(eval $(call UNITTEST,reftab,log memory hash))
$(eval $(call UNITTEST,semact,memory stackmemory log protocol usage hash position storage type id))
$(eval $(call UNITTEST,server,memory log session position stackmemory))
$(eval $(call UNITTEST,stackmemory,memory log))
$(eval $(call UNITTEST,symbol,memory stackmemory log))
$(eval $(call UNITTEST,symboltable,hash log memory stackmemory))
$(eval $(call UNITTEST,undo,memory log))
$(eval $(call UNITTEST,xref,memory log protocol))
$(eval $(call UNITTEST,yylex,memory stackmemory protocol log position filedescriptor id filetable hash symboltable macroargumenttable symbol lexembuffer stringlist))

.PHONY : $(PHONY)

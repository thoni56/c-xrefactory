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
# 4: build and run unittests with coverage and produce fresh coverage
# in .gcov files for Emacs cov-mode:
#
#	make unit
#
# 5: build c-xref with coverage, run all "quick" tests and produce
# coverage info in .gcov files for Emacs cov-mode:
#
#	make devel
#
# 6: build c-xref with coverage, run one test and produce cov-mode coverage:
#
#       make
#	make clean-coverage
#	<run test>
#	make coverage
#
# 6: run another test and produce aggregated cov-mode coverage:
#
#	<run another test>
#	make coverage
#
# 7: produce coverage HTML report for currently collected coverage:
#
#	make coverage-report
#
# 8: watch sources to build and run all tests:
#
#	make watch
#
# 9: watch sources to build and run unittests with notification (use as compile command in Emacs!):
#
#	make watch-for-unittests
#
# 10: watch for unittests completion and run the build (use in a separate
#     terminal):
#
#	make watch-for-build
#
# 11: watch for build completion and run all systemtests (use in a separate
#     terminal):
#
#	make watch-for-systemtests
#
#########################################################################

OS := $(shell uname)

# Where might libraries be?
ifeq ($(OS),Darwin)
	ifeq ($(shell arch),arm64)
		LIBS += -L/opt/homebrew/lib
		INCLUDES += -I/opt/homebrew/include
	else
		LIBS += -L/usr/local/lib
		INCLUDES += -I/usr/local/include
	endif
endif

ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
	LIBS += -L/usr/local/lib
endif

# What are they called?
OS := $(shell uname)
ifeq ($(OS),Darwin)
	EXT=dylib
else
	EXT=so
endif
ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
	EXT=dll
endif


# Extra libraries to link
LIBS += -lz -lcjson


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
ifeq ($(OS),Darwin)
	# Use Apple's native coverage instrumentation on Darwin
	export COVERAGE = -fprofile-instr-generate -fcoverage-mapping
	export LLVM_PROFDATA = xcrun llvm-profdata
	export LLVM_COV = xcrun llvm-cov
else
	# Use traditional gcov on other platforms
	export COVERAGE = --coverage
endif
export CC = cc

# Other variables

MAKEFLAGS += --no-print-directory

WARNINGS = -Wall -Wno-char-subscripts -Werror -Wimplicit-fallthrough

OPTIMIZATION = -O0

YYDEBUG = -DYYDEBUG

CFLAGS = -g $(OPTIMIZATION) $(INCLUDES) -fPIC -MMD -funsigned-char $(WARNINGS) $(COVERAGE) $(YYDEBUG) $(EXTRA_CFLAGS)

LDFLAGS = $(COVERAGE)

ROOTDIR=..

# We need our own yacc - although it is no longer patched for
# recursive parsing for Java, the grammars are not adapted to modern
# byacc and yacc/bison
YACC=$(ROOTDIR)/byacc-1.9/yacc

# On Darwin we can use gcc-14 if available
# CC=gcc-14

# Centralize lcov version handling and error flags for both Darwin and non-Darwin
ifneq (, $(shell lcov --version | grep 'version 2'))
	HAVE_LCOV2=yes
	LCOV_IGNORE_FLAGS = --ignore-errors unsupported,unsupported --ignore-errors gcov,gcov --ignore-errors unused,unused --ignore-errors inconsistent,inconsistent
	GENHTML_IGNORE_FLAGS = --ignore-errors inconsistent,inconsistent --ignore-errors unused,unused
else
	HAVE_LCOV2=no
	LCOV_IGNORE_FLAGS =
	GENHTML_IGNORE_FLAGS =
endif

ifeq ($(OS),Darwin)
	# Darwin: Use Apple's native coverage tools - much simpler!
	LLVM_PROFDATA = xcrun llvm-profdata
	LLVM_COV = xcrun llvm-cov
	# For compatibility with existing targets
	HAVE_APPLE_COV=yes
	HAVE_GCOV_TOOL=no  # Don't use gcov-tool on Darwin
else
	# Non-Darwin: Use traditional gcov/lcov toolchain
	# We need to use compatible gcov (for Darwin/Brew with Homebrew gcc-14 we need to use GCC:s gcov)
	ifeq ("$(CC)", "gcc-15")
		LCOV_BASE = lcov --gcov-tool gcov-15
		GCOV = gcov-15
		GCOV_TOOL = gcov-tool-15
	else ifeq ("$(CC)", "gcc-14")
		LCOV_BASE = lcov --gcov-tool gcov-14
		GCOV = gcov-14
		GCOV_TOOL = gcov-tool-14
	else
		LCOV_BASE = lcov
		GCOV = gcov
		GCOV_TOOL = gcov-tool
	endif

	# We might not have gcov-tool available
	ifneq (, $(shell command -v $(GCOV_TOOL) 2> /dev/null))
	    HAVE_GCOV_TOOL=yes
	else
	    HAVE_GCOV_TOOL=no
	endif

	# Final LCOV command with all flags
	LCOV = $(LCOV_BASE) $(LCOV_IGNORE_FLAGS)
	HAVE_APPLE_COV=no
endif

# Final GENHTML command with all flags (shared by both Darwin and non-Darwin)
GENHTML = genhtml $(GENHTML_IGNORE_FLAGS)

# Export the lcov tools for sub-makes
export LCOV
export GENHTML

# If we don't then we can generate .gcov from the .info but we need:
ifneq (, $(shell command -v info2gcov 2> /dev/null))
    HAVE_INFO2GCOV=yes
else
    HAVE_INFO2GCOV=no
endif


.PHONY:all
all: unit build

#########################################################################
# For production - no smartness just compile
.PHONY:prod
prod:
	$(MAKE) COVERAGE= YYDEBUG= OPTIMIZATION=-O3 c-xref

#########################################################################
# For development - various ways to build
.PHONY:devel
devel: check-tokens
	$(MAKE) -C ../editors/emacs
	$(MAKE) -e clean-coverage
	$(MAKE) -e devel-proper
	$(MAKE) -e system-tests
	@$(MAKE) cloc

.PHONY:devel-proper
devel-proper: unit build

.PHONY:ci
ci: OPTIMIZATION=-O3
ci:
	$(MAKE) -e -j unit
	$(MAKE) -e -j build
	$(MAKE) -e -C ../tests/test_systemd init
	$(MAKE) -e -C ../tests/test_ffmpeg init
	$(MAKE) -e -j -C ../tests all
	$(MAKE) -e merge-coverage-from-tests
	rm *.mock.gcov *.tab.*.gcov


.PHONY:yydebug
yydebug:
	touch parsers.h *.tab.c options.c
	$(MAKE) EXTRA_CFLAGS=-DYYDEBUG build

.PHONY:chcheck
chcheck:
	for m in `echo $(MODULES) | sed -e 's/[a-z]*_parser\.tab//g'` ; do \
		chcheck $$m ; \
	done

#########################################################################
include sources.mk

# Automatically generate a config header with GIT hash/tag info
TAG_FILE := .last_git_tag
options_config.h: options_config.h.in generate_options_config.sh $(TAG_FILE)
	./generate_options_config.sh
options.c: options_config.h

# Always check if git tag has changed
.PHONY: $(TAG_FILE)
$(TAG_FILE):
	@echo "$$(git describe --abbrev=0 --tags)" > $@.tmp
	@if [ ! -f $@ ] || ! cmp -s $@.tmp $@; then mv $@.tmp $@; else rm $@.tmp; fi


.PHONY:build
build: build-proper

.PHONY:build-proper
build-proper: c-xref
	$(MAKE) -C ../editors/emacs

c-xref: check-tokens $(OBJS) editor-defs
	$(CC) $(LDFLAGS) -o c-xref $(OBJS) $(LIBS)
	@touch .build.done

editor-defs: refactorings.def
	$(MAKE) -C ../editors

refactorings.def: refactorings.h
	# Convert enums in refactorings.h into #defines that the editor extensions can use
	grep AVR refactorings.h | sed -e 's/ *\(AVR.*\) =/#define \1/' -e 's/,//' > refactorings.def


#########################################################################
test:
	@echo "Only running quick tests for now - 'cd ../test; make all' to run all"
	$(MAKE) -e -C ../tests -k quick

unit: check-tokens unittests

#########################################################################
# We generate parsers for C, Yacc and C expressions

.PHONY:parsers
parsers: c_parser.tab.[ch] yacc_parser.tab.[ch] cppexp_parser.tab.[ch]

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
# NOTE: since this is partially a generated file, watching file
# changes need to ignore yacc_parser.y lest it would generate spurious
# rebuilds. This makes it necessary to force recompilation manually if
# any change is done to the parts of yacc_parser.y that are not copied
# from c_parser.y
yacc_parser.y: c_parser.y
	@echo "Yacc grammar needs updating from c_parser.y ..."
	sed -n '/^%%$$/,/^%%$$/{/^%%$$/!{/^%%$$/!p;};}' c_parser.y > c_rules.txt
	sed '/NOW FOLLOWS THE COMPLETE C GRAMMAR/q' yacc_parser.y > yacc_parser.head
	sed '1,/^%%$$/d' yacc_parser.y | sed -n '/^%%$$/,$$p' > yacc_parser.tail
	cat yacc_parser.head c_rules.txt yacc_parser.tail > yacc_parser.y
	rm  yacc_parser.head c_rules.txt yacc_parser.tail

.INTERMEDIATE: cppexp_parser.tab.intermediate
cppexp_parser.tab.c cppexp_parser.tab.h : cppexp_parser.tab.intermediate ;
cppexp_parser.tab.intermediate : cppexp_parser.y $(YACC)
	$(YACC) -v -b cppexp_parser -p cppexp_yy cppexp_parser.y
	sed -E '/^$$/q' cppexp_parser.output > cppexp_parser.rules

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
$(OBJDIR)/c_parser.tab.o : c_parser.tokens lexem.h
$(OBJDIR)/yacc_parser.tab.o : yacc_parser.tokens lexem.h

# All object files depend on the generated lexem.h (which is generated from c_parser.tab.h)
# This ensures that when enum values change, all files that use them are recompiled
$(OBJS): lexem.h

# And compare the three token sections to ensure that they are exactly the same
.PHONY:check-tokens
check-tokens: c_parser.tokens yacc_parser.tokens
	@echo Checking tokens across parsers ...
	@if ! diff -q c_parser.tokens yacc_parser.tokens ; then \
		echo "c_parser.tokens:1:1: ERROR: Parser token section must be identical:" ; \
		diff -c c_parser.tokens yacc_parser.tokens ; \
	fi

# Create an enum of the %token define's in c_parser.tab.h so that
# we don't need to use c_parser.tab.h to define tokens
lexem.h: lexem.h.head lexem.h.tail c_parser.tab.h character_lexems.txt Makefile
	# Convert the non-character tokens to defines
	@# Standard BYACC (which does not work yet) defines YYSTYPE_IS_DECLARED in .tab.h which we do not want
	@grep define c_parser.tab.h | sed -e "s/#define \(.*\) \([0-9]*\)/\1 = \2,/" | grep -v YYSTYPE_IS_DECLARED > lexem.h.tab
	# Combine the parts into a temporary file
	@cat lexem.h.head character_lexems.txt lexem.h.tab lexem.h.tail > lexem.h.tmp
	# Sanity check for content, in case yacc errored out
	@if grep " = " lexem.h.tmp > /dev/null ; then \
		cmp lexem.h.tmp lexem.h || mv lexem.h.tmp lexem.h && rm lexem.h.tmp ; \
	else \
		rm lexem.h.tmp ; \
	fi

lexem.c: lexem.h Makefile
	# Generate an init function for lexem names from lexem.h
	@echo "#include \"lexem.h\"" > lexem.c.tmp
	@echo >> lexem.c.tmp
	@echo "char *lexemEnumNames[LAST_TOKEN];" >> lexem.c.tmp
	@echo >> lexem.c.tmp
	@echo "void initLexemEnumNames(void) {" >> lexem.c.tmp
	@grep = lexem.h | grep -v LAST_TOKEN | awk '{ printf "    lexemEnumNames[%s] = \"%s\";\n", $$3, $$1}' | sed 's/,]/]/g' >> lexem.c.tmp
	@echo "}" >> lexem.c.tmp
	@if ! cmp lexem.c lexem.c.tmp ; then \
		mv lexem.c.tmp lexem.c ; \
	else \
		rm lexem.c.tmp ; \
	fi


#########################################################################
.PHONY:clean
clean:
	@-$(MAKE) -C ../tests clean & \
	rm -rf $(OBJDIR) c-xref *.gcov *.$(UNITTEST_EXT) & \
	wait

########################################################################

.PHONY:clean-coverage
clean-coverage:
ifeq ($(HAVE_APPLE_COV),yes)
	# Darwin: Remove .profraw and .profdata files
	@-find .. -name "*.profraw" -delete
	@-find .. -name "*.profdata" -delete
	# Generate empty .gcov files to show zero coverage
	@for src_file in *.c; do \
		if [ -f "$$src_file" ]; then \
			echo "        -:    0:Source:$$src_file" > "$$src_file.gcov" ; \
			echo "        -:    0:Graph:none" >> "$$src_file.gcov" ; \
			echo "        -:    0:Data:none" >> "$$src_file.gcov" ; \
			echo "        -:    0:Runs:0" >> "$$src_file.gcov" ; \
			awk '{ printf "        #:    %d:%s\n", NR, $$0 }' "$$src_file" >> "$$src_file.gcov" ; \
		fi ; \
	done
else
	# Non-Darwin: Use lcov to zero counters
	@-if command -v $(LCOV) > /dev/null 2>&1 ; then \
		$(LCOV) -q -z -d .. ; \
	fi
	$(MAKE) gcov
endif

.PHONY:gcov
# Create .gcov files for Emacs cov-mode
gcov:
ifeq ($(HAVE_APPLE_COV),yes)
	# Darwin: Convert llvm-cov output to traditional .gcov format for Emacs cov-mode
	@if [ -f ../coverage/merged.profdata ]; then \
		echo "Generating .gcov files..." ; \
		for src_file in *.c; do \
			case "$$src_file" in \
				*_tests.c) \
					test_dylib="$${src_file%%.c}.dylib" ; \
					if [ -f "$$test_dylib" ]; then \
						./llvm2gcov_test.sh "$$src_file" "$$test_dylib" > "$$src_file.gcov" 2>/dev/null || true ; \
					fi ; \
					;; \
				*) \
					if [ -f "$$src_file" ]; then \
						./llvm2gcov.sh "$$src_file" > "$$src_file.gcov" 2>/dev/null || true ; \
					fi ; \
					;; \
			esac ; \
		done ; \
		echo "Darwin: Generated .gcov files for Emacs cov-mode using llvm-cov conversion" ; \
	else \
		echo "Darwin: No coverage data found - run 'make coverage' first" ; \
	fi
else
	# Non-Darwin: Create .gcov files from .gcda in $(OBJDIR)
	-$(GCOV) $(OBJDIR)/*.o --object-directory $(OBJDIR) 2>&1 >/dev/null | grep -v "assuming not executed" || true
endif

.PHONY:coverage
coverage:
	$(MAKE) -e merge-coverage-from-tests


.PHONY:merge-coverage-from-tests
merge-coverage-from-tests:
ifeq ($(HAVE_APPLE_COV),yes)
	# Darwin: Merge all .profraw files from tests using llvm-profdata
	@mkdir -p ../coverage
	@-if find .. -name "*.profraw" | head -1 >/dev/null 2>&1; then \
		$(LLVM_PROFDATA) merge -sparse `find .. -name "*.profraw"` -o ../coverage/merged.profdata ; \
	fi
	$(MAKE) -e gcov
else
ifeq ($(HAVE_GCOV_TOOL),yes)
	# Merge all gcda from all tests then make gcovs from those
	@-for tc in ../tests/*/.cov ; do $(GCOV_TOOL) merge $$tc $(OBJDIR) -o $(OBJDIR) ; done
	$(MAKE) -e gcov
else
	# Merge all .info to total.info then extract .gcovs from it
	$(MAKE) -e total-coverage
	$(MAKE) -e info2gcov
endif
endif


LCOV_IGNORE_COVERAGE_FOR = 'log.c' '*.mock' '*.tab.c' '/usr/*' '/Library/*' '/Applications/*'

SYSTEM_TEST_COVERAGE_FILES = `ls ../tests/*/.cov/coverage.info | xargs printf -- '-a %s\n'`

.PHONY:total-coverage
total-coverage:
	@mkdir -p ../coverage
ifeq ($(HAVE_APPLE_COV), yes)
		# Darwin: Export merged profdata to LCOV format for genhtml compatibility
		@if [ -f ../coverage/merged.profdata ]; then \
			$(LLVM_COV) export --format=lcov --instr-profile=../coverage/merged.profdata c-xref --sources . > ../coverage/total.info ; \
		else \
			if ls ../tests/*/.cov/*.profraw >/dev/null 2>&1; then \
				$(LLVM_PROFDATA) merge -sparse ../tests/*/.cov/*.profraw -o ../coverage/merged.profdata ; \
				$(LLVM_COV) export --format=lcov --instr-profile=../coverage/merged.profdata c-xref --sources . > ../coverage/total.info ; \
			fi ; \
		fi
else
		# Non-Darwin: Use traditional lcov approach
ifeq ($(HAVE_GCOV_TOOL), yes)
			# Then we have already merged all test .gcda into OBJDIR and can just generate the total.info from .
			$(LCOV) -d . -c -q -o ../coverage/total.info 2>&1 | grep -v "did not produce any data" | grep -v "assuming not executed" || true
else
			# The run_test script in ../tests has created a coverage.info in each test directory
			# So we can add each of the tests coverage.info to ../coverage/total.info using '-a ...'
			# with lcov
			$(LCOV) $(SYSTEM_TEST_COVERAGE_FILES) -q -o ../coverage/total.info 2>&1
endif
		# @echo "LCOV-ing done..."
		# Then we can remove coverage for the ignored files
		$(LCOV) -q --remove ../coverage/total.info $(LCOV_IGNORE_COVERAGE_FOR) -o ../coverage/total.info
		# LCOV --remove done...
endif

.PHONY:info2gcov
info2gcov:
ifeq ($(HAVE_GCOV_TOOL),no)
  # Then we have to wait until now to generate the gcov's for Emacs cov-mode...
  # And we need the info2gcov command (https://github.com/thoni56/info2gcov)
  ifeq ($(HAVE_INFO2GCOV),yes)
	info2gcov -q ../coverage/total.info
  else
    # We have to do without the .gcov files, no Emacs coverage display sadly...
  endif
endif

.PHONY:coverage-report
coverage-report: total-coverage
	@$(GENHTML) -q -o ../coverage/$(COVERAGE_CASE) ../coverage/total.info
	@echo Coverage done: `grep -m 1 coverPer ../coverage/index.html | tr -dc '0-9.'`

.PHONY:cloc
cloc:
ifneq ($(shell command -v cloc 2> /dev/null),)
	@cloc --quiet *.[chy] *.t? | grep SUM: | awk '{print "Lines:", $$5}'
endif

#########################################################################
.PHONY:watch
watch:
	watchexec --no-vcs-ignore -i \*.tab.\[ch\] -i lexem.h -e c,h,y,mock,tc,th -- execnotify -t Development make -j$(JOBS) devel

.PHONY: watch-for-unittests
watch-for-unittests:
	watchexec --no-vcs-ignore -i yacc_parser.y -i \*.tab.\[ch\] -i lexem.h -e c,h,y,mock,tc,th -- execnotify -t Unittests make -j$(JOBS) unit


.PHONY: watch-for-unittests-silent
watch-for-unittests-silent:
	watchexec --no-vcs-ignore -i yacc_parser.y -i \*.tab.\[ch\] -i lexem.h -e c,h,y,mock,tc,th -- make -j$(JOBS) unit

.PHONY: watch-for-build-only
watch-for-build-only:
	watchexec --no-vcs-ignore -f '*/.unittests.done' -- execnotify -t Build make -j$(JOBS) c-xref

.PHONY: watch-for-systemtests-only
watch-for-systemtests-only:
	watchexec --no-vcs-ignore -f '*/.build.done' -- execnotify -t Systemtests make -j$(JOBS) system-tests-proper

.PHONY: watch-for-systemtests
watch-for-systemtests:
	watchexec --no-vcs-ignore -f '*/.unittests.done' -- make -j system-tests

.PHONY: watch-for-systemtests-silent
watch-for-systemtests-silent:
	watchexec --no-vcs-ignore -f '*/.unittests.done' -- make -j$(JOBS) system-tests-proper

.PHONY: system-tests-proper
system-tests-proper:
	$(MAKE) -e test
	$(MAKE) -e merge-coverage-from-tests
	@$(MAKE) -e coverage-report

.PHONY: system-tests
system-tests: c-xref
	$(MAKE) system-tests-proper

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist

$(YACC): $(ROOTDIR)/byacc-1.9/*.[ch]
	$(MAKE) -C $(ROOTDIR)/byacc-1.9


# Find all *_tests.c automatically
UNITTESTS = $(patsubst %.c,%.$(UNITTEST_EXT),$(wildcard *_tests.c))

OS := $(shell uname)
ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
    OS := Cygwin
	UNITTEST_EXT=dll
endif
ifeq ($(OS),Darwin)
	UNITTEST_EXT=dylib
else
	UNITTEST_EXT=so
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
		# And for Homebrew on arm we just build for that
		CGREEN_LIB = -L/opt/homebrew/lib -lcgreen
		DYLD_PATH = DYLD_LIBRARY_PATH=/opt/homebrew/lib
		#CFLAGS += -arch arm64
	else
		CGREEN_LIB = -L/usr/local/lib -lcgreen
		DYLD_PATH = DYLD_LIBRARY_PATH=/usr/local/lib
	endif
endif

ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
	CGREEN_LIB = -L/usr/local/lib -lcgreen
endif


.PHONY:unittests
ifeq ($(shell command -v cgreen-runner 2> /dev/null),)
unittests: parsers
	@echo Cgreen not available, cannot run unittests
else
unittests: parsers $(OBJS) $(UNITTESTS)
	$(MAKE) -e clean-coverage
ifeq ("$(HAVE_APPLE_COV)", yes)
		# Darwin: Set LLVM_PROFILE_FILE to enable coverage data collection
		LLVM_PROFILE_FILE="unittests-%p.profraw" $(DYLD_PATH) cgreen-runner -q $(CGREEN_OUTPUT) --suite unittests $(UNITTESTS)
		# Darwin: Merge coverage data from unit tests, then generate gcov files
		$(MAKE) -e merge-coverage-from-tests
else
		# Non-Darwin: Just run tests and generate gcov files directly
		$(DYLD_PATH) cgreen-runner -q $(CGREEN_OUTPUT) --suite unittests $(UNITTESTS)
		$(MAKE) gcov
endif
	@touch .unittests.done
endif

# A test lib for a module is built from its .o and the _test.o (and some extras)
#%_tests.$(UNITTEST_EXT): %.o %_tests.o $(EXTRA_OBJS)
#	$(CC) -shared -o $@ $(sort $(EXTRA_OBJS) $^) $(LDLIBS) -lcgreen


# Macro for unittests that link the SUT and a small number of dependent .o
# Parameters: <module>, <dependent modules ...>
define UNITTEST
$(1)_DEPENDENCIES = $(2)
$(1)_DEPS = $$(patsubst %,$(OBJDIR)/%.o,$$($(1)_DEPENDENCIES))
$(OBJDIR)/$(1)_tests.o: CFLAGS += $(CGREEN_INCLUDE)
$(1)_tests.$(UNITTEST_EXT): $(OBJDIR)/$(1)_tests.o $(OBJDIR)/$(1).o $$($(1)_DEPS)
	$(CC) $(LDFLAGS) -shared -o $$@ $$^ $$(COVERAGE) $$(CGREEN_LIB) $(LIBS)
endef


$(eval $(call UNITTEST,browsermenu,log protocol type position memory stackmemory usage))
$(eval $(call UNITTEST,characterreader,log))
$(eval $(call UNITTEST,commandlogger))
$(eval $(call UNITTEST,commons,log protocol stringlist))
$(eval $(call UNITTEST,complete,log protocol type memory stackmemory position))
$(eval $(call UNITTEST,completion,log memory usage position))
$(eval $(call UNITTEST,counters))
$(eval $(call UNITTEST,cxfile,log memory usage protocol position hash stackmemory))
$(eval $(call UNITTEST,cxref,server protocol log hash position type storage usage memory stackmemory position))
$(eval $(call UNITTEST,editor,position usage log memory stackmemory hash editormarker))
$(eval $(call UNITTEST,editorbuffer,position log memory stackmemory))
$(eval $(call UNITTEST,editorbuffertable,position log memory hash))
$(eval $(call UNITTEST,editormarker,log position stackmemory))
$(eval $(call UNITTEST,encoding,))
$(eval $(call UNITTEST,extract,position log memory stackmemory storage type id usage protocol))
$(eval $(call UNITTEST,fileio,log))
$(eval $(call UNITTEST,filetable,log memory stackmemory hash))
$(eval $(call UNITTEST,id,log memory stackmemory))
$(eval $(call UNITTEST,init,log position stackmemory storage))
$(eval $(call UNITTEST,lexemstream, lexem lexembuffer log))
$(eval $(call UNITTEST,lexembuffer,characterreader position memory stackmemory log))
$(eval $(call UNITTEST,lexer,log lexembuffer characterreader position memory stackmemory))
$(eval $(call UNITTEST,list,log))
$(eval $(call UNITTEST,lsp,log json_utils))
$(eval $(call UNITTEST,lsp_adapter,log json_utils lsp_utils position stackmemory))
$(eval $(call UNITTEST,lsp_utils,log))
$(eval $(call UNITTEST,lsp_dispatcher,log json_utils))
$(eval $(call UNITTEST,lsp_handler,log json_utils))
$(eval $(call UNITTEST,macroargumenttable,memory log hash))
$(eval $(call UNITTEST,match))
$(eval $(call UNITTEST,memory))
$(eval $(call UNITTEST,misc,protocol log type memory stackmemory))
$(eval $(call UNITTEST,options,protocol log position memory stackmemory))
$(eval $(call UNITTEST,parsers,log))
$(eval $(call UNITTEST,position,log))
$(eval $(call UNITTEST,progress,protocol log))
$(eval $(call UNITTEST,refactorings,log))
$(eval $(call UNITTEST,refactory,protocol usage position log memory stackmemory))
$(eval $(call UNITTEST,reference,log usage memory position stackmemory))
$(eval $(call UNITTEST,referenceableitem,log))
$(eval $(call UNITTEST,referenceableitemtable,log memory hash))
$(eval $(call UNITTEST,search,log session position stackmemory protocol))
$(eval $(call UNITTEST,semact,memory stackmemory log protocol usage hash position storage type id))
$(eval $(call UNITTEST,server,memory log position stackmemory))
$(eval $(call UNITTEST,session,log position stackmemory))
$(eval $(call UNITTEST,stackmemory,memory log))
$(eval $(call UNITTEST,symbol,memory stackmemory log))
$(eval $(call UNITTEST,symboltable,hash memory stackmemory))
$(eval $(call UNITTEST,reference_database,log position memory stackmemory globals usage))
$(eval $(call UNITTEST,undo,memory log))
$(eval $(call UNITTEST,xref,memory log protocol))
$(eval $(call UNITTEST,yylex,memory stackmemory protocol log position filedescriptor id filetable hash symboltable macroargumenttable symbol lexembuffer stringlist lexemstream))

variables:
ifeq ($(HAVE_APPLE_COV),yes)
	@echo "=== Darwin Coverage Variables ==="
	@echo HAVE_APPLE_COV = $(HAVE_APPLE_COV)
	@echo LLVM_PROFDATA = $(LLVM_PROFDATA)
	@echo LLVM_COV = $(LLVM_COV)
	@echo COVERAGE = $(COVERAGE)
else
	@echo "=== Traditional Coverage Variables ==="
	@echo HAVE_LCOV2 = $(HAVE_LCOV2)
	@echo LCOV = $(LCOV)
	@echo GCOV = $(GCOV)
	@echo HAVE_GCOV_TOOL = $(HAVE_GCOV_TOOL)
	@echo GCOV_TOOL = $(GCOV_TOOL)
	@echo HAVE_INFO2GCOV = $(HAVE_INFO2GCOV)
	@echo SYSTEM_TEST_COVERAGE_FILES = $(SYSTEM_TEST_COVERAGE_FILES)
endif

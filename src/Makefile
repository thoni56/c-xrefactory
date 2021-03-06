#########################################################################
# Always export following variables to environment of sub-makes (to use you need "-e")

export CC = gcc
export COVERAGE =

# Other variables

WARNINGS = -Wall -Wno-char-subscripts

CFLAGS = -g $(INCLUDES) -MMD -funsigned-char $(WARNINGS) $(COVERAGE)

LDFLAGS = $(COVERAGE)

ROOTDIR=..

# We need our own patched yacc
YACC=$(ROOTDIR)/byacc-1.9/yacc

# If there is no ZLIB on the system you can build it yourself, see sources.mk

# On Darwin we want to use gcc-11 if available
OS := $(shell uname)
ifeq ($(OS),Darwin)
	ifneq (, $(shell which gcc-11))
		CC=gcc-11
	endif
endif

# We need to use compatible gcov (Darwin/Brew now have GCC v10)
ifeq ("$(CC)", "gcc-11")
	LCOV = lcov --gcov-tool gcov-11
else
	LCOV = lcov
endif

#########################################################################
#
# The heavy lifting is here

# For production - no smartness just compile
prod: c-xref

yydebug: CFLAGS += -DYYDEBUG
yydebug: build

devel: COVERAGE=--coverage
devel: clean-coverage check-tokens
	$(MAKE) -e devel-proper

devel-proper: unit build test
	$(MAKE) coverage-report
	$(MAKE) -C ../editors/emacs

coverage: COVERAGE=--coverage
coverage: ci

ci: COVERAGE=--coverage
ci:
	$(MAKE) -e unit
	$(MAKE) -e build
	$(MAKE) -C ../tests/systemd init
	$(MAKE) -C ../tests/ffmpeg init
	$(MAKE) -e -C ../tests all

chcheck:
	for m in `echo $(MODULES) | sed -e 's/[a-z]*_parser\.tab//g'` ; do \
		chcheck $$m ; \
	done

# Be silent unless VERBOSE is set to something (VERBOSE=1)
#$(VERBOSE).SILENT:

#########################################################################
include sources.mk

.PHONY: build
build: check-tokens # check that token sections in parsers are identical before compiling
	@$(MAKE) -e c-xref

c-xref: CFLAGS+=-Og
.PHONY: c-xref
c-xref: $(OBJS) $(OPTIONAL_ZLIB_LIB)
	$(CC) $(LDFLAGS) -o c-xref $(OBJS) $(LIBS)

#########################################################################
test: c-xref
	@echo "Only running quick tests for now - 'cd ../test; make all' to run all"
	$(MAKE) -e -C ../tests quick

unit: check-tokens $(OBJS)
	$(MAKE) -e -C unittests

#########################################################################
# We generate parsers for C, Yacc, Java and C expressions
# Note the file prefix (-b) and symbol prefix (-p)
# Also create .rules files with only the rules in it

# Makefile magic to make two targets with one recepie, thanks to
# https://stackoverflow.com/a/10609434/204658

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

.INTERMEDIATE: java_parser.tab.intermediate
java_parser.tab.c java_parser.tab.h : java_parser.tab.intermediate ;
java_parser.tab.intermediate : java_parser.y $(YACC)
	$(YACC) -v -b java_parser -p java_yy java_parser.y
	sed -E '/^$$/q' java_parser.output > java_parser.rules


# There is no include feature in grammars so we need duplicate
# token definitons. Here we ensure they are in sync
# Extract all three token sections
EXTRACT_TOKEN_DEFINITIONS = sed -E '/END OF COMMON TOKEN DEFINITIONS/,$$d' | sed -E '/START OF COMMON TOKEN DEFINITIONS/,$$!d'
c_parser.tokens: c_parser.y
	cat c_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > c_parser.tokens

yacc_parser.tokens: yacc_parser.y
	cat yacc_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > yacc_parser.tokens

java_parser.tokens: java_parser.y
	cat java_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > java_parser.tokens

# Compare the three token sections to ensure that they are exactly the same
.PHONY: check-tokens
check-tokens: c_parser.tokens yacc_parser.tokens java_parser.tokens
	@if ! diff -q c_parser.tokens yacc_parser.tokens ; then \
		echo "ERROR: Parser token section must be identical:" ; \
		diff -c c_parser.tokens yacc_parser.tokens ; \
	fi
	@if ! diff -q yacc_parser.tokens java_parser.tokens ; then \
		echo "ERROR: Parser token section must be identical:" ; \
		diff -c yacc_parser.tokens java_parser.tokens ; \
	fi
	@if ! diff -q java_parser.tokens c_parser.tokens ; then \
		echo "ERROR: Parser token section must be identical:" ; \
		diff -c java_parser.tokens c_parser.tokens ; \
	fi

# Create an enum of the %token define's in c_parser.tab.h so that
# we don't need to use c_parser.tab.h to define tokens
lexem.h: c_parser.tab.h
	echo "#ifndef LEXEM_H_INCLUDED" > lexem.h.tmp
	echo "#define LEXEM_H_INCLUDED" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "/**** DO NOT EDIT - generated from c_parser.tab.h by Makefile ****/" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "typedef enum lexem {" >> lexem.h.tmp
	grep define c_parser.tab.h | sed -e "s/#define \(.*\) \([0-9]*\)/\1 = \2,/" >> lexem.h.tmp
	echo "} Lexem;" >> lexem.h.tmp
	echo >> lexem.h.tmp
	echo "#endif" >> lexem.h.tmp
	# Sanity check for content, otherwise yacc errored out probably
	if grep " = " lexem.h.tmp > /dev/null ; then \
		cp lexem.h.tmp lexem.h ; \
	fi


#########################################################################

clean:
	-$(MAKE) -C unittests clean
	-$(MAKE) -C ../tests clean
	-rm -rf $(OBJDIR) c-xref *.gcov

########################################################################

.PHONY: clean-coverage
clean-coverage:
	echo coverage=$(COVERAGE)
	-if command -v lcov > /dev/null 2>&1 ; then \
		lcov -q -z -d .. ; \
	fi

coverage-report:
	@echo Doing the coverage reporting in the background...
	$(MAKE) the-actual-coverage-report &

LCOV_IGNORE_COVERAGE_FOR = 'log.c' '*.mock' '*.tab.c' '/usr/*' '/Library/*'
# NOTE: lcov < 1.14 don't have --exclude, need to use separate --remove step for compatibility
ifeq ($(findstring 1.13, $(shell lcov -v)), 1.13)
LCOV_EXCLUDES = `echo $(LCOV_IGNORE_COVERAGE_FOR) | xargs printf -- '--exclude %s\n'`
endif

the-actual-coverage-report:
	@mkdir -p ../coverage
	@# Each test has created its own .info file in ../test, combine them
	ls ../tests/coverage/*.info | xargs printf -- '-a %s\n' | xargs lcov -q -o ../tests/tests.info
	@# Add the unittest coverage info and exclude modules we want to ignore (lcov >= 1.14)
	lcov -a ../tests/tests.info -a unittests/unittests.info $(LCOV_EXCLUDES) -o ../coverage/total.info
	@# Re-create collated gcov files from total.info for Emacs gcov-mode
	if command -v info2gcov > /dev/null 2>&1 ; then \
		info2gcov -q ../coverage/total.info ; \
	fi
	@printf "\\r"
ifeq (, $(LCOV_EXCLUDES))
	lcov -q --remove ../coverage/total.info $(LCOV_IGNORE_COVERAGE_FOR) -o ../coverage/total.info
endif
	genhtml -q -o ../coverage ../coverage/total.info
	@echo Coverage done

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist

$(YACC): $(ROOTDIR)/byacc-1.9/*.[ch]
	$(MAKE) -C $(ROOTDIR)/byacc-1.9

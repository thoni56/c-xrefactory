#########################################################################
CC = gcc

WARNINGS = -Wall -Wno-char-subscripts

CFLAGS = -g $(INCLUDES) -MMD -funsigned-char $(WARNINGS) $(COVERAGE)

LDFLAGS = $(COVERAGE)

ROOTDIR=..

# We need our own patched yacc
YACC=$(ROOTDIR)/byacc-1.9/yacc

# If there is no ZLIB on the system you can build it yourself, see sources.mk

#########################################################################
#
# The heavy lifting is here

debug: CFLAGS+=-DDEBUG -O0
devel: COVERAGE=--coverage
devel: CLEAN_COVERAGE=yes
yydebug: CFLAGS+= -DDEBUG -DYYDEBUG -Og

prod debug yydebug: $(ZLIB) c-xref
devel:
	make clean-coverage
	make -C ../editors/emacs
	make -j unit
	make -j debug
	make -j test
	make coverage-report

ci: COVERAGE=--coverage
ci:
	make -j unit
	make -j debug
	make -j test

chcheck:
	@for m in `echo $(MODULES) | sed -e 's/[a-z]*_parser\.tab//g'` ; do \
		chcheck $$m ; \
	done

#########################################################################
include sources.mk

c-xref: .c-xrefrc $(OBJS) $(OPTIONAL_ZLIB_LIB)
	$(CC) $(LDFLAGS) -o c-xref $(OBJS) $(LIBS)
	@make check-tokens			# check that token sections in parsers are identical

.PHONY: clean-coverage
clean-coverage:
	@if [ "$(CLEAN_COVERAGE)" = "yes" ] ; then \
		lcov -q -z -d $(OBJDIR) ; \
	fi

#########################################################################
# We generate parsers for C, Yacc, Java and C expressions
# Note the file prefix (-b) and symbol prefix (-p)
# Also create a .rules file with only the rules in it and files with
# only C-rules from C and Yacc parsers to be able to compare them

c_parser.tab.c c_parser.tab.h: c_parser.y $(YACC)
	$(YACC) -v -d -b c_parser -p c_yy c_parser.y
	@sed -E '/^$$/q' c_parser.output > c_parser.rules
	@awk '{$$1=""; print}' c_parser.rules > c_parser.tmp
	@sed -n '/primary_expr/,$$p' c_parser.tmp > c_rules_from_c_parser.txt

cexp_parser.tab.c: cexp_parser.y $(YACC)
	$(YACC) -v -b cexp_parser -p cexp_yy cexp_parser.y
	@sed -E '/^$$/q' cexp_parser.output > cexp_parser.rules

yacc_parser.tab.c: yacc_parser.y $(YACC)
	$(YACC) -v -b yacc_parser -p yacc_yy yacc_parser.y
	@sed -E '/^$$/q' yacc_parser.output > yacc_parser.rules
	@awk '{$$1=""; print}' yacc_parser.rules > yacc_parser.tmp
	@sed -n '/primary_expr/,$$p' yacc_parser.tmp > c_rules_from_yacc_parser.txt

java_parser.tab.c: java_parser.y $(YACC)
	$(YACC) -v -b java_parser -p java_yy java_parser.y
	@sed -E '/^$$/q' java_parser.output > java_parser.rules


# There is no include feature in grammars so we need duplicate
# token definitons. Here we ensure they are in sync
# Extract all three token sections
EXTRACT_TOKEN_DEFINITIONS = sed -E '/END OF COMMON TOKEN DEFINITIONS/,$$d' | sed -E '/START OF COMMON TOKEN DEFINITIONS/,$$!d'
c_parser.tokens: c_parser.y
	@cat c_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > c_parser.tokens

yacc_parser.tokens: yacc_parser.y
	@cat yacc_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > yacc_parser.tokens

java_parser.tokens: java_parser.y
	@cat java_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > java_parser.tokens

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
	@echo "#ifndef _LEXEM_H_" > lexem.h
	@echo "#define _LEXEM_H_" >> lexem.h
	@echo >> lexem.h
	@echo "/**** DO NOT EDIT - generated from c_parser.tab.h by Makefile ****/" >> lexem.h
	@echo >> lexem.h
	@echo "typedef enum lexem {" >> lexem.h
	@grep define c_parser.tab.h | sed -e "s/#define \(.*\) \([0-9]*\)/\t\1 = \2,/" >> lexem.h
	@echo "} Lexem;" >> lexem.h
	@echo >> lexem.h
	@echo "#endif" >> lexem.h

.c-xrefrc: c-xrefrc.tpl
	@sed -e "s+CURDIR+$(CURDIR)+g" c-xrefrc.tpl > .c-xrefrc

#########################################################################

clean:
	-make -C unittests clean
	-make -C ../tests clean
	-rm -rf $(OBJDIR) c-xref *.gcov

########################################################################
test:
	@echo "Only running quick tests for now - 'cd ../test; make all' to run all"
	$(MAKE) -C ../tests -j quick


unit:
	make -C unittests debug

LCOV=lcov -q --exclude log.c --exclude '*.mock' --exclude '*.tab.c' --exclude '/usr/*' --exclude '/Library/*'
coverage: COVERAGE=--coverage
coverage: clean test
	$(LCOV) -d . -c -o total.info
	genhtml -q -o ../coverage total.info

coverage-report:
	@echo Doing the coverage reporting in the background
	@$(MAKE) the-actual-coverage-report &

the-actual-coverage-report:
	@$(LCOV) -d . -c -t unittests -o ../coverage/unittests.info
	@ls -1q ../tests/*.info | sed "s/^/-a /g" | xargs $(LCOV) -o ../coverage/total.info -a ../coverage/unittests.info
	@printf "\\r"
	@lcov -q --remove ../coverage/total.info 'log.c' '*.tab.c' '/usr/*' '/Library/*' -o ../coverage/total.info
	@-[ -x "$$(command -v info2gcov)" ] && info2gcov -q ../coverage/total.info
	genhtml -q -o ../coverage ../coverage/total.info

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist and run it on
# possibly updated grammar files

$(YACC): $(ROOTDIR)/byacc-1.9/*.[ch]
	$(MAKE) -C $(ROOTDIR)/byacc-1.9

#########################################################################
CC = gcc

WARNINGS = -Wall -Wno-char-subscripts

CFLAGS = -g $(INCLUDES) -MMD -funsigned-char $(WARNINGS) $(COVERAGE)

LDFLAGS = $(COVERAGE)

ROOTDIR=..

# We need our own patched yacc
YACC=$(ROOTDIR)/byacc-1.9/yacc

# If there is no ZLIB on the system you can activate building it in sources.mk

#########################################################################
#
# The heavy lifting is here

debug: CFLAGS+=-DDEBUG -O0
debug yydebug: COVERAGE=--coverage
debug yydebug: CLEAN_COVERAGE=yes
yydebug: CFLAGS+= -DDEBUG -DYYDEBUG -Og

prod debug yydebug: $(ZLIB) c-xref
devel:
	make -j debug
	make -j unit
	make -j test
	make coverage-report

chcheck:
	@for m in `echo $(MODULES) | sed -e 's/[a-z]*_parser\.tab//g'` ; do \
		chcheck $$m ; \
	done

#########################################################################
include sources.mk

c-xref: .c-xrefrc $(OBJS) $(OPTIONAL_ZLIB_LIB) $(CLEAN_COVERAGE)
	@if [ "$(CLEAN_COVERAGE)" = "yes" ] ; then \
		lcov -q -z -d $(OBJDIR) ; \
	fi
	$(CC) $(LDFLAGS) -o c-xref $(OBJS) $(LIBS)
	@make check-tokens			# check that token sections in parsers are identical


#########################################################################
# We generate parsers for C, Yacc, Java and C expressions
# Note the file prefix (-b) and symbol prefix (-p)

c_parser.tab.c c_parser.tab.h: c_parser.y $(YACC)
	$(YACC) -v -d -b c_parser -p c_yy c_parser.y

java_parser.tab.c: java_parser.y $(YACC)
	$(YACC) -v -b java_parser -p java_yy java_parser.y

yacc_parser.tab.c: yacc_parser.y $(YACC)
	$(YACC) -v -b yacc_parser -p yacc_yy yacc_parser.y

cexp_parser.tab.c: cexp_parser.y $(YACC)
	$(YACC) -v -b cexp_parser -p cexp_yy cexp_parser.y

EXTRACT_TOKEN_DEFINITIONS = sed -E '/END OF COMMON TOKEN DEFINITIONS/,$$d' | sed -E '/START OF COMMON TOKEN DEFINITIONS/,$$!d'
c_parser.tokens: c_parser.y
	@cat c_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > c_parser.tokens

yacc_parser.tokens: yacc_parser.y
	@cat yacc_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > yacc_parser.tokens

java_parser.tokens: java_parser.y
	@cat java_parser.y | $(EXTRACT_TOKEN_DEFINITIONS) > java_parser.tokens

# Check that the token definitions in the three main grammars are exactly the same
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

# Create an enum of the %token define's in c_parser.tab.h
lexem.h: c_parser.tab.h
	@echo "typedef enum lexem {" > lexem.h
	@grep define c_parser.tab.h | sed -e "s/#define \(.*\) \([0-9]*\)/\t\1 = \2,/" >> lexem.h
	@echo "} Lexem;" >> lexem.h

.c-xrefrc: c-xrefrc.tpl
	@sed -e "s+CURDIR+$(CURDIR)+g" c-xrefrc.tpl > .c-xrefrc

#########################################################################

clean:
	-make -C $(ROOTDIR)/lib/zlib clean
	-make -C unittests clean
	-rm -rf $(OBJDIR) c-xref
	-make -C ../tests clean

########################################################################
test: debug
	@echo "Only running quick tests for now - 'cd ../test; make all' to run all"
	$(MAKE) -C ../tests -j quick


unit:
	make -C unittests debug

coverage: COVERAGE=--coverage
coverage: clean test
	lcov -q --exclude \*.mock -exclude /usr -exclude /Library -d . -c -o total.info
	genhtml -q -o ../coverage total.info

coverage-report: unit test
	@echo Doing the coverage reporting in the background
	@$(MAKE) the-actual-coverage-report &

LCOV=lcov -q --exclude '*.mock' --exclude '*.tab.c' --exclude '/usr/*' --exclude '/Library/*'
the-actual-coverage-report:
	@$(LCOV) -d . -c -t unittests -o ../coverage/unittests.info
	@ls -1q ../tests/*.info | sed "s/^/-a /g" | xargs $(LCOV) -o ../coverage/total.info -a ../coverage/unittests.info
	@printf "\\r"
	@lcov -q --remove ../coverage/total.info '*.tab.c' '/usr/*' '/Library/*' -o ../coverage/total.info
	@-[ -x "$$(command -v info2gcov)" ] && info2gcov -q ../coverage/total.info
	genhtml -q -o ../coverage ../coverage/total.info

#########################################################################
#
# Trigger building our patched yacc if it doesn't exist and run it on
# possibly updated grammar files

yacc: $(YACC)
	$(MAKE) YACCING=yes

$(YACC):
	$(MAKE) -C $(ROOTDIR)/byacc-1.9

# Every CU that can expand a macro gives its referent for a cursor in its body, however many
#
# `next` in node.h is a field of a struct of its own in each of 130 CUs, so the menu
# has 130 referents (ADR-0027), the same as for two. The second push finds the CUs
# current, so only the macro path parses them.

include ../Makefile.server

N ?= 130
CUS = $(shell seq -w 1 $(N))

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer cu*.c
	@for n in $(CUS) ; do printf '#include "node.h"\nstruct s%s { struct s%s *next; };\nvoid w%s(struct s%s *n) { n = NEXT(n); }\n' $$n $$n $$n $$n > cu$$n.c ; done
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@awk '/<resolution>/ { r++ } /<symbol line=/ { n[r]++ } END { print "referents:", n[1]+0, n[2]+0 }' output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

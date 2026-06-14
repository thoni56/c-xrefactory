# Regression test for a #pragma once bug (kept .suspended until fixed).
#
# Renaming a symbol whose declaration lives in a `#pragma once` header must
# reach that declaration. processPragmaDirective (yylex.c:1598) has a
# "consume the rest of the #pragma line" loop, `while (lexem != '\n')`, that
# over-consumes when a declaration immediately follows `#pragma once` on the
# next line: the directive's terminating newline is not presented as a '\n'
# lexem, so the loop runs on and swallows `extern int foo(int x);`. The
# declaration is never parsed -> no symbol -> no reference -> rename misses it.
# (A blank/comment line between the pragma and the declaration avoids it,
# which is why the original stale-header test only saw the bug intermittently.)
#
# The output is a one-line verdict: "reaches" iff the rename edits header.h.

include ../Makefile.server

$(TEST): .c-xrefrc
	@-rm -rf .c-xref
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@if grep -q 'header\.h' output.tmp; then \
	   echo 'rename reaches the declaration in the #pragma once header'; \
	 else \
	   echo 'BUG: rename misses the declaration in the #pragma once header'; \
	 fi > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

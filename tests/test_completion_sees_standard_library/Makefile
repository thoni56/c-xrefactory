# Completion sees the standard library
#
# sp completes to sprintf, declared in <stdio.h>, which is only possible if the
# compiler's include paths were discovered and the header was parsed. The prefix
# is chosen to have exactly one match in both glibc and macOS's libc, so the
# answer is the same on every platform.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

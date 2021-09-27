# Template for an edit_server test:
#
# 1. generate CXrefs if they don't exist by running c-xref
# 2. use ../../utils/server_driver to feed the edit_server what's in commands.input
# 3. normalize the output and compare
#
# N.B. You can create a good starting point for your commands.input by
# running emacs from ../sandboxed_emacs and use
# ../../utils/messages2commands.py to convert *Messages* buffer
# content to commands.input

include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST):
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "diff output expected" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: EXTRA=--extra "-trace"
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 15 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1` -ex 'br mainRefactory' -ex 'continue'

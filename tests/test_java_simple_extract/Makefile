# This should become the golden standard for creating an edit_server test:
#
# 1. generate CXrefs if they don't exist by running c-xref
# 2. use ../../utils/edit_server_driver to feed the edit_server what's in commands.input
# 3. normalize the output and compare
#
# N.B. You can create a good starting point for your commands.input by running emacs
# from ../sandboxed_emacs.

include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST):
	@-$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer > output.tmp
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

trace: CXREF+=-trace
trace: $(TEST)

clean:
	@-rm -rf .c-xrefrc CXrefs output.log output.tmp output buffer *~ log

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer 15 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1` -ex 'br mainRefactory' -ex 'continue'

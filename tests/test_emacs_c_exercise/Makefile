# Automated tests for emacs_c_exercise
#
# Re: GCOV_ERROR_FILE=gcov.err
# Since emacs executes multiple c-xref's (one for edit-server, one
# for refactorings) gcov data collection will report an error. This
# error, "Merge mismatch for function..." ends up in the process-to
# process communication which sometimes happens in the middle of a
# sync, which is then interpreted as a "SYNCHRO PROBLEM" causing
# c-xref.el to "(error...". The above environment variable ensures
# that any gcov error messages are sent to a file instead. We
# actually don't care about these errors, but perhaps it makes this
# test unstable... (This might be the cause for inconsistent coverage
# reports, sometimes some executions are missed because of this
# spurrious errors...)

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@$(MAKE) --no-print-directory execute
	@$(MAKE) --no-print-directory check

setup:
	@# Copy clean set of c-example files here...
	@rm -rf cexercise
	@cp -r ../../doc/cexercise .
	@if [ ! -e .emacs.d/el-get/c-xrefactory ] ; then \
		ln -s $(CURDIR)/../.. .emacs.d/el-get/c-xrefactory ; \
	fi
	@# Ensure .c-xrefrc includes a project pointing to cexercise directory
	@grep cexercise .c-xrefrc > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		sed -e s+$(TEST)$$+$(TEST)/cexercise+g -e s+$(TEST)]$$+$(TEST)/cexercise]+g .c-xrefrc > .c-xrefrc.new ; \
		mv .c-xrefrc.new .c-xrefrc ; \
	fi
	@# Generate refs
	@../../src/c-xref cexercise -xrefrc .c-xrefrc -refs $(CURDIR)/CXrefs -o $(TEST).log

EMACS = emacs --geometry 80x40

# emacs = startup emacs with correct settings
# exec = also run the script for the exercise
# execute = do that in batch (no window) mode
execute: EMACS_STARTUP=-batch
execute: EMACS_RUN=-l emacs_c_exercise.el -l emacs_kill.el
exec: EMACS_RUN=-l emacs_c_exercise.el
execute exec emacs: setup
	@# Run emacs with commands to execute the example sequence
	@yes | GCOV_ERROR_FILE=gcov.err HOME=$(CURDIR) $(EMACS) --no-splash $(EMACS_STARTUP) -l .emacs.d/init.el -l .emacs.d/override-el-get-update.el $(EMACS_RUN) > /dev/null 2>&1
	@if [ -f cexercise/refactorings/gcov.err ] ; then \
		cat cexercise/refactorings/gcov.err ; \
		rm cexercise/refactorings/gcov.err ; \
	fi
	@# Remove tilde-files after editing
	@find cexercise -name \*~ -exec rm {} \;

check:
	@# Compare output to expected resulting files
	@diff -B -n -r cexercise expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		diff -r cexercise expected ; \
		exit 1 ; \
	fi
	@$(OK)
	@exit 0

clean:
	@rm -rf $@.log CXrefs .emacs.d/el-get/c-xrefactory .c-xrefrc

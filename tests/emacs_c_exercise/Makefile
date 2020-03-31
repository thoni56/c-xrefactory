# Automated tests for emacs_c_exercise

include ../Makefile.boilerplate

$(TEST):
	@$(MAKE) --no-print-directory setup
	@$(MAKE) --no-print-directory execute
	@$(MAKE) --no-print-directory check

setup: clean
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


# emacs = startup emacs with correct settings
# exec = also run the script for the exercise
# execute = do that in batch (no window) mode
execute: EMACS_STARTUP=-batch
execute: EMACS_RUN=-l emacs_c_exercise.el
exec: EMACS_RUN=-l emacs_c_exercise.el
execute exec emacs: setup
	@# Run emacs with commands to execute the example sequence
	@HOME=$(CURDIR) emacs --no-splash $(EMACS_STARTUP) -l .emacs.d/init.el -l .emacs.d/override-el-get-update.el $(EMACS_RUN) > /dev/null 2>&1
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
	@rm -rf $@.log CXrefs .emacs.d/el-get/c-xrefactory
	@# Copy c-example files here...
	@rm -rf cexercise
	@cp -r ../../doc/cexercise .

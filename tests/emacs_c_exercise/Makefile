# Automated tests for emacs_c_exercise

include ../Makefile.boilerplate

$(TEST):
	@printf "Tests in \"$@\": "
	@$(MAKE) --no-print-directory setup
	@$(MAKE) --no-print-directory execute
	@$(MAKE) --no-print-directory check

setup:
	@rm -rf $@.log CXrefs
	@# Copy c-example files here...
	@rm -rf cexercise
	@cp -r ../../doc/cexercise .
	@# Ensure .c-xrefrc includes a project pointing to cexercise directory
	@grep cexercise .c-xrefrc > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		sed -e s+$(TEST)$$+$(TEST)/cexercise+g -e s+$(TEST)]$$+$(TEST)/cexercise]+g .c-xrefrc > .c-xrefrc.new ; \
		mv .c-xrefrc.new .c-xrefrc ; \
	fi
	@# Generate refs
	@../../src/c-xref cexercise -xrefrc .c-xrefrc -refs $(CURDIR)/CXrefs -o $(TEST).log

execute:
	@# Run emacs with commands to execute the example sequence
	@HOME=$(CURDIR) emacs --no-splash -batch -l .emacs.el -l emacs_c_exercise.el > /dev/null 2>&1
	@# Remove tilde-files after editing
	@find cexercise -name \*~ -exec rm {} \;

check:
	@# Compare output to expected resulting files
	@diff -r cexercise expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0

clean:
	@rm -rf $@.log CXrefs
	@# Copy c-example files here...
	@rm -rf cexercise
	@cp -r ../../doc/cexercise .

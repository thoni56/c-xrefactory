# Building here only generates c-xrefprotocol.el from the C protocol
# definitions. Byte-compiling is not part of a build:
#
#   make check    - compile everything as a test, keeping no .elc (see
#                   check-elisp.el). Run from src's 'test' target.
#   make test     - run the ERT tests in tests/*-tests.el. Also run from
#                   src's 'test' target.
#   make compile  - compile and keep the .elc files. Used when installing,
#                   from the top level Makefile.
#
# utils/emacs-devel loads the sources, so a development tree keeps no .elc.

all: c-xrefprotocol.el

# Ensure protocol is in sync
c-xrefprotocol.el: ../../src/protocol.th ../../src/refactorings.def c-xrefprotocol.elt
	# Convert protocol.th into Elisp defvar's
	cpp c-xrefprotocol.elt | grep -v \# | sed '/./,$$!d' > c-xrefprotocol.el

.PHONY: check
check: c-xrefprotocol.el
	@if command -v emacs > /dev/null; then \
		emacs -Q --batch -l check-elisp.el ; \
	else \
		echo "emacs not found - skipping elisp compilation check" ; \
	fi

.PHONY: test
test: c-xrefprotocol.el
	@if command -v emacs > /dev/null; then \
		emacs -Q --batch -L . -l ert -l c-xrefprotocol.el -l c-xrefdoc.el -l c-xrefactory.el -l c-xref.el \
		      $(addprefix -l ,$(wildcard tests/*-tests.el)) -f ert-run-tests-batch-and-exit ; \
	else \
		echo "emacs not found - skipping elisp tests" ; \
	fi

.PHONY: compile
compile: c-xrefprotocol.el
	-rm -f *.elc
	emacs -Q --batch -l compileCxrefactory.el

clean:
	-rm -f *.elc

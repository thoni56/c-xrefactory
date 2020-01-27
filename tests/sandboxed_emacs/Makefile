# Run an emacs in a sandboxed environment to e.g. inspect commuication
# or run a hacked c-xref binary

ACTUAL_HOME = $(HOME)

# Run a sandboxed emacs
EMACS = emacs
all emacs:
	@HOME="$(PWD)" PATH="$(PWD)/bin":"$(PATH)" $(EMACS) -l $(PWD)/.emacs.d/init.el -l "$(PWD)"/.emacs.d/override-el-get-update.el --eval '(setq c-xref-home-directory "$(PWD)/bin/")' ../../src/cxref.c

gui: EMACS = /Applications/Emacs.app/Contents/MacOS/Emacs
gui: emacs

spy: c-xref-spy

c-xref-spy: bin/c-xref
	gcc -o bin/c-xref -g -Wall c-xref-spy.c

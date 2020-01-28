# Run an emacs in a sandboxed environment to e.g. inspect commuication
# or run a hacked c-xref binary

ACTUAL_HOME = $(HOME)
TARGET=/Users/thomas/Utveckling/c-xrefactory/src/c-xref

# Run a sandboxed emacs
EMACS = emacs
all emacs:
	@HOME="$(PWD)" PATH="$(PWD)/bin":"$(PATH)" $(EMACS) -l $(PWD)/.emacs.d/init.el -l "$(PWD)"/.emacs.d/override-el-get-update.el --eval '(setq c-xref-home-directory "$(PWD)/bin/")' --eval '(setq c-xref-debug-mode t)' ../../src/cxref.c

gui: EMACS = /Applications/Emacs.app/Contents/MacOS/Emacs
gui: emacs

spy: c-xref-spy

c-xref-spy: bin/c-xref
	$(CC) -o bin/c-xref -g -Wall -DTARGET=\"$(TARGET)\" c-xref-spy.c

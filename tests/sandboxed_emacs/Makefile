# Run an emacs in a sandboxed environment to e.g. inspect commuication
# or run a hacked c-xref binary

# This is the target to exec...
TARGET=../../src/c-xref
# ... with the full path...
TARGET_PATH=`python -c "import os,sys; print(os.path.realpath(os.path.expanduser('$(TARGET)')))" "${1}"`

# Run a sandboxed emacs
EMACS = emacs
all emacs:
	@if [ ! -e .emacs.d/el-get/c-xrefactory ] ; then \
		ln -s $(CURDIR)/../../ .emacs.d/el-get/c-xrefactory ; \
	fi
	@HOME="$(PWD)" PATH="$(PWD)/bin":"$(PATH)" $(EMACS) -l $(PWD)/.emacs.d/init.el -l "$(PWD)"/.emacs.d/override-el-get-update.el --eval '(setq c-xref-home-directory "$(PWD)/bin/")' --eval '(setq c-xref-debug-mode t)' ../../src/cxref.c

gui: EMACS = /Applications/Emacs.app/Contents/MacOS/Emacs
gui: emacs

spy: bin/c-xref

c-xref-spy: c-xref-spy.c Makefile
	$(CC) -o c-xref-spy -g -Wall -DTARGET=\"$(TARGET_PATH)\" c-xref-spy.c

bin/c-xref: c-xref-spy
	cp c-xref-spy bin/c-xref

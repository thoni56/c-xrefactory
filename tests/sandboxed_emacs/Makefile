# Run an emacs in a sandboxed environment to e.g. inspect communication
# or run a hacked c-xref binary
#
# NOTE: uses a copy of pipe-spy (https://github.com/thoni56/pipe-spy) and compiles
# that to a c-xref spy

# Environment variables for the pipe-spy/c-xref-spy to pick them up
ENV=HOME="$(PWD)" PATH="$(PWD)/bin:$(PATH)"
TARGET=TARGET=$(TARGET_PATH)
LOG=LOGFILE=$(CURDIR)/$(LOGFILE)

all: pure

EMACS_COMMAND = emacs -l $(PWD)/.emacs.d/init.el --eval '(setq c-xref-exec-directory "$(PWD)/bin/")' --eval '(setq c-xref-debug-mode t)' --eval '(setq c-xref-debug-preserve-tmp-files t)'

# Run a pure sandboxed Emacs which should use the latest elisp and c-xref
pure: c-xrefactory
	make -C c-xrefactory/src c-xref
	cp c-xrefactory/src/c-xref $(CURDIR)/bin/
	-rm -r c-xrefactory/editors/emacs/*.elc
	@$(ENV) $(EMACS_COMMAND)

debug: c-xrefactory
	@$(ENV) $(EMACS_COMMAND)


# Run a sandboxed emacs using the spy to log interaction

# This is the target to exec...
TARGET_FILE=../../src/c-xref
# ... with the full path...
TARGET_PATH=`python -c "import os,sys; print(os.path.realpath(os.path.expanduser('$(TARGET_FILE)')))" "${1}"`

# Where to log interaction (pipe-spy adds its PID to the filename to avoid clashes)
LOGFILE=logfile

spy: .emacs.d/c-xrefactory pipe-spy
	$(ENV) $(TARGET) $(LOG) $(EMACS_COMMAND) ../../src/cxref.c
	@echo Interaction saved in $(CURDIR)/$(LOGFILE)

pipe-spy: pipe-spy.c Makefile
	$(CC) -o pipe-spy -g -Wall -DTARGET=\"$(TARGET_PATH)\" pipe-spy.c
	cp pipe-spy bin/c-xref

c-xrefactory :
	ln -s $(CURDIR)/../.. c-xrefactory

curdir:
	echo $(CURDIR)

# Run an emacs in a sandboxed environment to e.g. inspect communication
# or run a hacked c-xref binary
#
# NOTE: uses a copy of pipe-spy (https://github.com/thoni56/pipe-spy) and compiles
# that to a c-xref spy

# This is the target to exec...
TARGET_FILE=../../src/c-xref
# ... with the full path...
TARGET_PATH=`python -c "import os,sys; print(os.path.realpath(os.path.expanduser('$(TARGET_FILE)')))" "${1}"`

# Where to log interaction (pipe-spy adds its PID to the filename to avoid clashes)
LOGFILE=logfile

# Environment variables for the pipe-spy/c-xref-spy to pick them up
ENV=HOME="$(PWD)" PATH="$(PWD)/bin:$(PATH)"
TARGET=TARGET=$(TARGET_PATH)
LOG=LOGFILE=$(CURDIR)/$(LOGFILE)

all:
	@echo "Make 'spy' or 'pure'"

# Run a sandboxed emacs using the spy to log interaction
spy: clean bin/c-xref .emacs/el-get/c-xrefactory
	@$(ENV) $(TARGET) $(LOG) emacs -l $(PWD)/.emacs.d/init.el -l "$(PWD)"/.emacs.d/override-el-get-update.el --eval '(setq c-xref-home-directory "$(PWD)/bin/")' --eval '(setq c-xref-debug-mode t)' --eval '(setq c-xref-debug-preserve-tmp-files t)' ../../src/cxref.c
	@echo Interaction saved in $(LOGFILE)

bin/c-xref: pipe-spy
	-rm bin/c-xref
	ln -s $(CURDIR)/pipe-spy bin/c-xref

.emacs.d/el-get/c-xrefactory:
	ln -s $(CURDIR)/../../ .emacs.d/el-get/c-xrefactory ; \

pipe-spy: pipe-spy.c Makefile
	$(CC) -o pipe-spy -g -Wall -DTARGET=\"$(TARGET_PATH)\" pipe-spy.c

clean:
	-rm $(LOGFILE)* /tmp/c-xrefx*.log

pure: .emacs.d/el-get/c-xrefactory
	@-rm bin/c-xref
	@ln -s $(CURDIR)/../../src/c-xref bin/c-xref
	@$(ENV) emacs -l $(PWD)/.emacs.d/init.el -l "$(PWD)"/.emacs.d/override-el-get-update.el --eval '(setq c-xref-home-directory "$(PWD)/bin/")' --eval '(setq c-xref-debug-mode t)' --eval '(setq c-xref-debug-preserve-tmp-files t)' ../../src/cxref.c

include ../Makefile.boilerplate

# Every option the Emacs client sends is one the server accepts. The system
# tests exercise the server through commands.input, so a client still sending
# a renamed or removed option is otherwise only found by a user, as an
# "unknown option" on every request.

TOP = ../..

$(TEST):
	@$(TOP)/utils/client_options_known.py $(TOP)/src/options.c $(TOP)/src/server.c \
	    $(TOP)/editors/emacs/c-xref.el $(TOP)/editors/emacs/c-xrefactory.el > output
	$(VERIFY)

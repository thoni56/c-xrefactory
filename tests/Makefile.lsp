# Makefile for LSP-based tests
#
# Include this instead of Makefile.boilerplate for tests that use the LSP interface.
# Provides LSP-specific debug targets that handle stdin redirection from input.lsp

include $(dir $(lastword $(MAKEFILE_LIST)))/Makefile.boilerplate

# Platform detection
UNAME_S := $(shell uname -s)

# Default breaking point
DEBUG_BREAK = lsp_server

# Debug target - starts c-xref with stdin from input.lsp, then attaches debugger
# Requires ptrace permissions (sets ptrace_scope on Linux)
debug: input.lsp
ifeq ($(UNAME_S),Linux)
	@echo "Setting ptrace_scope to allow attach..."
	@echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope > /dev/null
endif
	@$(COMMAND) -delay=1 < input.lsp & echo $$! > .c-xref.pid
	@gdb -q -p `cat .c-xref.pid` -ex "break internalCheckFail" -ex "break $(DEBUG_BREAK)" $(EXTRA_DEBUG) -ex "continue"
	@rm -f .c-xref.pid

# Variant with cgdb
cgdb: input.lsp
ifeq ($(UNAME_S),Linux)
	@echo "Setting ptrace_scope to allow attach..."
	@echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope > /dev/null
endif
	@$(COMMAND) -delay=1 < input.lsp & echo $$! > .c-xref.pid
	@cgdb -q -p `cat .c-xref.pid` -ex "break internalCheckFail" -ex "break $(DEBUG_BREAK)" $(EXTRA_DEBUG) -ex "continue"
	@rm -f .c-xref.pid

# Quick crash dump for segfaults (non-interactive)
crash: input.lsp
	gdb -batch -ex "run" -ex "bt" -ex "info locals" \
	--args $(COMMAND) < input.lsp

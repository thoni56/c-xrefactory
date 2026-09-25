# utils/messages2commands.py keeps the preloads of a recorded session
#
# A request with -preload gets its content from the editor, so a command file
# that drops the preload replays a different session. By default the preloaded
# content is copied next to the commands and the -preload points at the copy.
# --strip-preloads drops them.

include ../Makefile.cli

$(TEST):
	@rm -rf emacs-tmp *.preload messages.txt output output.tmp
	@mkdir emacs-tmp && echo 'int edited;' > emacs-tmp/c-xref-preload-1 && echo 'int other;' > emacs-tmp/c-xref-preload-2
	@sed -e "s+CURDIR+$(CURDIR)+g" messages.tpl > messages.txt
	@echo "--- default" > output.tmp
	@../../utils/messages2commands.py messages.txt >> output.tmp 2>/dev/null
	@echo "--- source.c.preload" >> output.tmp
	@cat source.c.preload >> output.tmp 2>/dev/null || echo "(none)" >> output.tmp
	@echo "--- other.c.preload" >> output.tmp
	@cat other.c.preload >> output.tmp 2>/dev/null || echo "(none)" >> output.tmp
	@echo "--- --strip-preloads" >> output.tmp
	@../../utils/messages2commands.py --strip-preloads messages.txt >> output.tmp 2>/dev/null
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

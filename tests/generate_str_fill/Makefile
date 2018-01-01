include ../Makefile.boilerplate

$(TEST):
	@rm -rf output $@.log
	@printf "Tests in \"$@\": "
	@../../src/c-xref -xrefrc c-xrefrc -p $@ -task_regime_generate -str_fill $@.[ch] -o $@.log > $(TEST).output
	@diff $(TEST).output $(TEST).expected ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

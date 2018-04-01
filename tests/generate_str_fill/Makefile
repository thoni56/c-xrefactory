include ../Makefile.boilerplate

$(TEST):
	@rm -rf output $@.log
	@printf "Tests in \"$@\": "
	@../../src/c-xref -xrefrc .c-xrefrc -p $@ -task_regime_generate -str_fill structures.h -o $@.log > $(TEST).h
	@gcc -c -o /dev/null $(TEST).c ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

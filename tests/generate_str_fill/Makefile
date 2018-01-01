#

TEST = $(notdir $(CURDIR))

all: c-xrefrc $(TEST)

c-xrefrc: ../c-xrefrc.tpl
	@sed -e "s+PWD+$(CURDIR)+g" -e "s+PROJ+$(TEST)+g" ../c-xrefrc.tpl > c-xrefrc

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

# Automated test for finding standard includes (uses stdbool.h)
# Initial version

all: standard_includes

standard_includes:
	@rm -rf Xrefs $@.log
	@printf "Tests in \"$@\": "
	@../../src/c-xref $@.c -o $@.log
	@grep "can't open file stdbool.h" $@.log > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

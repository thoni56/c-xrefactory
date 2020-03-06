include ../Makefile.boilerplate

$(TEST):
	@../../utils/edit_server_driver.py replay1.txt $(PWD) buffer > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

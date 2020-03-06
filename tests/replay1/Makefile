include ../Makefile.boilerplate

$(TEST):
	@printf "Tests in '$@': "
	@../../utils/edit_server_driver.py replay1.txt $(CURDIR) buffer > output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

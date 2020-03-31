# Automated test for running c-xref on itself and see if generated
# cross references are the same

TEST = $(shell basename '$(CURDIR)')
OK = echo "Tests in '$(TEST)': $(shell tput setaf 2)Ok$(shell tput sgr0)"
FAIL = echo "Tests in '$(TEST)': $(shell tput setaf 1)Failed$(shell tput sgr0)"


all: c-xref

c-xref: .c-xrefrc
	@rm -rf C-Xrefs $@.log
	@../run-c-xref -p c-xref -o c-xref.log -refs $(CURDIR)/C-Xrefs/c-xref
	@diff CXrefs/c-xref c-xref.expected > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

.c-xrefrc: c-xrefrc.tpl
	@sed -e "s+PWD+$$PWD+g" c-xrefrc.tpl > .c-xrefrc

clean:
	rm -rf CXrefs c-xref.log out .c-xrefrc

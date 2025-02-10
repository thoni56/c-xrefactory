# This test exposes a problem with 'delete parameter' when the function is used as a
# function argument

all:
	echo '<exit>' | ../../src/c-xref -xrefactory-II -crlfconversion -crconversion -refactory -xrefrc .c-xrefrc -p `pwd` -rfct-del-param -olcxparnum=3 -olcursor=2196 src/classhierarchy.c

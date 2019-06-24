cp -r ../doc/cexercise /tmp

export SOURCEPATH=/tmp/cexercise
export CXREFRC="-xrefrc /tmp/cexercise/.c-xrefrc -p /tmp/cexercise -refs /tmp/cexercise/C-Xrefs"

./c-xref -refactory -rfct-add-param -olcxparnum=1 "-rfct-param1=int i;" -rfct-param2=0 $CXREFRC -olcursor=293 /tmp/cexercise/refactorings/parameter.c

./c-xref -refactory -rfct-del-param -olcxparnum=1 -xrefrc $CXREFRC -olcursor=293 /tmp/cexercise/refactorings/parameter.c

./c-xref -refactory -rfct-move-param -olcxparnum=1 -olcxparnum2=2 $CXREFRC -olcursor=293 /tmp/cexercise/refactorings/parameter.c

./c-xref -refactory -rfct-rename -renameto=tototo $CXREFRC -olcursor=293 /tmp/cexercise/refactorings/parameter.c

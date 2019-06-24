cp -r ../doc/jexercise /tmp

export SOURCEPATH=/tmp/jexercise
export CXREFRC="-xrefrc /tmp/jexercise/.c-xrefrc -p /tmp/jexercise"

./c-xref -refactory -rfct-add-param -olcxparnum=1 "-rfct-param1=int i;" -rfct-param2=0 $CXREFRC -olcursor=866 /tmp/jexercise/com/xrefactory/refactorings/MoveMethod.java

./c-xref -refactory -rfct-del-param -olcxparnum=1 -xrefrc $CXREFRC -olcursor=866 /tmp/jexercise/com/xrefactory/refactorings/MoveMethod.java

./c-xref -refactory -rfct-move-param -olcxparnum=1 -olcxparnum2=2 $CXREFRC -olcursor=866 /tmp/jexercise/com/xrefactory/refactorings/MoveMethod.java

./c-xref -refactory -rfct-rename -renameto=tototo $CXREFRC -olcursor=866 /tmp/jexercise/com/xrefactory/refactorings/MoveMethod.java

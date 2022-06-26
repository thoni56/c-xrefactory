[CURDIR]
  //  input files and directories (processed recursively)
  CURDIR
  //  directory where tag files are stored
  -refs CURDIR/CXrefs
  //  number of tag files
  -refnum=10
  -set cp CURDIR
  -classpath ${cp}	// classpath for c-xrefactory
  -sourcepath ${cp}	// sourcepath for c-xrefactory

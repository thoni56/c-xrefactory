[TEST]
  //  input files and directories (processed recursively)
  CURDIR
  //  directory where tag files are stored
  -refs CURDIR
  //  number of tag files
  -refnum=10
  //  setting for Emacs compile and run
  -set compilefile "cc %s"
  -set compiledir "cc *.c"
  -set compileproject "
    cd CURDIR
    make
    "
  -set run1 "a.out"
  -set run5 ""  // an empty run; C-F8 will only compile
  //  set default to run1
  -set run ${run1}

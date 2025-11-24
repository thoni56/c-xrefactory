[CURDIR]
  //  input files and directories (processed recursively)
  CURDIR/ffmpeg
  //  exclude compat atomics (breaks system stdatomic.h due to state pollution)
  -prune atomics
  //  include paths for ffmpeg internal headers
  -ICURDIR/ffmpeg
  //  directory where tag files are stored
  -refs CURDIR/CXrefs
  //  number of tag files
  -refnum=10
  -DFFMPEG_VERSION="unknown"
  -DFFMPEG_CONFIGURATION="unknown"

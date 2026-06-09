[CURDIR]
  CURDIR/ffmpeg
  //  exclude compat atomics (breaks system stdatomic.h due to state pollution)
  -prune atomics
  -ICURDIR/ffmpeg
  -DFFMPEG_VERSION="unknown"
  -DFFMPEG_CONFIGURATION="unknown"

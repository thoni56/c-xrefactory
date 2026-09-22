#!/bin/bash

GIT_HASH=$(git describe --abbrev=5 --always --tags --dirty)
VERSION=$(git describe --abbrev=0 --tags)

sed "s/@GIT_HASH@/$GIT_HASH/g; s/@VERSION@/$VERSION/g" options_config.h.in > options_config.h.tmp
# Only replace the real file when the content differs, so an unchanged
# regeneration does not move its mtime. That is not enough on its own: GIT_HASH
# carries `git describe --dirty`, so the content legitimately changes the moment
# the working tree goes from clean to dirty and back again. The watch targets in
# the Makefile therefore also ignore options_config.h, or every first edit after
# a commit would run the tests twice.
if [ ! -f options_config.h ] || ! cmp -s options_config.h.tmp options_config.h; then
    mv options_config.h.tmp options_config.h
else
    rm options_config.h.tmp
fi

#!/bin/bash

GIT_HASH=$(git describe --abbrev=5 --always --tags --dirty)
VERSION=$(git describe --abbrev=0 --tags)

sed "s/@GIT_HASH@/$GIT_HASH/g; s/@VERSION@/$VERSION/g" options_config.h.in > options_config.h

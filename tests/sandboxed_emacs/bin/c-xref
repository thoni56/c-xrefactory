#! /bin/sh
# This is a "c-xref spy" in that in can be used as an intermediary between a sandboxed Emacs
# and the actual c-xref executable to inspect arguments and communication
CXREF=$HOME/../../src/c-xref
echo $CXREF "$@" > $HOME/latest-invocation
$CXREF "$@"

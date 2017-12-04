# NOTES about c-xref implementation, design etc. #

## Building ##

One step in the build process is generating initialization information
for all the things in standard include files, which of course becomes
very dependent on the system you are running this on.

The initial recovered c-xrefactory relied on having a working _c-xref_
for the current system. I don't really know how they managed to do
that for all the various systems they were supporting.

Modern thinking is that you should always be able to build from
source, so this is something that needed change. We also want to
distribute _c-xref_ as an el-get library which requires building from
source and should generate a version specific for the current system.

The strategy selected, until some better idea comes along, is to try
to build a _c-xref.bs_, if there isn't one already, from the sources in
the repository and then use that to re-generate the definitions and
rebuild a proper _c-xref_.

Currently that causes some [warning]:s when _c-xref.bs_ generates new
_strTdef.h_ and _strFill.h_.

## Design ##

### Editor-Server

The c-xrefactory application is divided into the server, _c-xref_ and
the editor part, currently only emacs:en are supported so that's
implemented in the env/emacs-packages.

Communication between them is performed using text through standard
input/output to/from _c-xref_. The protocol is defined in
src/protocol.tc and must match env/emacs/c-xrefprotocol.el.

NOTE: I find it strange that the macros for C define static variables for
these PROTOCOL_ITEMs in every C unit that includes _protocol.h_.

NOTE: There is a similar structure with _c-xrefprotocol.elt_ which
includes _protocol.tc_ to wrap the PROTOCOL_ITEMs into
`defvar`s. Although, at this point, I don't understand exactly where
that expansion is done for Emacs-lisp.

### Bootstrapping

It seems like _c-xref_ needs to be bootstrapped by reading in a lot of
predefined header files to get system definitions. This is done using
options like `-task_regime_generate' which prints a lot of data
structures on the standard output which is then fed into _strFill.h_,
_strTdef.h_ and _enumTxt.h_ by the Makefile.

NOTE: Why this is necessary, I don't know. It might be an
optimization. In any case it creates an extra complexity building and
maintaining and to the structure of _c-xref_.

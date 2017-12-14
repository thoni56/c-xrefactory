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

_c-xref_ needs to be bootstrapped by reading in a lot of predefined
header files to get system definitions. This is done using options
like `-task_regime_generate' which prints a lot of data structures on
the standard output which is then fed into _strFill.h_, _strTdef.h_
and _enumTxt.h_ by the Makefile.

NOTE: Why this is necessary, I don't exactly know. It might be an
optimization. In any case it creates an extra complexity building and
maintaining and to the structure of _c-xref_.

#### typedefs

In _options.h_ there are a number of definitions which somehow are
send to the compiler/preprocessor or used so that standard settings
are the same as if a program will be compiled using the standard
compiler on the platform. At this point I don't know how this is done,
maybe just entered as symbols in one of the many symboltables?

Typical examples include "__linux" but also on some platforms things
like "fpos_t=long".

#### Include paths

Also in _options.h_ some standard-like include paths are added, but
there is a better attempt in _getAndProcessGccOptions()_ which uses
the compiler/preprocessor itself to figure out those paths. This is
much better and should really be the only way, I think.

## Include files

Seems this code is using very old C style with a separate proto.h
where all prototypes for all externally visible functions are placed.

This file also is used in the "generation" step, which I can't
understand right now. Does the "generation" step create internal data
for c-xref source code!?!??!

## C-Xrefs file

This file (or files) contains compact, but textual representations of
the cross-reference information. Format is somewhat complex, but here
are somethings that I think I have found out:

- the encoding has one character markers which are listed at the top
  of cxfile.c
- the coding seems to often start with a number and then a character,
  such as '4l' means line 4
- references seems to be optimized to not repeat information if it
  would be a repetition, such as '15l3cr7cr' means that there are two
  references on line 15, one in column 3 the other in column 7
- so there is a notion of "current" for all values which need not be
  repeated
- e.g. references all use 'fsulc' fields, i.e. file, symbol index,
  line and column, but do not repeat 'f' as long as it is the same

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
_strTdef.g_ and _strFill.g_.

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

NOTE: that there has been an improvement here since we now only
generate a .c-file for the option data which is access through the
.h-definitions as per normal, modern, C-style.

NOTE: There is a similar structure with _c-xrefprotocol.elt_ which
includes _protocol.tc_ to wrap the PROTOCOL_ITEMs into
`defvar`s. Although, at this point, I don't understand exactly where
that expansion is done for Emacs-lisp.

NOTE: there is now some Makefile trickery that ensures that these two
are in sync.


### Bootstrapping

#### Reasons

_c-xref_ uses a load of structures, and lists of them, that need to be
created and initialized in a lot of places (such as the parsers). To
make this somewhat manageable, _c-xref_ itself parses the strucures
and generates macros to fill them.

_c-xref_ is also bootstrapped into reading in a lot of predefined
header files to get system definitions as "preloaded
definitions".

NOTE: Why this is necessary, I don't exactly know. It might be an
optimization. In any case it creates an extra complexity building and
maintaining and to the structure of _c-xref_.

#### Mechanism

The bootstrapping uses _c-xref_'s own capability to parse C-code and
parse those structures and spit out filling macros, and some other
stuff.

This is done using options like `-task_regime_generate' which prints a
lot of data structures on the standard output which is then fed into
generate versions of _strFill_, _strTdef_ and _enumTxt_ by the
Makefile.

#### Compiler defines

In _options.h_ there are a number of definitions which somehow are
sent to the compiler/preprocessor or used so that standard settings
are the same as if a program will be compiled using the standard
compiler on the platform. At this point I don't know exactly how this
conversion from C declarations to compile time definitions is done,
maybe just entered as symbols in one of the many symboltables?

Typical examples include "__linux" but also on some platforms things
like "fpos_t=long".

I've implemented a mechanism that uses "gcc -E -mD" to print out and
catch all compiler defines in `compiler_defines.h`. This was necessary
because of such definitions on Darwin which where not in the
"pre-programmed" ones. And this is more general and should possibly
completely replace the "programmed" ones in `options.c`?

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
for c-xref source code!?!??! I think this is because of the
bootstrapping of fill macros etc.

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


## Naming conventions

_C-xref_ started probably as a cross-referencer for the languages
supported (C, Java, C++), orginally had the name "xref" which became
"xrefactory" when refactoring support was added. And when Mariàn
released a "C only" version in 2009 some of all "xref" references was
changed to "c-xref". So, as most software, there is a history and a
naming legacy to remember.

Here are some of the conventions in naming that are being used:

  *olcx* \
  "On-line CX" (Cross-reference)

  *OLO* \
  "On-line option" - some kind of options for the server

## proto.h

_proto.h_ is a file which declares enums and structures that are read
as part of the bootstrap process. Here be dragons (magic...).

It seems like for each struct defined by reading _proto.h_ there will
be a set of macros generated that can act as "functions" to fill a
structure as a one-liner.

E.g given a structure like

    struct myStruct { int a; struct anotherStruct b; }

There will be a typedef generated (in _strTdef_):

    typedef struct myStruct S_myStruct;

And there will be two "filler"-function macros defined (in
_strFill_):

    #define FILL_myStruct(XXX, ARG0, ARG1) { .... }

and

    #define FILLF_myStruct(XXX, ARG0, ARG1, ARG2, ARG3, ...) { .... }

The first simply allows you to initialize a structure pointed to by
the `XXX` argument using the `ARG0` etc. to fill out consecutive
fields.

The second is a "serialized" version where you can fill *every* field
in the complete structure. So the number of arguments will depend on
the total number of fields in all the substructures.

This makes it easy to add a new structure or to add fields to an already
existing one without having to manually update the "Create"-function
for tha structure.

However, the usage of the _FILL_ functions is less that crystal clear...

I also think that you could actually merge the struct definition with
the typedef so that _strTdef.h_ would not be needed. But it seems that
this design is because the structures in _proto.h_ are not a directed
graph, so loops makes that impossible. Instead the typedefs are
included before the structs:

    #include "strTdef.h"

    struct someNode {
        S_someOtherNode *this;
        ...

    struct someOtherNode {
        S_someNode *that;
        ...

But this is ideomatically solved using the structs themselves:

    struct someNode {
        struct someOtherNode *this;
        ...

    struct someOtherNode {
        struct someNode *that;
        ...

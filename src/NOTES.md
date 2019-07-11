# NOTES about c-xref implementation, design etc. #

## Functionality ##

The `c-xref` proper can do the following things:

  - editor server - answering to an editors requests
  - xref - cross-referencing sources
  - html - HTML-ify sources with cross references in links
  - generate - a c-xref internal feature to generate structure fill macros

## Versions ##

The current sources are in 1.6.X range. This is the same as the orginal
xrefactory and probably also the proprietary C++ supporting version.

There is an option, "-xrefactory-II", that might indicate
something going on. But currently the only difference seems to be if
output is in the form of fprintf:s or using functions in the
`ppc`-family (either calling `ppcGenRecord()` or `fprint`ing using
some PPC-symbol). This, and hinted to in how the emacs-part starts
the server and some initial server option variables in refactory.c,
indicates that the communication from the editor and the refactory
server is using this. It does *not* look like this is a forward to
next generation attempt.

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
rebuild a proper _c-xref_. See Bootstrapping below.

## Design ##

### Editor-Server

The c-xrefactory application is divided into the server, _c-xref_ and
the editor part, currently only emacs:en are supported so that's
implemented in the env/emacs-packages. (Though the jEdit source is now
also resurrected, it is completely untested.)

#### Protocol

Communication between the editor and the server is performed using
text through standard input/output to/from _c-xref_. The protocol is
defined in src/protocol.tc and must match env/emacs/c-xrefprotocol.el.

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

TODO: Create tests that exercise the protocol, both directions.

#### Invocation of server

It is at this point a bit unclear how the server calls actually
work. It seems like the editor fires up a server and keeps talking
over the established channel (elisp function
'c-xref-start-server-process'). This puts extra demands on the memory
management in the server, since it might need to handle multiple
information sets and options (as read from a .cxrefrc-file) for
multiple projects simultaneously over a longer period of
time. (E.g. if the user enters the editor starting with one project
and then continues to work on another then new project options need to
be read, and new tag information be generated and read and cached.)

TODO: Figure out and describe how this works by looking at the
elisp-sources.

FINDINGS:
- c-xref-start-server-process in c-xref.el
- c-xref-send-data-to-running-process in c-xref.el
- c-xref-server-call-refactoring-task in c-xref.el

TODO: create some examples and tests for this context.

### Bootstrapping

#### Rationale

_c-xref_ uses a load of structures, and lists of them, that need to be
created and initialized in a lot of places (such as the parsers). To
make this somewhat manageable, _c-xref_ itself parses the strucures
and generates macros that can be used to fill them with one call.

_c-xref_ is also bootstrapped into reading in a lot of predefined
header files to get system definitions as "preloaded
definitions".

NOTE: Why this pre-loading is necessary, I don't exactly know. It
might be an optimization. In any case it creates an extra complexity
building and maintaining and to the structure of _c-xref_.

#### Mechanism

The bootstrapping uses _c-xref_'s own capability to parse C-code and
parse those structures and spit out filling macros, and some other
stuff.

This is done using options like `-task_regime_generate' which prints a
lot of data structures on the standard output which is then fed into
generated versions of _strFill_, _strTdef_(no longer exists) and
_enumTxt_ by the Makefile.

The process starts with building a _c-xref.bs_ executable from checked
in sources. This compile uses a BOOTSTRAPPING define that causes some
header files to include pre-generated versions of the generated files
(currently _strFill.bs.h_ and _enumTxt.bs.h_) which should work in all
environments.

NOTE: if you change the name of a field in a structure that is subject
to FILL-generation you will need to manually update the
_strFill.bs.h_, but a "make cleaner all" will show you where those are.

After the _c-xref.bs_ has been built, it is used to generate _strFill_
and _enumTxt_ which might include specific structures for the current
environment.

HOWEVER: if FILL macros are used for structures which are different on
some platforms, say a FILE structure, that FILL macro will have
difference number of arguments, so I'm not sure how smart this "smart"
generation technique actually is.

TODO: Investigate alternative approaches to this generate "regime",
perhaps move to a "class"-oriented structure with initialization
functions for each "class" instead of macros.

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
"pre-programmed" ones.

TODO?: As this is a more general approach it should possibly
completely replace the "programmed" ones in `options.c`?

#### Include paths

Also in _options.h_ some standard-like include paths are added, but
there is a better attempt in _getAndProcessGccOptions()_ which uses
the compiler/preprocessor itself to figure out those paths.

TODO?: This is much better and should really be the only way, I think.

### Regimes

As the base for `c-xrefactory` is some kind of cross-referencing
engine, there are traces of multiple responsibilities, all still
functional. The command line options `-regime X`/`s_opt.taskRegime` as
shown in the final lines in `main()`.

They are intertwined, probably through re-use of already existing
functionality when extending to a refactoring browser.

TODO?: Strip away the various "regimes" into more separated concerns.

### Passes

There is a variable in `main()` called `firstPassing` which is set and passed
down through `mainEditServer()` until it is reset in
`mainFileProcessingInitialisations()` after `initCaching()`.

This seems to be a very hacky way to only initialize caching
once... Maybe I'm missing something...

### Memory allocation

There are multiple levels of memory management.

- Why is this required (possibly because of the long running server
model)?
- Exactly how this memory is allocated?
- Why handle this allocation in disparate spaces?
- Why does not standard malloc()/free() suffice?

### Caching

There is obviously some caching going on. Don't know of what at this
point. Tag data?

## Include files

This code is using very old C style with a separate proto.h where all
prototypes for all externally visible functions are placed.

This file also is used in the "generation" step, which I can't
understand right now. Does the "generation" step create internal data
for c-xref source code!?!??! I think this is because of the
bootstrapping of fill macros etc.

Of course this will have to change into the modern x.h/x.c externally
visible interface model.

## CXrefs file

This file (or files) contains compact, but textual representations of
the cross-reference information. Format is somewhat complex, but here
are somethings that I think I have found out:

- the encoding has one character markers which are listed at the top
  of cxfile.c

- the coding seems to often start with a number and then a character,
  such as '4l' (4 ell) means line 4, 23c mean column 23

- references seems to be optimized to not repeat information if it
  would be a repetition, such as '15l3cr7cr' means that there are two
  references on line 15, one in column 3 the other in column 7

- so there is a notion of "current" for all values which need not be
  repeated

- e.g. references all use 'fsulc' fields, i.e. file, symbol index,
  usage, line and column, but do not repeat a 'fsulc' as long as it is
  the same

- some "fields" have a length indicator before, such as filenames
  ('6:/abc.c') indicated by ':' and version information ('34v file
  format: C-xrefactory 1.6.0 ') indicated by 'v'.

So a line might say

    12205f 1522108169p m1ia 84:/home/...

The line identifies the file with id 12205. The file was last included
in an update of refs at sometime which is identified by 1522108169
(mtime), has not been part of a full update of xrefs, was mentioned on
the command line. (I don't know what the 'a' means...) Finally, the
file name itself is 84 characters long.

TODO: Build a tool to decipher this so that tests can query the
generated data for expected data. This is now partly ongoing in the
'utils' directory.

## Parsers

_C-xref_ uses a patched version of berkley yacc to generate
parsers. There are a number of parsers, for C, C expressions and
Java. There are also traces of calls to the C++ parser that existed
but was proprietary.

The patch to byacc is mainly to the skeleton and seems to relate
mostly to handling of errors and adding a recursive parsing feature
that is required for Java. It is not impossible that the change can be
adapted to other versions of yacc, but this has not be tried.

Some changes are also made to be able to accomodate multiple parsers
in the same executable. The Makefile generates the parsers and renames
them as appropriate.

TODO?: Should we just scrap the Java support and focus on C since a)
the Java support is for Java 1.6 and b) there are more mature Java
refactoring support available?

## Comment handling

It seems like there are special provisions for handling comments that
starts with "//&" because references to symbols within these comments
are actually found and registered. They will be affected by renames
and local motions.

## Naming conventions

_C-xref_ started (probably) as a cross-referencer for the languages
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
for that structure.

NB. If one structure is different on different platforms the
FILLF-function will also be, so that creates possible compilation
problems...

So, the usage of the _FILL_ functions is less that crystal clear, and
might be the target of some serious refactoring into real functions
instead. That would show which structures, and fields, actually need
"fill" functions.

## How things were

In this section are descriptions and saved texts that described how
things were before. They are no longer true, since that quirk, magic
or bad coding is gone. But it is kept here as an archive for those
wanting to do backtracking to original sources.

### strTdef.h

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

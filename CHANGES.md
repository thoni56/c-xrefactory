# Version 1.8.2

- Remove caching, slowdown but less complexity in code

# Version 1.8.1

- Add C11, C19 and C22 features and symbols

# Version 1.8.0

- Remove Java-specific fields in references database

# Version 1.7.1

- Many refactorings and tidyings
- New refactoring: Rename Included File

# Version 1.6.22

- Support for Java & Jedit removed
- Many, many refactorings and tidyings

# Version 1.6.16

- HTML cross-referencing removed

# Version 1.6.13

- Add support for compound literals in C.
- Thousands of refactorings...


# Version 1.6.12

- Adding the first test (ever?) for some coverage.


# Version 1.6.11

- First version as c-xrefactory picked from sourceforge and moved to GitHub by @thoni56, https://github.com/thoni56/c-xrefactory for future changes.


# Version 1.6.10

- A forgotten license check has been removed.


# Version 1.6.9

- Fixed problem of compilation of source with gcc4.


# Version 1.6.8

- The first free release.


# Version 1.6.7

- Fixed problem with removal of unused parameter during 'Turn virtual
method to Static'.

- Fixed problem with pretty printing in a browser windows when a field
has the same name as a method.


# Version 1.6.6

- Database files are more compact because unnecessary white spaces
have been removed.

- Faster update when only small number of files has been modified
(this is the case during majority of refactorings).

- Faster Extract Method and renaming of local variables in Java.

- Fixed the problem with browser opened in multiple Emacs frames.

- 'Turn Dynamic into Static' now removes the new parameter if it is
unused.  However, the case of recursive calls is still unhandled.

- Fixed the problem with the compilation under FreeBSD 5.


# Version 1.6.5

- OS/2 support for the xref-any distribution (thanks to Dmitry
Kuminov).

- Fixed the problem with imports in dead code detection.

- Fixed the problem when the symbol search dialog did not propose the
list of project symbols.

- Fixed the problem with truncation of lines making that in the
extract method dialog a part of the profile of the new method was
invisible.

- Fixed the problem with buffer switching after two successive
completion dialogs.

- Fixed the problem when an inner interface needed to be declared
static before moving.


# Version 1.6.4

- Refactoring problems with UTF-8 and UTF-16 encodings have been
fixed. Support for EUC and SJIS encodings has been implemented.

- Ordering of Java fully qualified type name completions has been
improved. Exact matches now appear at first positions.

- Problem with Extract Method refactoring when the region is selected
from bottom to top has been fixed.

- Few problems in syntax highlighting of completions under Emacs have
been fixed.


# Version 1.6.3

- Xrefactory correctly updates tags before refactoring also in C.

- the option 'Save files and update tags before refactorings' is
turned off under Emacs/XEmacs.

- problem with adding/removing/moving of parameter of a macro is
fixed.

- Emacs/XEmacs dialogs should work under an alphanumerical console.

- under Emacs/XEmacs, the C-g key combination pressed during
creation/update of tags (or when precomputing a refactoring) will kill
extern task and clean-up temporary files.

- problems with refactoring dialog after a failed refactoring are
fixed.

- problem with non-generating of correct imports in moving
refactorings is fixed.

- problem with undoing of 'Move Class to New File' refactoring
fixed. Also a failed refactoring does not create an empty buffer
anymore.

- problem when progress bar went beyond 100% fixed.

- few more non-specific bugs in refactoring fixed.


# Version 1.6.2

- fixed problem with update of references from header files.

- completion after left parenthesis displays also methods with no
parameters.



# Version 1.6.1

- problem with first update after create Tag file fixed (sometimes the
first update has reparsed too many files).

- fixed problem with formatting of completions when line truncation is
turned off under Emacs.

- fixed problem with 'move class' refactoring applied on interfaces.

- fixed few SPARC related problems (especially compilation with gcc).

- fixed problem with generation of HTML for large projects.

- fixed small problem with macro expansions in C preprocessor.

- jEdit interface works with both jEdit 4.1 and jEdit 4.2.


# Version 1.6.0

- Completions may be case insensitive removing the necessity of typing
upper case letters.  For example: getba_ is completed to getBackground
if this is the only appropriate completion.

- The Emacs interface has been entirely reimplemented. It now supports
multiple windows (protecting those opened by the user); supports basic
version control systems; uses left mouse button equivalently to middle
button and provides refactorings in a single function. The new
interface will require a little effort from older users, but it should
be more natural than the old one.  For beginning: <escape> closes
dialogs and ? proposes helps.

- Parameter completions for Java methods.  Completion of an empty
string at place where a parameter is expected shows method profiles.

- Symbol retrieval functions accept shell like expressions containing
wild character * [ ] and ?.

- Wildcard characters * [ ] and ? can be used also in input files,
include directories, classpath, sourcepath and javadocpath.

- Input files can be entered using paths relative to project
directory.

- Include directories can be entered using paths relative to currently
parsed file.

- -prune option can be used to prune unwanted directories when looking
for input files.

- Completion of fully qualified type names in Java can be customized.
It may propose all classes from jar archives, current project,
classpath and sourcepath.

- Moving refactorings can be customized to move also commentaries
preceding the moved definition.

- New communication protocol between Emacs and xref task has been
implemented.  This should considerably speed up refactorings under
MS-Windows systems.

- New editing function has been implemented under Emacs. It maps a
user defined macro on all occurrences of a symbol (dangerous but
useful).

- 'Last Compile & Run' macro under Emacs memorizes the last command
per project. This is more natural when working simultaneously on
several projects.

- The .xrefrc configuration file can be stored in a user defined
directory.



# Version 1.5.10

- Fixed problem when nested class was not inherited from an interface.

- Extraction is adding newline when region does not finish by it.

- Fixed problem when class tree items were not linked to sources
correctly.

- Option -source indicating used version of Java (i.e. 1.3 or 1.4) is
added for compatibility with javac compiler.

- Problem with evaluation of long constants in pre-preprocessor's
constant expressions fixed.



# Version 1.5.9

- New browsing function allows to push all references of a symbol
entered simply by its name.  This function was requested by ctags
users.

- In 'Search in Tag file' dialog the key is automatically browsing
Javadoc in case that definition reference is not found.

- Fixed possible problem in Extract-Method in presence of try-catch
statements.

- Fixed problem with inserting parameter to C function declarations.

- Better error recovery has been implemented in C parser.



# Version 1.5.8

- Improvements in completion user interface.  Those improvements make
completion auto-focus very natural to use.

- Several implementation constants were increased in binary
distributions which are now compiled with XREF_HUGE flag. This gives
possibility to index large files to users having only personal
licenses.

- Variable __BASE is now predefined in Xrefactory environment and is
referring to current project name. You can use ${__BASE} string in
your .xrefrc file instead of current project (directory) name.

- The 'Search Symbol in foreign Tag file' function does not take
default name of foreign project from .xrefrc file.  It is now asking
for the name interactively.

- An improvement in 'Extract region into function' refactoring for C
language. The improvement concerns better handling of & operator.

- Problem with '-stdop' option is fixed. The option is also available
under more mnemonic name '-include <file>'.

- Problem with '-javadocpath' option in combination with non JDK
javadocs in HTML generation fixed.

- Problem of browsing javadoc of a method having parameter of a nested
class is fixed.

- In the menu as well as in the documentation we are now using the
word 'Parameter' instead of 'Argument'.  The new term is conform to
M. Fowler's terminology.



# Version 1.5.7

- The problem with empty path in -sourcepath option has been fixed.

- Emacs keeps focus while browsing Javadoc with external viewer. This
makes browsing more natural.

- Browsed URL now starts with file:// prefix when browsing files in
local file system.

- New registration function has been implemented in Emacs. Windows
users do not need to edit _xrefrc file manually anymore.

- The Emacs window containing completions is now resized proportionaly
to number of completions. The minimal size can be customized with
xref-window-minimal-size variable.  If you do not like the new
feature, set this variable to negative value.



# Version 1.5.6

- a customizable variable 'xref-auto-switch-to-completion-window' is
available under Emacs/XEmacs.  When this variable is set to t, then
after a completion the keyboard focus is automatically switched to new
window containing completions.  In this window user can select
completion either by cursor motion or by continuing typing the
identifier.  The completion is definitely selected with <return>.

- key binding in symbol resolution window has been changed.  Now
<space> is bound to a function selecting only one item and browsing
definition reference.  The <Ins> key is bound to function
selecting/unselecting an item (formerly bound to <space>). 'q' key is
bound to function closing the window and poping references.

- the window showing results of search in tag file has automatic
focus.  In this window <space> inspects the definition of the symbol
and 'q' closes the window and returns to initial position in source
code.

- optionally minibuffer completion works for "Search in Tag File"
functions. In order to get minibuffer completion works you need to set
'xref-allow-completion-in-tag-search-prompt' (a customizable variable)
to t.

- fixed problem with JNI interface.

- fixed problem in "Turn static method to dynamic" in situations when
the parameter needs to be replaced by an explicit 'this'.

- fixed problem with '-fastupdate' in combination with
'-exactpositionresolve' option. The latest version did not marked that
in fact a full update was performed in such situation.

- fixed bug with evaluation of C preprocessor conditionals containing
undefined symbols.

- newline character in string constants is tolerated.

- definition of an empty structure (having no field) in C is
  tolerated.

- -I<directory> option is allowed with the same semantics as -I
<directory>.




# Version 1.5.5

- Move class refactoring moves top-level and nested classes.

- Move (rename) package moves also corresponding directories.

- File names are considered case sensitive also under MS-Windows.  The
option '-filescaseunsensitive' can be used to change them to case
unsensitive.

- A function deleting useless project is available under Emacs.

- Generation of HTML documentation can be launched directly from
  Emacs.



# Version 1.5.4

- Improvements in project creation macro.  Projects generated with
'Project->New' menu item are now more complete and cover more complex
projects.

- Possibility to put multiple project names for same options.  This
allows correct project auto-detection when project files are accessed
through different names (via symbolic links, for example).

- Extract method is now proposing new method definition and invocation
instead of generated header and footer. It seems to be more natural.

- When renaming or adding/removing parameter of a virtual method,
Xrefactory now proposed whole related definitions to be processed at
once. In practice it means that you can rename all methods related
through inheritance and interface implementations at once.  At
difference to previous versions related classes are computed as
transitive closure not only direct super classes and their subclasses.

- Add parameter now checks for possible collision of new parameter
with symbols from outer scope.

- Turn static to dynamic now checks for the correct type of new
parameter. It also checks for accidental clash of new parameter with
symbols from outer scope.

- Refactorings now check and report problems with method invocations
of form super.method() (when such a problem occurs).

- Completions in Java now display array variables and methods
returning an array in format 'Class[] foo()' instead of 'Class
foo()[]'.

- Wrong displaying of completions containing $ sign fixed.

- Completions now display access modification (public, protected)
every-time (even when completing only appropriate symbols).

- Optionally a completion completing single string will close window
with list of previous completions.

- An item 'run this' has been added to (X)Emacs IDE menu. It will run
Java virtual machine with currently editing class as main class
argument.

- Improvements in compile-run macros.



# Version 1.5.3

- Xrefactory is now capable to read majority of compressed jar
archives.

- New function 'Search Definition in Tag File' was implemented. The
function is scanning tag file and reporting all symbols defined in
your project and matching searched string. In difference to 'Search
symbol in Tag File' this function returns only symbols defined in your
project, not symbols defined in headers or jar archives. Also, only
the name of the symbol is matched against the searched string, not its
profile.

- Delete parameter refactoring is checking for accidental deletion of
used parameter.

- Encapsulate field is now processing only those occurrences of field,
where the field becomes inaccessible after making it private.  New
refactoring 'Self encapsulate field' was added. Self encapsulate field
replaces all occurrences of the field by get-set methods (which was in
fact the original behavior of 'Encapsulate field').

- When an argument manipulation refactoring (insert, delete, reorder)
is invoked on a constructor, Xrefactory automatically resolves the
ambiguity: Constructor <-> Class in favor of constructor.

- Improvements in project creation Emacs macro and several bug fixes
in compile-run macro.

- Problem with '' in URLs when generating HTML documentation under
MS-Windows has been fixed. You can now generate HTML under Windows and
then move it to a Unix server.

- New options '-htmlcutsuffix' and '-htmllinenumlabel=<label>' are
available. Using them you can generate Java html files fitting to
JavaDoc from Jdk 1.4.

- Indexing of large projects should be faster thanks to new hashing
function.

- Xrefactory now handles correctly C macros with ellipsis, such as:
#define DPRINF(format, ...) fprintf(debug, format, __VA_ARGS__)
and #include followed by a macro. Those constructions are allowed
in ANSI C'99 standard.



# Version 1.5.2

- pull-up / push-down method with corrections of 'this' references.

- completion works also for explicit constructor invocations (in the
first line of a constructor) after 'this' and 'super' keywords.
Completion shows all possible parameters for constructors.

- "Rename symbol" refactoring checks for accidental application on
class or package and referring you to specialized forms
'xref-rename-class or 'xref-rename-package.

- Options in .xrefrc which are enclosed in double-quotas can contain
newline character.  Thanks to this it is now possible to enter more
complex scripts for compilation and program runs. If compile or run
option contains more than one line, a temporary shell (or .bat) script
is created and executed.

-  Options can contain special predefined variables for entering
problematic characters: ${dq} for double-quotas; ${dl} for dollar
sign; ${pc} for percent sign and also ${nl} for newline (even if
newline can now be entered directly in strings).

- The word assert is by default not recognized as a keyword. This is
due to compatibility reasons with programs written in java 1.3 and
sooner.  By default, you can use variables or methods named 'assert'.
If your program is written for java 1.4, then you should put option
"-java1.4" into your .xrefrc. This option causes that the word assert
will be recognized as keyword and assert commands will be correctly
processed. However, you will not be able to define methods named
assert anymore.

- Maximal number of listed completions is now customizable.

- numerous small bug-fixes.

# Version 1.5.1

- bug fix in indexation of .jar archives.

- possible confusion when class is imported by both general
import-on-demand and single-type-import is fixed.

- multiple Emacs frames are now correctly supported.  It is now
possible to browse, complete, etc independently in all opened
frames. It is also possible to fix a list of references in one frame
and link them to source code displayed in another frame.

- in nearly all Xrefactory dialogs (completion list,
search-in-tag-file, symbol resolution dialog and in class tree) it is
now possible to inspect definition or browse Javadoc of displayed
symbols.  This function is bind to either button2 or Shift-button2.

- function 'search-in-foreign-tag-file' was added. It is a shortcut
allowing to scan Tag file of another project without need to switching
to it. This function switch to another project, scans its tag file and
then switch back to original project. It is supposed that the foreign
project contains some large database of symbols (probably a huge .jar
archive indexed).

- Shift left-arrow and Shift right-arrow keys are binded to scroll
left and right in completion and reference list buffers. This will
allow Emacs users to see whole completions (source lines) if they are
longer than current screen.

- a possibility to add general import (i.e. import-on-demand with .*
at the end) directly from comletions has been added to completion
pop-up menu.

- multiple invocations of xref-completions will scroll down the list
of proposed completions (if longer then size of its window).

# Version 1.4 -> 1.5

New refactorings for Java language:

- Move static field
- Move static method
- Turn virtual method to static
- Turn static method to virtual
- Move field
- Encapsulate field
- Pull up field
- Push down field
- Pull up method
- Push down method
- Rename (move) package

Refactorings are equipped by security checks. If a refactoring is done
without warning messages it is safe and behavior preserving. Note that
'Move method' refactoring can be done by turning the method static,
moving it and finally turning it back virtual.

Fully qualified type name completions for Java. If completing a simple
type name which is not imported, a fully qualified name is proposed in
completions. For example completion of 'LinkedLi' proposes
'java.util.LinkedList' string.

Several new special context completions for Java. A completion after
package declaration completes this package name (directory with cut
sourcepath). A completion in the class declaration completes this file
name. A completion (of an empty string) on a method definition place
will propose whole definitions of inherited methods including
profiles.

Improvements in (X)Emacs IDE interface. A possibility to define
several alternatives for run command allows to define divers initial
input arguments for your program run.

A 'build' function allows compilation followed (in case of success) by
execution of your program.

Configuration file can refer to environment variables as well as to
define its own variables.

Possibility to customize Xrefactory (X)Emacs options via 'customize
package.

A simple e-lisp function 'xref-find-file-on-mouse' is bind to
Shift-button2 mouse event. It opens a file pointed by mouse cursor. It
may be useful if after a run of your program you receive a stack
dump. It helps you to move directly to your source file (and line)
referred from the stack dump.

A new option -exactpositionresolve is available for C
programmers. This option controls how symbols which are local to a
compilation unit, but usually used in several files, are linked
together. This concerns symbols like macros, structures and their
records, etc. Such symbols have no link names passed to linker (like
global functions have). If the -exactpositionresolve option is present
then such symbols are considered to be the same only if their
definitions come from the same header file and if they are defined at
the same position in the file (in other words if this is a single
definition in the source code). Otherwise two symbols are considered
to be same iff they have the same name. This is a very powerful
feature because it allows perfect resolution of browsed symbols and
allows to safely rename one of symbols if a name clash occurs.


# Version 1.3 -> 1.4

"Xref-Speller" has been renamed to "Xrefactory".

Class Tree. It is now possible for Java users to show curent
inheritance hierarchy in separate window or in separate frame.
Automatic update of completion database. Intellisense code completion
database is updated after any modification of a source file. In
particular it means that it is no longer necessary to recompile or
update tag file in order that changes appear in completions.
Refactoring's Undo. It is now possible to undo a suite of
refactorings. The undo is built upon existing Emacs undo mechanism,
but it is able to switch buffers at beginnings of refactorings.

Several improvements in project management.

A safety check detects name clashes introduced by renaming.

Many new browsing functions are available. For example:

- Local motion in source. Using those functions you can simply move to
  the previous (next) reference of a symbol without need to push its
  references onto the reference stack.

- Repushing of lastly poped references.

- Pushing through class tree. This allows more fine selection of
  references to browse also for class fields, not only for mehods as
  it was previously. This may be very usefull for "push down field"
  refactorings.

- Possibility to kill running xref process. If something is going
  wrong you can kill the extern process during creation or update of
  tag file. You can also kill the xref-server process if you feel that
  it is running into an inconsistent state (the next browsing action
  will restart it).



# Version 1.2 -> 1.3

Filters can be applied on showed reference list. There are 4 filter
levels for now. Their meaning depends on the type of the browsed
symbol. If the browsed symbol is a variable the filter levels are
following.

- Level 3: only definition and declarations are shown.
- Level 2: as level 3 plus l-value usages.
- Level 1: as level 2 plus usages when address of the variable is got.
- Level 0: all references.

If the browsed symbol is a type then the filter levels are:

- Level 3: Only definitions and declarations are shown;
- Level 2: as level 3 plus usages in the EXTENDS and IMPLEMENTS clauses (meaningfull only for Java);
- Level 1: as level 2 plus all usages in the top level scope (global vars and function definitions);
- Level 0: all references are shown.

When pushing references of an ambiguous symbol an additional dialog is
offered to resolve the symbol manually. This can happened either for
symbols inside a macro body or for Java virtual methods. In the case
of Java the 'Push references' action do not show the dialog if there
is only one 'usage appropriate' reference. Others pushing actions show
the dialog each time when there is an ambiguity. A 'usage appropriate'
reference is the method definition if you start browsing on a method
invocation.

Filters are applied also on the symbol resolution dialogs. If the
dialog for manual symbol resolution is shown, you can filter the
listed symbols by following filters (remember that the list is
generated relative to the symbol you have chosen to browse and that
this dialog occurs practically only in resolution of Java virtual
methods):

- Level 0: Only references of methods having the same profile as the
  selected one and beeing from a class related to the class of the
  selected one.
- Level 1: All symbols having the same profile.
- Level 0: All symbols of the same name.


New environment variable JDKCLASSPATH is used for setting '.jar'
archives implicitly used by the JDK tools. For example, jdk1.2 do not
require that the file 'rt.jar' is listed in the CLASSPATH variable,
however this file is used by compiler and interpreter. In order to
permit Xrefactory using those implicit archives, you have to list them
in the JDKCLASSPATH variable. The format of the JDKCLASSPATH value is
the same as of the CLASSPATH variable. For example on most linux/unix
systems using jdk1.2.2 from Sun you should set this variable on the
"/usr/local/jdk1.2.2/jre/lib/rt.jar" value.

Note: You do not need to set JDKCLASSPATH if using JDK from Sun
Microsystems, only if using JDK from some other provider not having
the same distribution structure.

A new option -jdkclasspath is available to override the JDKCLASSPATH
variable from command line (see also the previous point).

New options are available for generated HTML files. Options
'-htmlcutpath=', '-htmllinkcolor=' and '-htmlnounderline' control
respectively pathes to be cutted when generating file hierarchy under
-htmlroot, set the color of generated links and avoid links to be
underlined.

Xref now handles situations when C pre-processor macro is using the `##`
gluing construction and you browse a symbol which is glued to some other.

Under Emacs when you call a browsing function with cursor at the
begining of the buffer, then references on this buffer are pushed
rather than the references of the first symbol of the buffer. If you
wish to browse all the places when the current file is included, just
go to the begining of the file and call any reference pushing
function.


# Version 1.1 -> 1.2

Full integration with built-in C pre-processor is implemented. The
include-included relation is stored in cross references and can be
browsed when positioning the cursor on the '#include' keyword.

The semantics of '-update' option is changed. From now it will reload
not only modified files, but also (recursively) all files including
those files. A new option '-fastupdate' implements the old fashioned
update (re-parsing only modified files).

The code completion function as well as symbol resolution now works
when writing a macro body in a header file (meaning also when the
macro is not invoked in this header file).

A possibility of multiple passes through the sources is
implemented. The '-pass' markers (used in a '.xrefrc' file) allow that
the same source file can be processed for different initial macro
settings and that all the references are collected in a single TAG
file. More info in the 'xrefrc' manual page.

Two new options controlling generated HTML are implemented. The
'-htmllinenums' causes that lines will be numbered and the
'-htmlnocolors' causes that keywords will not be highlighted. The last
one can considerably decrease the size of generated HTML files.

A possibility to set "dynamically" input source files is
implemented. If your .xrefrc file contains a shell command enclosed in
`` apostrophes, the command will be executed and its standard output
will be interpreted as names of files to process. Usually you will use
this with the 'find' unix comand.

Ordering of references according to l-value and r-value usages is no
more performed in on-line browsing. From now (as it was before the
version 1.0), the definition goes first, than go declarations and
finally all other usages ordered by their positions. If you are
interested in browsing only l-value usages, you should list all
references and check the 'usage character' (the first char of the
line).

# Version 1.0 -> 1.1

A symbol retrieval function is available under Emacs/XEmacs
editors. It permits to search a symbol name in the whole TAG file.  It
is now possible to compress generated HTML files. Two options
'-htmlzip=' and '-htmlsuffix=' can be used to set respectively the
zipping command and the suffix of zipped files.

# Version v.93 -> 1.0

Automatic extraction of a region into a new function/macro is now
possible under Emacs/XEmacs. The new function/macro is created from
the selected region by adding an automatically generated header. When
creating the header, Xrefactory takes into account full static analyse
of local variable life-times.

Xrefactory distinguishes between l-value (for example: x=5; &x;) and
r-value references (for example: f(x)) of a variable. References in
listings are ordered by descending "importance": the definition goes
first, then go declarations, l-value usages and finaly r-value usages.

The completion list is sensitive on mouse clicks under
Emacs/XEmacs. Middle button click permits you to inspect the
definition place of the symbol, right button selects the symbol for
completion.

The completion and reference lists are now colored under
Emacs/XEmacs. Coloring can be switched off by editing the
emacs/xrefin.el configuration and key-binding file.

The '.xrefrc' file can now contain names of source files (or
directories with the '-r' option). This information is used by the new
Emacs/XEmacs macro 'xref-create-refs' (invocable from menu bar as
'Create Xref Tags File'). This macro re-creates the cross-reference
file from scratch.

Xrefactory now understands the YACC-file format (suffix '.y'). It
understands both grammar and C semantic action parts.

In HTML output it is possible to generate horizontal bars separating
function definitions. Bars generation is triggered by the option
'-htmlfunseparate'.

The '-brief' option is default from now. You can switch it off with
the '-nobrief' option.

# Version v.92 -> v.93

Argument manipulation macros are available in Emacs/XEmacs. They
provide global source manipulations such as adding, deleting and
exchanging arguments of a function or a cpp-macro.

Searching of an identifier in context sensitive completions is
available under Emacs/XEmacs. This permits to find for example all
functions usable in the current context with name containing given
string or to find all applicable methods (direct or inherited)
containing given string.

New option '-r' is available. If a directory name is given to xref as
argument and the '-r' option is present then xref searches recursively
all files and subdirectories of that directory. It process all files
having the '.c' and '.java' suffixes (note that '.h' are useless as
they are processed when included from '.c').

For Java users the option '-classpath <CLASSPATH>' is now
available. When present it overrides the value of the CLASSPATH
environement variable.

# Version v.91 -> v.92

Browsing of corresponding #if((n)def)-#elif-#else-#fi directives is
now possible. When positionning the cursor on an #elif, #else or #fi
directive you can move directly to the corresponding #if((n)def)
directive.

Xref is now capable to generate HTML files from your source
files. When invoked with the '-html' option it converts source files
into HTML format. Output files are stored in file hierarchy under the
directory specified by the '-htmlroot=<dir>' option. The
'-htmltab=<number>' option can be used to set tabulator value in
output files.

'xrefinstall' script can be used to install Xref into system
directories of the computer.

# Version 0.9 -> v.91

New option '-refnum=<number>' is available. It causes that the
reference file is splitted into the <number> smaller files. If the
<number> is greater than 1 then the file specified by the '-refs
<file>' option is interpreted as directory name, where reference files
will be stored.

When Xref searches references of a given symbol, only one of reference
files is read. In consequence '-refnum' option makes browsing much
faster for large projects. It is recomennded to specify the <number>
proportionally to the size of your project (aproximately one cross
reference file per 5 000 lines of code). The default value is
'-refnum=1'.

NOTE: If you change the 'refnum' constant you have to re-CREATE your
cross reference files.

The possibility to list all references of given symbol is now
available under Emacs/XEmacs. The list is created in a separate
window, each line contains the reference in the form <file>:<line
number>:<content of the line>. Selecting such a line will move you
directly to this reference in the source code window. The new macro
can be invoked from the Emacs/XEmacs menu bar, or by the 'M-x
xref-listCxrefs' command.

# Version 0.8 -> 0.9

Personal '.xrefrc' file placed in user's HOME directory is the
preferred way of passing options to Xref.

The 'Grey+' and 'Grey-' keys are released under Emacs/XEmacs. You
should use 'F4' and 'F3' keys instead. If you wish to continue using
original keys you have to edit 'xrefin.gel' file in your editor
subdirectory of Xref distribution and then run 'sh init' script.

# Version 0.7 -> 0.8

Xref now can read the 'classes.zip' Java archive file. However it is
still unable to read compressed archives. If you are using some, you
will need to zip them with no compression.

# Version 0.6 -> 0.7

Semantics of the '-update' command line option changed! Now, the 'xref
-update file1 ...' command do not only updates references for the
explicitly mentioned files 'file1 ...', but also for all modified
files mentioned in the cross reference file. For example, the command
'xref -update' will first read the cross references file, then it
reparse modified files. In practice it means that you can update cross
reference file by just typing the 'xref -update' command.

NOTE: You have to CREATE the cross reference file with the version 0.7
(and later) in order to make the '-update' option works properly!

NOTE2: Only references of really modified files are updated. It means
that the 'include-included' relation is not taken into accounts.

If no input file is given xref provide no action! This is different to
previous versions, where the standard input was scanned as being an
input file.

The directory structure for source distribution changed! This concerns
only users, using the 'xref-any' source distribution. From now, the
xref source distribution is placed in the 'xref-any' directory, not in
the 'xref' directory. The 'xref' directory is then created
automatically and it contains only the distribution for the given
platform (no sources). If you are updating the source distribution
from older revision, please modify your path setting to this new
configuration (it means change the path '.../xref/xref' to
'.../xref').

New setup utility!

# Version 0.5 -> 0.6

Under Emacs/XEmacs you can now use the 'safe renaming' macro. It can
be invoked from the Emacs/XEmacs menu bar, or by the 'M-x xref-rename'
command.

Three special 'program commentary' lexems '/*&', '&*/' and '//&' are
recognized by Xref and are ignored (meaning they do not cause putting
parts of program into commentary during cross-referencing). For
example, in the code:

    void fun(int i) {
        //& printf( "entering fun(%d)n" , i );
        ...
        /*& printf( "leaving fun(%d)n" , i ); &*/
    }

the occurrences of the variable 'i' in the 'printf' calls will be
included into cross-references (and so renamed, ...), even when they
are commented for standard compiler.

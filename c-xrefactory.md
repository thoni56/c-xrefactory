# C-xrefactory for Emacs and jEdit

- [General]
- [Completion]
- [Browser]
- [Symbol Retriever]
- [Refactorer]
- [Refactorings]
- [Feedback]
- [License]


# General

C-xrefactory is a refactoring development environment for C/Yacc.
Its functions can be accessed via the 'c-xref' menu in Emacs and the
'Plugins &gt; C-xrefactory' submenu of the main menu bar in jEdit.
Optionally the most frequent functions can be bound to shortcuts.
C-xrefactory can also be customized via standard Emacs and jEdit
customization dialog.

> **NOTE** — Some of these descriptions refers to the jEdit plugin (not
> supported and may or may not work).


## Quick start

To quickly start working with C-xrefactory just invoke any of its
functions.  You will be asked for creation of a project.  Then
C-xrefactory will create its database for the project and is ready for
use.


## Projects

C-xrefactory is project based, you will need to create and maintain
description of your projects. Particularity of C-xrefactory projects
is possibility of autodetection.  C-xrefactory detects the 'active
project' from the path of currently edited file.  Each project
contains list of 'autodetection directories' specifying when the
project should be triggered 'active'.  Only one project at the time
can be active.  In consequence 'autodetection directories' have to be
distinct for all projects.  Information about the selected 'active
project' is displayed after each browsing action in the bottom
information line of the editor.

> **NOTE** — Project autodetection directories are (in general) not the
> same as directories containing project files.  Project files can contain
> common libraries used in many projects, while project detection
> directory should be the directory which is project specific,
> i.e. directory containing project specific files not shared with other
> projects.


## ReferenceDB

The main part of C-xrefactory are the 'ReferenceDB'.  Each project has
its references stored in special files specified by the user.  They
contain information about your project sources.  In particular,
information about all symbols, their linking properties, place(s) of
definition and all usages.  The ReferenceDB can be split into any
number of files.  The number of files is specified by user.  A larger
number of files makes browsing faster, but makes creation and update
slower.  The maintenance of the ReferenceDB is to a large extent part
automatic.  However they may become inconsistent, and when it does it
is recommended to re-create the ReferenceDB manually.


## C-xref task

C-xrefactory cooperates with an external task 'c-xref'.  If you feel
that the task has run into an inconsistent state, you can invoke the
'Kill c-xref task' function.


# Completion

Completion is implemented with the function 'Complete', in Emacs
usually bound to the the <kbd>F8</kbd> key.  Completion tries to
complete identifier before the caret by contextually appropriate
strings.  C-xrefactory is recognizing different context among them,
completion of function parameters, variable definitions, etc.  When
several possibilities are available a completion dialog appears.


## Completion Dialog

Each line of the dialog contains following informations: the
identifier to insert and the full definition of the symbol.

Symbol can be selected using standard motion keys.  The following
special keys are available:

- <kbd>return</kbd> — close dialog and insert completion.
- <kbd>space</kbd> — inspect definition of the symbol.
- <kbd>escape</kbd> — return to invocation place and close dialog.


# Browser

C-xrefactory browser allows resolving any symbol in source file,
inspect its definition and all usages.  Following four basic functions
are available for browsing and it is usual to bind them to hot-keys:

- Browser &gt; Push Symbol and Goto Definition
- Browser &gt; Pop Symbol
- Browser &gt; Previous Reference
- Browser &gt; Next Reference.

When browsing a symbol, you first need to activate it by moving the
caret on an occurrence of the symbol and invoke 'Push Symbol and Goto
Definition' function.  In the case of success the caret is moved to
the definition of the symbol. This is because inspecting definition is
the most usual browsing function and it is performed together with the
resolution. In case you are trying to browse a virtual method and the
unique definition can not be determined by static analysis, the
[Browser Dialog] is opened for manual resolution.
After successful resolution you can inspect all usages of the symbol
using 'Previous' and 'Next Reference' functions.

Browser allows simultaneous browsing of multiple symbols. For example,
let's imagine you are browsing all usages of a variable 'variable' and
you see that it is used as a parameter of a method 'method'.  So, you
wish to find the definition of the 'method'. In this situation simply
put caret on the 'method' and invoke 'Push Symbol and Goto Definition'
once more time. You will be moved to the definition of the 'method'
and you can inspect its code. After this you wish to continue browsing
remaining usages of the 'variable'. But when you try to use 'Previous'
and 'Next Reference' you see that those functions are now inspecting
all usages of the 'method' (not 'variable' as you wish).  In this
situation you first need to invoke the function 'Backward'. After
this, the 'Previous' and 'Next Reference' functions works with usages
of the 'variable'.

As illustrated by this example, C-xrefactory browser is based on
browser stack. Newly browsed symbols are pushed on the top of the
stack by the function 'Push Symbol and Goto Definition'. Functions
'Previous' and 'Next Reference' are inspecting previous and next usage
of the symbol placed on the top of the stack. Function 'Backward' is
removing one element from the top of the stack.

It is possible to push back an element which was popped from the stack
by error. This is done with the function 'Browser &gt; Forward'.  This
function is particularly useful when using visualization of the
browser stack in the Browser Dialog.

Another way how to push references onto browser stack is the function:
'Browser &gt; Push Name and Goto Definition'. This function takes a
string pointed by the caret, scans the ReferenceDB and pushes ALL symbols
having this name. No semantic information such as scopes or
overloading resolution is taken into account. This is why this
function is useful to find definition of a known symbol rather than to
browse an existing code.  Also this function does not permit browsing
of local symbols, such as local variables, parameters and labels, as
they are not recorded in global ReferenceDB.

## Browser Dialog

Browser dialog allows visualization of C-xrefactory browser stack by
displaying its top element.  It contains two major information panes.
Information displayed in information panes can be filtered by
selecting filter from combo boxes placed above.


### Toolbar

Browser toolbar contains four buttons putting new symbol(s) onto browser
stack.

- **Push Symbol and Goto Definition**. This function parses the
    buffer opened in the editor pane and resolves the symbol pointed
    by the caret. This symbol is then pushed onto the browser
    stack. Then the caret is moved to the definition of the
    symbol. This is the most used browsing function.  If you want to
    browse a symbol from your program, this is the function to be
    used. Other functions are used for few specific cases when this
    function can not do the job.

- **Push Name and Goto Definition**. This function takes the
    identifier pointed by the caret in the editor pane of jEdit and
    pushes onto the stack all global symbols having this name. No
    semantic information is examined, no overload or other resolution
    is made. This function allows to browse only references of global
    project symbols, it does not allow to browse local variables,
    method parameters and other local symbols.

- **Browse File Local Unused Symbols**. This function parses the
    buffer opened in the editor pane and pushes onto browser stack all
    unused symbols with local scope. Those are local variables, method
    parameters, but also labels and import statements, so this
    function can be used to purge imports.

- **Browse Global Unused Symbols**. This functions scans references and
    pushes them onto the browser stack.


Four other functions are accessible from the Toolbar:

- **Backward**. Go back to the previously browsed symbol.

- **Forward**. Go forward to the symbol (if any) from where you went back.

- **Previous Reference**. Move to the previous reference of currently browsed symbol.

- **Next Reference**. Move to the next reference of currently browsed symbol.


### Browse name

The browse name text field can be used to enter manually a name to be
pushed onto the browser stack. This may be used when you wish to
browse a symbol which you do not see in your source code. Otherwise,
it is more natural to use either "Push Name and Goto Definition" or
"Push Symbol and Goto Definition" function.

### Symbol Pane

The symbol information pane contains symbol names, profile
informations and where those symbols are defined. Window is organized
as tree. The tree shows the location of the symbols definition
followed the number of references.

|   |   |
|---|---|
| &lt;mouse-left-button&gt;              | select only one reference and inspect definition.  |
| &lt;mouse-right-button&gt;             | toggle select/unselect.  |
| &lt;SHIFT&gt;&lt;mouse-left-button&gt; | toggle select/unselect. |

**Available filters**:

|   |   |
|---|---|
| Equal name    | all symbols of given name are displayed. Tree is not restricted. |
| Equal&nbsp;profile | the browsed symbol is displayed. Tree is not restricted.  |
| Relevant      | the browsed symbol is displayed. Tree does not display references not related to the browsed symbol.  |

### References Pane

The references pane contains list of references. Each reference is
listed together with corresponding line of source code. The list is
selectable and selected reference is automatically opened in editor.

Meaning of **filters for classes**:

|   |   |
|---|---|
| Level&nbsp;3 | Only definitions and declarations are shown.  |
| Level&nbsp;2 | As level 3 plus usages in the EXTENDS and IMPLEMENTS clauses  |
| Level&nbsp;1 | As level 2  plus all usages  in the top level  scope (in global vars and method definitions). This can be used  to see all the methods working with a particular type. |
| Level&nbsp;0 | All references are shown. |


Meaning of **filters for variables**:

|   |   |
|---|---|
| Level&nbsp;3 | Only definitions and declarations are shown.  |
| Level&nbsp;2 | As level 3 plus l-value usages. |
| Level&nbsp;1 | Not used. |
| Level&nbsp;0 | All references are shown. |


# Symbol Retriever

C-xrefactory symbol retriever is useful for finding forgotten symbol
names and for finding symbols from third parties libraries.  You enter
a string to search and C-xrefactory scans the references for
matches. All symbols matching entered string are reported.


Entered strings are interpreted as shell expressions and are composed
from a sequence of characters possibly containing wildcard
characters. The following wildcard characters can be used:

- `*` expands to any (possibly empty) string
- `?` expands to any single character
- `[...]` expands to one of the enclosed characters

Ranges of characters can be included between `[` and `]`,
so for example `[a-zA-Z]` matches any letter, `[0-9]` matches any
digit, as per usual in shell expressions. If the first character
following the `[` is a `^` then the meaning of the expansion is
inverted, for example `[^0-9]` expands to any non-digit character.  A
symbol is reported only if it completely matches the searched string.
Method profile is considered as part of the name of the method, for
example, the expression `*(*int*)` will report all methods taking at
least one parameter of type int.  Letters are considered case
insensitive except letters enclosed between `[` and `]`.


For example the expression `*get*` will report all symbols containing
the string 'get', for example symbols `getField` and `Target` will match.
Expression `get*` will report all symbols starting by the string
'get'. Expression `[A-Z]*` will report all symbols starting by an upper
case letter. Expression `get[abc0-2]*` will report all symbols starting
by the string 'get' followed by one of characters a,b,c,0,1,2 followed
by any (possibly empty) string, so for example `getact` will match, but
`getAccount` will not.


If you enter an expression which does not contain any of the wildcard
characters `*`, `?` or `[` then C-xrefactory reports all symbols
containing the entered string.  For example, entering `get` as the
expression is equivalent to entering `*get*`.


# Refactorer

Refactoring is a software development and maintenance process where
the source code is changed in such a way that it does not alter the
external behaviour. C-xrefactory offers automatic support for several
general refactoring patterns available via refactorer function.
Whenever it is possible it also checks that performed modifications do
not change program behaviour. For example, in case of symbol renaming,
C-xrefactory checks whether renamed symbol does not clash with an
existing one, etc.

Invocation of refactorings will display a pop up menu with
refactorings available for symbol pointed by the caret. Selection of
one of items will perform the refactoring.

Here follows the list of refactorings implemented by C-xrefactory:

- [Rename Symbol]
- [Add Parameter]
- [Delete Parameter]
- [Move Parameter]
- [Extract Function/Macro]
- [Set Target for Next Moving Refactoring]


# Refactorings

## Rename Symbol

**Description**: Change the name of a program symbol

**Example**:

_Before refactoring_:

```c
for (int a=0; a<args.length; a++) {
    action(args[a]);
}
```

_After refactoring_:

```c
for (int i=0; i<args.length; i++) {
    action(args[i]);
}
```

**Refactoring Context**: Cursor has to be on the symbol.

**Input parameters**: New name of the symbol (for example: 'i')

**Mechanics**:  Replace  old  symbol name  by  the  new  name on  all  its
        occurrences in the project.

## Add Parameter

**Description**: Add parameter to a method, function or macro.

**Example**:

_Before refactoring_:

```c
public int method(int x) {
    if (x<=1)
	    return 1;
    return method(x-1)+method(x-2);
}
```

_After refactoring_:

```c
public int method(int x, int y) {
    if (x<=1)
	    return 1;
    return method(x-1, 0)+method(x-2, 0);
}
```

**Refactoring Context**: Cursor  has to be on the  method's (function's or
        macro's) name.

**Input parameters**:  Position of the new parameter,  its declaration and
        default value.  (for example: '2', 'int y' and '0').

**Mechanics**: Inspect  all references of  the method (function  or macro)
        and add  declaration of the  new parameter to  each definition
        and default value to each invocation of the method.

## Delete Parameter

**Description**: Delete parameter of a method, function or macro.

**Example**:

_Before refactoring_:

```c
public int method(int x, int y) {
    if (x<=1)
	    return 1;
    return method(x-1, 0)+method(x-2, 0);
}
```

_After refactoring_:

```c
public int method(int x) {
    if (x<=1)
	    return 1;
    return method(x-1)+method(x-2);
}
```

**Refactoring Context**: Cursor  has to be on the  method's (function's or
        macro's) name.

**Input parameters**:  Position of the  parameter to delete  (for example:
      '2').

**Mechanics**: Inspect  all references of  the method (function  or macro)
        and remove the parameter.

## Move Parameter

**Description**: Reorder parameter of a method, function or macro.

**Example**:

_Before refactoring_:

```c
public int method(int x, int y) {
    if (x<=1)
		return 1;
    return method(x-1, 0)+method(x-2, 0);
}
```

_After refactoring_:

```c
public int method(int y, int x) {
    if (x<=1)
	    return 1;
    return method(0, x-1)+method(0, x-2);
}
```

**Refactoring Context**: Cursor  has to be on the  method's (function's or
        macro's) name.

**Input parameters**: Old and new positions of the parameter (for example:
      '1' and '2').

**Mechanics**: Inspect all references of  the method and move the parameter
        from its original to its new position.

## Extract Method/Function/Macro

**Description**: Extract region into new method (function or macro).

**Example**:

_Before refactoring_:

```c
int main(int argc, char *argv[]) {
    int n,x,y,t;
    n = atoi(argv[1]);
    x=0; y=1;
    for (int i=0; i<n; i++) {
        t=x+y; x=y; y=t;
    }
    sprintf("%d-th fib == %d", n, x);
}
```

_After refactoring_:

```c
static int fib(int n) {
    int x, y, t;
    x=0; y=1;
    for (int i=0; i<n; i++) {
        t=x+y; x=y; y=t;
    }
    return x;
}

int main(int argc, char *argv[]) {
    int n,x,y,t;
    n = atoi(argv[1]);
    x = fib(n);
    sprintf("%d-th fib == %d", n, x);
}
```

**Refactoring Context**: The code for extraction has to be selected within
            the editor.

**Input Parameters**: Name of the new function or macro.

**Mechanics**: Copy the region  before the function, generate new
           header  and footer  based on  static analysis  of  code and
           generate call to the new method at the original place.

## Set Target for Next Moving Refactoring

**Description**: Set target position for moving refactorings

**Refactoring Context**: Cursor has to be on the position where the functio,
        or variable will be moved.

**Input Parameters**: None.


# Feedback

Any feedback is welcome at the [c-xrefactory GitHub repo].


# License

You can read the license at the [c-xrefactory GitHub repo][repo license].


<!----------------------------- REFERENCE LINKS ------------------------------>

[c-xrefactory GitHub repo]: https://github.com/thoni56/c-xrefactory "Visit the c-xrefactory repository at GitHub"
[repo license]: https://github.com/thoni56/c-xrefactory/blob/main/LICENSE "View c-xrefactory license file"

<!-- in-doc x-refs -->

[Add Parameter]: #add-parameter
[Browser Dialog]: #browser-dialog
[Browser]: #browser
[Completion]: #completion
[Delete Parameter]: #delete-parameter
[Extract Function/Macro]: #extract-methodfunctionmacro
[Feedback]: #feedback
[General]: #general
[License]: #license
[Move Parameter]: #move-parameter
[Refactorer]: #refactorer
[Refactorings]: #refactorings
[Rename Symbol]: #rename-symbol
[Set Target for Next Moving Refactoring]: #set-target-for-next-moving-refactoring
[Symbol Retriever]: #symbol-retriever


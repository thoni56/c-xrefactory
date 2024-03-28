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

C-xrefactory is a refactoring development environment for C and Java.
Its functions can be accessed via the 'c-xref' menu in Emacs and the
'Plugins &gt; C-xrefactory' submenu of the main menu bar in jEdit.
Optionally the most frequent functions can be bound to shortcuts.
C-xrefactory can also be customized via standard Emacs and jEdit
customization dialog.

> **NOTE** — Some of these descriptions refers to the jEdit plugin (not
> supported and may or may not work) and/or are relevant only when
> browsing Java.


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

Completion is implemented with the function 'Complete', in jEdit
usually bound to 'Control Space' hot-key, in Emacs the <kbd>F8</kbd> key.
Completion tries to complete identifier before the caret by
contextually appropriate strings.  C-xrefactory is recognizing around
15 different context among them, completion of class attributes,
classes, methods parameters, variable definitions, etc.  When several
possibilities are available the completion dialog appears.


## Completion Dialog

Each line of the dialog contains following informations: the
identifier to insert, the inheritance level and the class where
proposed symbol is defined, and the full definition of the symbol.

Symbol can be selected using standard motion keys.  The following
special keys are available:

- <kbd>return</kbd> — close dialog and insert completion.
- <kbd>space</kbd> — inspect definition (or javadoc) of the symbol.
- <kbd>escape</kbd> — return to invocation place and close dialog.

When completing a type name, fully qualified names of classes from jar
archives can also be proposed.  When selecting such name, an
additional dialog appears proposing you to add new import clause.


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
informations and classes where those symbols are defined. Window is
organized as tree, where after each symbol follows inheritance subtree
relevant for this symbol. Tree contains classes defining the symbol
and (sub) classes where symbol is syntactically referred. After the
class name follows a number of references.

|   |   |
|---|---|
| &lt;mouse-left-button&gt;              | select only one class references and inspect definition reference (or javadoc).  |
| &lt;mouse-right-button&gt;             | toggle select/unselect.  |
| &lt;SHIFT&gt;&lt;mouse-left-button&gt; | toggle select/unselect. |
| &lt;CTRL&gt;&lt;mouse-left-button&gt;  | inspect class (or its javadoc). |

**Available filters**:

|   |   |
|---|---|
| Equal name    | all symbols of given name are displayed. Class tree is not restricted. |
| Equal&nbsp;profile | the browsed symbol is displayed. Class tree is not restricted.  |
| Relevant      | the browsed symbol is displayed. Class tree does not display classes not related to class of browsed symbol.  |

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
| Level&nbsp;1 | Not used in Java language. |
| Level&nbsp;0 | All references are shown. |


# Symbol Retriever

C-xrefactory symbol retriever is useful for finding forgotten symbol
names and for finding symbols from third parties libraries.  You enter
a string to search and C-xrefactory scans the references (optionally
also jar archives from the class path) for matches. All symbols
matching entered string are reported.


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

- [Rename Symbol/Class/Package]
- [Add Parameter]
- [Delete Parameter]
- [Move Parameter]
- [Extract Method/Function/Macro]
- [Expand Names]
- [Reduce Names]
- [Set Target for Next Moving Refactoring]
- [Move Static Field/Method]
- [Move Class]
- [Move Class to New File]
- [Move Field]
- [Pull Up/Push Down Field/Method]
- [(Self) Encapsulate Field]
- [Turn Virtual Method to Static]
- [Turn Static Method to Virtual]


# Refactorings

## Rename Symbol/Class/Package

**Description**: Change the name of a program symbol

**Example**:

_Before refactoring_:

```java
for(int a=0; a<args.length; a++) {
    action(args[a]);
}
```

_After refactoring_:

```java
for(int i=0; i<args.length; i++) {
    action(args[i]);
}
```

**Refactoring Context**: Cursor has to be on the symbol.

**Input parameters**: New name of the symbol (for example: 'i')

**Mechanics**:  Replace  old  symbol name  by  the  new  name on  all  its
        occurrences in the project. In case of public class stored in a
        file  of the  same  name, also  rename  the file.  In case  of
        package also store files in new directories.RM110-039-53484

## Add Parameter

**Description**: Add parameter to a method, function or macro.

**Example**:

_Before refactoring_:

```java
public int method(int x) {
    if (x<=1) return(1);
    return(method(x-1)+method(x-2));
}
```

_After refactoring_:

```java
public int method(int x, int y) {
    if (x<=1) return(1);
    return(method(x-1, 0)+method(x-2, 0));
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

```java
public int method(int x, int y) {
    if (x<=1) return(1);
    return(method(x-1, 0)+method(x-2, 0));
}
```

_After refactoring_:

```java
public int method(int x) {
    if (x<=1) return(1);
    return(method(x-1)+method(x-2));
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

```java
public int method(int x, int y) {
    if (x<=1) return(1);
    return(method(x-1, 0)+method(x-2, 0));
}
```

_After refactoring_:

```java
public int method(int y, int x) {
    if (x<=1) return(1);
    return(method(0, x-1)+method(0, x-2));
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

```java
public static void main(String[] args) {
    int i,n,x,y,t;
    n = Integer.parseInt(args[0]);
    x=0; y=1;
    for(i=0; i<n; i++) {
        t=x+y; x=y; y=t;
    }
    System.out.println("" + n + "-th fib == " + x);
}
```

_After refactoring_:

```java
static int fib(int n) {
   int i, x, y, t;
   x=0; y=1;
   for(i=0; i<n; i++) {
        t=x+y; x=y; y=t;
   }
   return(x);
}

public static void main(String[] args) {
   int i,n,x,y,t;
   n = Integer.parseInt(args[0]);
   x = fib(n);
   System.out.println("" + n + "-th fib == " + x);
}
```

**Refactoring Context**: The code for extraction has to be selected within
            the editor.

**Input Parameters**: Name of the new method (function or macro).

**Mechanics**: Copy the region  before the method (function), generate new
           header  and footer  based on  static analysis  of  code and
           generate call to the new method at the original place.

## Expand Names

**Description**: Expand types to fully qualified names.

**Example**:

_Before refactoring_:

```java
package com.xrefactory.refactorings;
import javax.swing.\*;
class Hello {
    public static void main(String argv[]) {
        JOptionPane.showMessageDialog(null, "Hello world");
    }
}
```

_After refactoring_:

```java
package com.xrefactory.refactorings;
import javax.swing.\*;
class Hello {
    public static void main(java.lang.String argv[]) {
        javax.swing.JOptionPane.showMessageDialog(null, "Hello world");
    }
}
```

**Refactoring Context**: Cursor has to be on a definition of a class.

**Input Parameters**: None.

**Mechanics**: Replace short type names by fully qualified names.

## Reduce Names

**Description**: Reduce fully qualified type names to short form.

**Example**:

_Before refactoring_:

```java
package com.xrefactory.refactorings;
class Hello {
    public static void main(java.lang.String argv[]) {
        javax.swing.JOptionPane.showMessageDialog(null, "Hello world");
    }
}
```

_After refactoring_:

```java
package com.xrefactory.refactorings;
import javax.swing.JOptionPane;
class Hello {
    public static void main(String argv[]) {
        JOptionPane.showMessageDialog(null, "Hello world");
    }
}
```

**Refactoring Context**: Cursor has to be on a definition of a class.

**Input Parameters**: None.

**Mechanics**: Replace fully qualified names  by short names.  If the type
           is  not imported then add either import on demand or single
           type import command.

## Set Target for Next Moving Refactoring

**Description**: Set target position for moving refactorings

**Refactoring Context**: Cursor has to be on the position where the field,
            method or class will be moved, pulled up or pushed down.

**Input Parameters**: None.

## Move Static Field/Method

**Description**: Move static field or method to another place.

**Example**:

_Before refactoring_:

```java
class Target {
    static int i=0;
}

class Source {
    static int j=1;
    public static void method() {
        System.out.println("i, j == " + Target.i + ", " + j);
    }
    public static void main(String[] args) {
        method();
    }
}
```

_After refactoring_:

```java
class Target {
    static int i=0;
    public static void method() {
         System.out.println("i, j == " + i + ", " + Source.j);
    }
}

class Source {
    static int j=1;
    public static void main(String[] args) {
        Target.method();
    }
}
```

**Refactoring  Context**:  Target  place  has  to set  using  'Set  Target
            Position',  cursor has  to be  on the  name of  the method
            (field) to be moved (at its definition).

**Input Parameters**: None.

**Mechanics**: Move  the method (field), adjust all  references within its
           body  (initialisation)  and   all  its  references  in  the
           project.

## Move Class

**Description**: Move class from one place to another.

**Example**:

_Before refactoring_:

```java
class A {
    static int i;
    static class B {
        static void method() {
            System.out.println("i==" + i);
        }
    }
}
```

_After refactoring_:

```java
class A {
    static int i;
}

class B {
    static void method() {
        System.out.println("i==" + A.i);
    }
}
```

**Refactoring  Context**:  Target  place  has  to set  using  'Set  Target
            Position', cursor has to be on the name of the class to be
            moved (at its definition).

**Input Parameters**: None.

**Mechanics**: Move the class and adjust all its references.

## Move Class to New File

**Description**: Move class into its own file.

**Example**:

_Before refactoring_:

file A.java:

```java
package pack;

class A {
    static int i;
    static class B {
        static void method() {
            System.out.println("i==" + i);
        }
    }
}
```

_After refactoring_:

file A.java:

```java
package pack;

class A {
    static int i;
}
```

file B.java:

```java
package pack;

public class B {
    static void method() {
        System.out.println("i==" + A.i);
    }
}
```

**Refactoring Context**: Cursor  has to be on the name of  the class to be
            moved (at its definition).

**Input Parameters**: Name of the file to create

**Mechanics**: Create new file, add package and imports and move the class
           and adjust all its references.

## Move Field

**Description**: Move field from one class to another

**Example**:

_Before refactoring_:

```java
class Target {

}

class Source {
    Target link;
    int field;
    public int method() {
        return(field);
    }
}
```

_After refactoring_:

```java
class Target {
    int field;
}

class Source {
    Target link;
    public int method() {
        return(link.field);
    }
}
```

**Refactoring  Context**:  Target  place  has  to set  using  'Set  Target
            Position', cursor has to be on the name of the field to be
            moved (at its definition).

**Input Parameters**: the field pointing from source class to target class
      (in the example: 'link').

**Mechanics**: Move the  field, inspect all its references  add insert the
           field pointing from source to target.

**Comment**:  This  refactoring  relies  on  semantic  properties  of  the
         program,  it can not  be ensured  to be  100% safe  by static
         analysis of the program.

## Pull Up/Push Down Field/Method

**Description**: Move method (field) up (down) in the class hierarchy.

**Example**:

_Before refactoring_:

```java
class SuperClass {
    int x = 0;
    int y = 0;

}

class InferClass extends SuperClass {
    int y = 1;

    void method() {
        System.out.println("x == " + x);
        System.out.println("this.x == " + this.x);
        System.out.println("this.y == " + this.y);
        System.out.println("super.y == " + super.y);
    }

    public static void main(String args[]) {
        (new InferClass()).method();
    }
}
```

_After refactoring_:

```java
class SuperClass {
    int x = 0;
    int y = 0;

    public void method() {
        System.out.println("x == " + x);
        System.out.println("this.x == " + this.x);
        System.out.println("this.y == " + ((InferClass)this).y);
        System.out.println("super.y == " + this.y);
    }
}

class InferClass extends SuperClass {
    int y = 1;

    public static void main(String args[]) {
        (new InferClass()).method();
    }
}
```

**Refactoring  Context**:  Target  place  has  to set  using  'Set  Target
            Position',  cursor has  to be  on the  name of  the method
            (field) to be moved (at its definition).

**Input Parameters**: None.

**Mechanics**:  Move the  method  and adjust  references  inside its  body
           (initialisation).

## (Self) Encapsulate Field

**Description**: Generate field accessors and their invocations.

**Example**:

_Before refactoring_:

```java
class EncapsulateField {
    public int field;

    void incrementField() {
        field = field + 1;
    }
}

class AnotherClass extends EncapsulateField {
    void printField() {
        System.out.println("field == " + field);
    }
}
```

_After refactoring_:

```java
class EncapsulateField {
    private int field;
    public int getField() {return field;}
    public int setField(int field) { this.field=field; return field;}

    void incrementField() {
        setField(getField() + 1);
    }
}

class AnotherClass extends EncapsulateField {
    void printField() {
        System.out.println("field == " + getField());
    }
}
```

**Refactoring  Context**:  Cursor has  to  on the  name  of  the field  to
             encapsulate (on its definition).

**Input Parameters**: None.

**Mechanics**: Generate getter and  setter methods. Inspect all references
           of the field and  replace them by appropriate accessor. The
           Self Encapsulate field (in difference to Encapsulate field)
           processes  also references  within the  class  defining the
           field (see example). The  Encapsulate Field will left those
           references untouched.

## Turn Virtual Method to Static

**Description**: Turn method into static.

**Example**:

_Before refactoring_:

```java
class Project {
    Person[] participants;
}

class Person {
    int id;
    public boolean participate(Project proj) {
        for(int i=0; i<proj.participants.length; i++) {
            if (proj.participants[i].id == id) return(true);
        }
        return(false);
    }
}
```

_After refactoring_:

```java
class Project {
    Person[] participants;
    static public boolean participate(Person person, Project proj) {
        for(int i=0; i<proj.participants.length; i++) {
            if (proj.participants[i].id == person.id) return(true);
        }
        return(false);
    }
}

class Person {
    int id;
}
```

**Refactoring Context**:  Cursor has to  on the name  of the method  to be
             turned static (on its name).

**Input  Parameters**: The  name of  the  new parameter  (in the  example:
       'person').

**Mechanics**:  Add new parameter to  the method  passing its invocation
           object.  Make  all accesses to method's object via the new
           parameter.  Declare the method static and make all method
           invocations static.

**Comment**: This refactoring is usually used to move virtual method from
         one class to another in the  sequence 'Turn Virtual Method
         into Static', 'Move Static  Method' and 'Turn Static Method
         into Virtual'.

## Turn Static Method to Virtual

**Description**: Make the method virtual.

**Example**:

_Before refactoring_:

```java
class Target {
    int field;
    static int method(Source ss) {
        System.out.println("field==" + ss.link.field);
    }
}

class Source {
    Target link;
    static void main(String[] args) {
        Target.method(new Source());
    }
}
```

_After refactoring_:

```java
class Target {
    int field;
    int method() {
        System.out.println("field==" + field);
    }
}

class Source {
    Target link;
    static void main(String[] args) {
        new Source().link.method();
    }
}
```

**Refactoring Context**:  Cursor has to  on the name  of the method  to be
             turned virtual (on its name).

**Input Parameters**: The parameter containing method's object. Optionally
      a  field getting  method's  object from  the  parameter. In  the
      example: '1' and "link".

**Mechanics**:  Remove the  'static' keyword,  inspect all  references and
           apply  method on  the method's  object (method's  object is
           determine from the combination of parameter and field).

**Comment**: This refactoring  is usually used to move  virtual method from
         one  class to another  in the  sequence 'Turn  Virtual Method
         into Static',  'Move Static  Method' and 'Turn  Static Method
         into Virtual'.

# Feedback

Any feedback is welcome at the [c-xrefactory GitHub repo].


# License

You can read the license at the [c-xrefactory GitHub repo][repo license].


<!----------------------------- REFERENCE LINKS ------------------------------>

[c-xrefactory GitHub repo]: https://github.com/thoni56/c-xrefactory "Visit the c-xrefactory repository at GitHub"
[repo license]: https://github.com/thoni56/c-xrefactory/blob/main/LICENSE "View c-xrefactory license file"

<!-- in-doc x-refs -->

[(Self) Encapsulate Field]: #self-encapsulate-field
[Add Parameter]: #add-parameter
[Browser Dialog]: #browser-dialog
[Browser]: #browser
[Completion]: #completion
[Delete Parameter]: #delete-parameter
[Expand Names]: #expand-names
[Extract Method/Function/Macro]: #extract-methodfunctionmacro
[Feedback]: #feedback
[General]: #general
[License]: #license
[Move Class to New File]: #move-class-to-new-file
[Move Class]: #move-class
[Move Field]: #move-field
[Move Parameter]: #move-parameter
[Move Static Field/Method]: #move-static-fieldmethod
[Pull Up/Push Down Field/Method]: #pull-uppush-down-fieldmethod
[Reduce Names]: #reduce-names
[Refactorer]: #refactorer
[Refactorings]: #refactorings
[Rename Symbol/Class/Package]: #rename-symbolclasspackage
[Set Target for Next Moving Refactoring]: #set-target-for-next-moving-refactoring
[Symbol Retriever]: #symbol-retriever
[Turn Static Method to Virtual]: #turn-static-method-to-virtual
[Turn Virtual Method to Static]: #turn-virtual-method-to-static

# C-Xref task used as server for editing

## Introduction

When c-xref is invoked with option "-task\_regime\_server", it acts like a server reading input options from standard input and producing answers on the standard output.

Because on majority of OS, pipes are slower than temporary files, two pipes (standard input and output) are used only for small quantity of data and their main purpose is synchronisation between the editing environment and the c-xref task.

The c-xref task expects a suite of input options finished by the "end-of-options" string.
Options are basically those which can be used on command line (with exception of -I and -D options).
Moreover numerous special options not documented in the c-xref manual page can be used.
One of options should specify the output file for the answer.
In such case the c-xref task computes the answer, stores it into the output file and then writes the "<sync>" string on its standard output.

NOTE:

If you wish to examine the exact communication between c-xref task and Emacs editor, you can edit the c-xref.el file and change the line:

    (defvar c-xref-debug-mode nil)

to:

    (defvar c-xref-debug-mode t)

This will make Emacs display all informations exchanged with c-xref task in the *Messages* buffer.



## Example

The simplest example of c-xref comunication protocol is an invocation of a completion function.
Let's consider that we have a file t.c containing the following text:

```
----------------------------------------------------------------------
struct toto {int x,y;} t;

main() {
  t.
  ;
  ma
}
----------------------------------------------------------------------
```

The following sequence of commands will call c-xref server task, ask for
all completions after "t." string at line 4 (offset 40), then it will
ask for completion on line 5 (offset 49).

```
vittek:~/tmp>~/c-xref/c-xref -task_regime_server -xrefactory-II
-olcxcomplet
-olcursor=40
/home/vittek/tmp/t.c
end-of-options

<no-project-found len=20>/home/vittek/tmp/t.c</no-project-found>
<all-completions nofocus=0 len=19>x  :int x
y  :int y</all-completions>
<sync>


-olcxcomplet
-olcursor=49
/home/vittek/tmp/t.c
end-of-options

<no-project-found len=20>/home/vittek/tmp/t.c</no-project-found>
<goto>
<position-lc line=6 col=2 len=20>/home/vittek/tmp/t.c</position-lc>
</goto>
<single-completion len=4>main</single-completion>
<sync>
```

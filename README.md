[![Build Status](https://travis-ci.com/thoni56/c-xrefactory.svg?branch=master)](https://travis-ci.com/thoni56/c-xrefactory)[![Coverage Status](https://coveralls.io/repos/github/thoni56/c-xrefactory/badge.svg?branch=master)](https://coveralls.io/github/thoni56/c-xrefactory?branch=master)[![codecov](https://codecov.io/gh/thoni56/c-xrefactory/branch/master/graph/badge.svg)](https://codecov.io/gh/thoni56/c-xrefactory)
# C-xrefactory - A refactoring tool for C/Java and Emacs

## TL;DR

`c-xrefactory` is a free Emacs refactoring tool and code browser for
_C_ (& _Java_ and _Yacc_).

Easiest way to install is using `el-get` (see NOTE below):

    M-x el-get-install<ENTER>c-xrefactory<ENTER>

Place cursor on an identifier and `go to definition` (F-6), navigate
between occurrences (F-3, F-4), refactor (F-11) with `rename`, mark
some code and `extract function`. Your C programming and
code will never be the same. Some highlights:

- navigate through all usages of any type of symbol
- rename any type of symbol, variable, macro, parameter, or Yacc grammar rule
- add or delete function parameter
- extract function or macro
- detect unused symbols locally or in the complete project

When installed correctly there will be an Emacs menu called `C-xref`
for easy access to most functions.

## Tutorial

Once you have that menu in Emacs you can try the Tutorial under
`C-xref->C-xref Misc`.  It will automatically create the tutorial
project files in a temporary directory and open the initial file.
Then just follow the instructions for a quick walk-through of what
`c-xrefactory` can do.

## Setup

If your Emacs runs in a "standard" *ux-like environment with `bash`,
`make`, `cc` etc. you probably don't need anything in particular.

Just make sure that you have a `zlib-devel` installed as the current
`c-xrefactory` install using `el-get` does not automatically compiles
it own.

This will make Emacs/c-xrefactory work on Linux, including WSL,
Cygwin, Msys2-MSYS (but not Msys2-MingW*) and on MacOS/Darwin.

For Windows I really recommend doing your development in WSL
(especially WSL2). You can setup `c-xrefactory` as described above and
run Emacs in a terminal window without any problems.

To use Emacs in a graphical environment (in an X11 window) you need a
Windows X11 server like
[VcXsrv](https://sourceforge.net/projects/vcxsrv/) (my
recommendatioin), x410 (from the Windows Store),
[Xming](https://sourceforge.net/projects/xming/)) or, if you already
have MobaXterm, that also comes with an X11 server. Then set it the
server according to [these
instructions](https://stackoverflow.com/a/61110604/204658).

## Features and notes

TBD. Some documentation is [here](https://github.com/thoni56/c-xrefactory/blob/master/doc/c-xrefactory.md).

### Yacc special features

C code inside semantic actions in a Yacc file will be analysed so that
you can (mostly) navigate symbol even inside them.

Non-terminals (rule names) can be navigated and completed as if they
were any other symbol. Furthermore the special symbol `$<n>`, where
<n> is a number referencing symbol <n> in the rule can also be
navigated. This is extra handy with F6 to be sure that you referenced
the terminal or non-terminal that you meant to.

Unfortunately there are no refactorings of Yacc rules possible (but
that would be a cool project!). But you can of course do all the C
refactorings you want from inside the C code in the semantic actions.

### Java notes

The current Java grammar has not been updated from Java 1.4, but since
`c-xrefactory` recovers gracefully that is not a major problem except
for the fact that you can't for certain navigate all symbols inside
construct that was introduced in later Java versions.

You need to have a JRE installed so that it can be found. If it's
Java 8 it will also be parsed for all Java standard library
symbols. Unfortunately later versions does not store JRE in a
standardized format (which was Jar-files with class files) so
`c-xrefactory` cannot read it. You can still compile with Java > 8 and
use `c-xrefactory` with a Java 8 JRE.

## Help!

This code is pre-historic legacy. It seems to have been born in a
world of copy-paste-hack-and-debug, there were no unit tests, or any
tests at all for that matter.

I'm on a long-time quest to resurrect this code to understandable,
maintainable and developable standard. This is hard, difficult and
time consuming, as it is almost impossible to understand what anything
does. At this point we have reached almost 60% test coverage which
makes me confident in refactoring many things.

If you think you can help, I'll be happy to take it, even if it is
only adding one line of understanding in the wiki...


## The Story

Once apon a time there was [http://www.xref.sk](http://www.xref.sk), a
site promoting possibly the worlds first refactoring browser to cross
the ["Refactoring's
Rubicon"](http://martinfowler.com/articles/refactoringRubicon.html). That
site seems to be going up and (mostly) down and there has been no
support for many years, but it seems that
[http://www.xrefactory.com/](http://www.xrefactory.com/) is now a
better and more stable URL to the original Xrefactory, where it is
still available.

At that time it had a free Java and C version, and a paid C++
version. Development seems to have been headed by a [Marián
Vittek](http://dai.fmph.uniba.sk/w/Marian_Vittek/en).

Marián made a C-version, c-xref, available under GPL already 2009 on
[SourceForge](http://sourceforge.net/projects/c-xref/). The reason
seems to have been to allow parallell installations of free and
non-free versions. `c-xref` seems to be intended to be limited to C
and Emacs. Actually it's not. It is all but identical to the original
full/free C/Java refactoring browser from
[http://www.xref.sk](http://www.xref.sk). So Java is also supported,
at least Java 1.3(?).

## About This Clone

As I'm almost dependent on this and refactoring tools for C is sadly
lacking, I decided to pick this up and work a bit on it, maybe even
make it a [bit more visible](http://sourceforge.net/projects/c-xref/),
by moving it to GitHub.

No-one would be more happy than me if the C++ version where also put
in the public domain.

If someone has contact with Marián, please inform him and ask him to
contact me. Perhaps we can create something great from this together
with others who might be interested.

NOTE: You can easily install `el-get` in your running emacs using a
snippet from [`el-get`s github
repo](https://github.com/dimitri/el-get), but don't forget to add the
`el-get` pieces in you emacs init.

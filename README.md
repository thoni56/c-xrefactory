[![Build Status][travis badge]][travis link]&nbsp;
[![Coverage Status][coveralls badge]][coveralls link]&nbsp;
[![codecov][codecov badge]][codecov link]

# C-xrefactory — A refactoring tool for C/Yacc and Emacs

## TL;DR

`c-xrefactory` is a free Emacs refactoring tool and code browser for
_C_ & _Yacc_.

> [!CAUTION]
> `c-xrefactory` is still under radical refactorings and restructuring, and
> might be missing some tests, so occasional hiccups are possible. I use it almost
> everyday and the stable version, which you get if you install as per below, has passed
> all tests. But YMMV.

## Features

- navigate to definition and all usages of it of any type of symbol, including `#include`
- search and browse for all symbols
- rename any type of symbol - variable, field, type, macro, parameter, or Yacc grammar
  rule
- add, delete or re-order function and macro parameter
- rename included file
- extract function, macro or variable
- move function to another file adding extern declaration and include of header file
- detect unused symbols in a file or in the complete project
- more to come!

## Install

### From repo:

```
git clone https://github.com/thoni56/c-xrefactory
emacs -l c-xrefactory/load.el
```

>[!NOTE]
>You can clone the repo anywhere and use a full path to `load.el`.

### Using `el-get`:

    M-x el-get-install<ENTER>c-xrefactory<ENTER>

> [!TIP]
> You can easily install `el-get` in your running emacs using
> a snippet from [`el-get`'s GitHub repo][el-get repo], but don't forget
> to add the `el-get` pieces to your emacs init.


## Example Use

Place cursor on an identifier and `go to definition` (<kbd>F6</kbd>),
navigate between occurrences (<kbd>F3</kbd>, <kbd>F4</kbd>), refactor
(<kbd>F11</kbd>) with `rename`, mark some code and `extract function`.

Your C programming and code will never be the same. Some highlights:

When installed correctly there will be an Emacs menu called `C-xref`
for easy access to most functions.

## Tutorial

Once you have that menu in Emacs you can try the Tutorial under
`C-xref->C-xref Misc`.  It will automatically create the tutorial
project files in a temporary directory and open the initial file.
Then just follow the instructions for a quick walk-through of what
`c-xrefactory` can do.

## Setup

If your Emacs runs in a "standard" *ix-like environment with `bash`,
`make`, `cc` etc. you probably don't need anything in particular.

`c-xrefactory` works with recent Emacsen on Linux, including WSL,
Cygwin, Msys2-MSYS (but not Msys2-MingW*) and MacOS/Darwin.

For Windows I really recommend doing your development in WSL
(especially WSL2). You can setup `c-xrefactory` as described above and
run Emacs in a terminal window without any problems.

To use Emacs in a graphical environment from WSL (in an X11 window) you
need a Windows X11 server like [VcXsrv] (my recommendation), [X410]
(from the Windows Store), [Xming] or, if you already have [MobaXterm],
that also comes with an X11 server. Then setup the server according
to [these instructions][X11 WSL2].

## Features and notes

_WORK IN PROGRESS!_

Whatever documentation there is can be found [here][our docs]. We are
using C4 and Structurizr to document the architecture as best we can.

### Yacc special features

C code inside semantic actions in a Yacc file will be analysed so that
you can (mostly) navigate symbols even inside them.

Non-terminals (rule names) can be navigated and completed as if they
were any other symbol. Furthermore the special symbol `$<n>`, where
&lt;n&gt; is a number referencing symbol &lt;n&gt; in the rule can also
be navigated. This is extra handy with <kbd>F6</kbd> to be sure that
you referenced the terminal or non-terminal that you meant to.

Unfortunately there are no refactorings of Yacc rules possible (but
that would be a cool project!). But you can of course do all the C
refactorings you want from inside the C code in the semantic actions.

### LSP

As noted in one issue, a natural development would be to support LSP
(The [Language Server Protocol]), but that is still far out of reach.
A lot of refactoring needs to be done to clean up the current modules
and protocol to get closer to that point.

Experimental work has been started and it is possible to hook up
the `c-xrefactory` server to an LSP client. The basic communication
works but no features are yet accessible.

### Java notes

> [!IMPORTANT]
> Java support has now been removed since there are far
> better options now. The text below does not apply and is kept
> only for historical/nostalgic reasons. It will be removed soon.

The current Java grammar has not been updated from Java 1.4, but since
`c-xrefactory` recovers gracefully that is not a major problem except
for the fact that you can't for certain navigate all symbols inside
construct that was introduced in later Java versions.

You need to have a JRE installed so that it can be found. If it's
Java 8 it will also be parsed for all Java standard library
symbols. Unfortunately later versions does not store JRE in a
standardized format (which was Jar-files with class files) so
`c-xrefactory` cannot read it. You can still compile with Java &gt; 8
and use `c-xrefactory` with a Java 8 JRE.

NOTE: As there are good Java tools available with refactoring support
I'm thinking of removing all Java support from `c-xrefactory` since
that complicates a lot of things. And some of the cool solutions don't
work any longer, like navigating Java library symbols by loading the
jar-files that were part of any Java install up to Java 8.

## Help!

This code is pre-historic legacy. It seems to have been born in a
world of copy-paste-hack-and-debug, there were no unit tests, or any
tests at all for that matter.

I'm on a long-time quest to resurrect this code to understandable,
maintainable and developable standard. This is hard work, difficult
and time consuming, as it is almost impossible to understand what
anything does. At this point we have reached around 80% test coverage
which makes me confident in refactoring many things, and able to
add new features.

If you think you can help, I'll be happy to take it, even if it is
only adding one line of understanding in the wiki...

## Development and Documentation

There are some information about my current understanding of this
beast in the design documentation. And there is a budding user
manual. Those documents are continually published into the
[C-xrefactory documentation site](https://thoni56.github.io/c-xrefactory).

These documents are in no means complete, or perhaps not even
readable, but are attempts to continually collect bits and pieces of
information from old documents and newly found knowledge.

The information stored in the wiki has been transferred into these
documents and the wiki has been removed.

## The Story

Once apon a time there was [www.xref.sk], a site promoting possibly the
worlds first refactoring browser to cross the ["Refactoring's Rubicon"].
That site seems to be going up and (mostly) down and there has been no
support for many years, but it seems that [www.xrefactory.com] is now a
better and more stable URL to the original Xrefactory, where it is
still available.

At that time it had a free Java and C version, and also a paid C++
version. Development seems to have been headed by a [Marián Vittek].

Marián made a C-version, `c-xref`, available under GPL already 2009 on
[SourceForge][c-xref]. The reason seems to have been to allow parallel
installations of free and non-free versions. `c-xref` seems to be
intended to be limited to C and Emacs. Actually it was not. It was all
but identical to the original full/free C/Java refactoring browser
from [www.xref.sk] except for extensive name changes. So Java was also
still supported, at least Java 1.4.

During many years I have worked on this off and on to improve it and
extend the support for more C refactorings. It is still in flux but
fully usable.

## About This Clone

As I'm almost dependent on this and refactoring tools for C is sadly
missing, I decided to pick this up and work a bit on it, and maybe even
[give it more visibility][c-xref], by moving it to GitHub.

No-one would be more happy than me if the C++ version was also put
in the public domain.

If someone is in contact with Marián, please inform him and ask him to
contact me. Perhaps we can create something great from this, together
with others who might be interested.

<!------------------------- REFERENCE LINKS -------------------------->

[el-get repo]: https://github.com/dimitri/el-get "Vist el-get repository at GitHub"
[Language Server Protocol]: https://microsoft.github.io/language-server-protocol "LSP website"

<!-- xrefactory -->

[our docs]: https://thoni56.github.io/c-xrefactory "Read our C-xrefactory documentation"

["Refactoring's Rubicon"]: http://martinfowler.com/articles/refactoringRubicon.html "Read the 'Crossing Refactoring's Rubicon' article, by Martin Fowler (2001)"
[c-xref]: http://sourceforge.net/projects/c-xref/ "c-xref project at SourceForge"
[www.xref.sk]: http://www.xref.sk "Xrefactory old website (unreachable)"
[www.xrefactory.com]: http://www.xrefactory.com "Xrefactory website"

<!-- X11 Windows OS -->

[MobaXterm]: https://mobaxterm.mobatek.net "MobaXterm website"
[VcXsrv]: https://sourceforge.net/projects/vcxsrv/ "VcXsrv project at SourceForge"
[X11 WSL2]: https://stackoverflow.com/a/61110604/204658 "StackOverflow: How to set up working X11 forwarding on WSL2"
[X410]: https://www.microsoft.com/en-us/p/x410/9nlp712zmn9q "X410 page at MS Windows Store"
[Xming]: https://sourceforge.net/projects/xming/ "Xming project at SourceForge"

<!-- people -->

[Marián Vittek]: http://dai.fmph.uniba.sk/w/Marian_Vittek/en "Marián Vittek profile at Comenius University (Bratislava)"

<!-- badges -->

[travis badge]: https://app.travis-ci.com/thoni56/c-xrefactory.svg?token=3qJ2cLpEpe6KaXZ5PipE&branch=main "Travis CI build status"
[travis link]: https://app.travis-ci.com/thoni56/c-xrefactory
[coveralls badge]: https://coveralls.io/repos/github/thoni56/c-xrefactory/badge.svg?branch=main "Coveralls code coverage status"
[coveralls link]: https://coveralls.io/github/thoni56/c-xrefactory?branch=main
[codecov badge]: https://codecov.io/gh/thoni56/c-xrefactory/branch/main/graph/badge.svg "Codecov code coverage status"
[codecov link]: https://codecov.io/gh/thoni56/c-xrefactory

[![Build Status](https://travis-ci.org/thoni56/c-xrefactory.svg?branch=master)](https://travis-ci.org/thoni56/c-xrefactory)[![Coverage Status](https://coveralls.io/repos/github/thoni56/c-xrefactory/badge.svg?branch=master)](https://coveralls.io/github/thoni56/c-xrefactory?branch=master)[![codecov](https://codecov.io/gh/thoni56/c-xrefactory/branch/master/graph/badge.svg)](https://codecov.io/gh/thoni56/c-xrefactory)
# C-xrefactory - A refactoring tool for C/Java and Emacs

## TL;DR

`c-xrefactory` is a free emacs refactoring tool and code browser for
C (and Java).

Easiest way to install is using `el-get` (see note):

    M-x el-get-install<ENTER>c-xrefactory<ENTER>

Place cursor on an identifier and `go to definition` (F-6), navigate
between occurrances (F-3, F-4), refactor (F-11) with `rename`, mark
some code and `extract function` and so on. Your C programming and
code will never be the same.

When installed correctly there will be an Emacs menu called `C-xref`
for easy access to most functions.

## Help!

This code is pre-historic legacy. It seems to have been born in a
world of copy-paste-hack-and-debug, there were no unit tests, or any
tests at all for that matter.

I'm on a long-time quest to resurrect this code to understandable,
maintainable and developable standard. This is hard, difficult and
time consuming, as it is almost impossible to understand what anything
does. At this point I've added a couple of smoke/functional tests,
that at least should crash if something basic becomes broken.

If you think you can help, I'll be happy to take it, even if it is
only adding one line of understanding in _src/NOTES.md_...


## The Story

Once apon a time there was [http://www.xref.sk](http://www.xref.sk), a
site promoting possibly the worlds first refactoring browser to cross
the ["Refactoring's
Rubicon"](http://martinfowler.com/articles/refactoringRubicon.html).

At that time it had a free Java and C version, and a paid C++
version. Development seems to have been headed by a [Marián
Vittek](http://dai.fmph.uniba.sk/w/Marian_Vittek/en).

Marián have been hard to track down and
[http://www.xref.sk](http://www.xref.sk) seems to be going up and
(mostly) down and there has been no support for many years.

NOTE: It seems that
[http://www.xrefactory.com/](http://www.xrefactory.com/) is a better
and more stable URL to the original Xrefactory, where it is still
available.

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

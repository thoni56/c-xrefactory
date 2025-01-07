;;; c-xrefdoc.el - (X)Emacs interface to C-xrefactory (documentation)

;; Copyright (C) 1997-2004 Marian Vittek, Xref-Tech
;; Put under GPL in 2009 by Marián Vittek
;; Work resurrected and continued by Thomas Nilefalk 2015-
;; (https://github.com/thoni56/c-xrefactory)

;; This file is part of C-xrefactory software; it implements an
;; interface between c-xref task and (X)Emacs editors.  You can
;; distribute it under the terms of the GNU General Public License
;; version 2 as published by the Free Software Foundation.  You should
;; have received a copy of the GNU General Public License along with
;; this program; if not, write to the Free Software Foundation, Inc.,
;; 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  The
;; content of this file is copyrighted by Xref-Tech. This file does
;; not contain any code written by independent developers.

(defun c-xref-refactoring-documentation ()
  (let ((res))
    (setq res "


----------------------------------------------------------
* Rename Symbol:
----------------------------------------------------------


Description: Change the name of a program symbol

Example:

  Before refactoring:

        int main(int arc, char *argv[]) {
               for (int a=0; a<argc; a++) {
                       action(argv[a]);
               }
        }

  After refactoring:

        int main(int arc, char *argv[]) {
               for (int i=0; i<argc; i++) {
                       action(argv[i]);
               }
        }

Refactoring Context: Cursor has to be on the symbol.

Input Parameters: New name of the symbol (for example: 'i')

Mechanics: Rename all occurrences of the symbol in the scope of
      the symbol, which might be the whole project.



----------------------------------------------------------
* Add Parameter:
----------------------------------------------------------

Description: Add parameter to a method, function or macro.

Example:

        Before refactoring:

            public int method(int x) {
                if (x<=1)
                   return 1;
                return method(x-1)+method(x-2);
            }

        After refactoring:

            public int method(int x, int y) {
                if (x<=1)
                   return 1;
                return method(x-1, 0)+method(x-2, 0);
            }


Refactoring Context: Cursor has to be on the method's
      (function's or macro's) name.

Input Parameters: Position of the new parameter, its declaration
      and default value.  (for example: '2', 'int y' and '0').

Mechanics: Inspect all referenqces to the method (function or
      macro) and  add a declaration of the  new parameter to
      each definition and default value to each invocation.




----------------------------------------------------------
* Delete Parameter:
----------------------------------------------------------

Description: Delete parameter of a function or macro.

Example:

        Before refactoring:

            public int method(int x, int y) {
                if (x<=1)
                    return 1;
                return method(x-1, 0)+method(x-2, 0);
            }

        After refactoring:

            public int method(int x) {
               if (x<=1)
                   return 1;
               return method(x-1)+method(x-2);
            }


Refactoring Context: Cursor has to be on the name of the function
      or macro.

Input Parameters: Position of the parameter to delete (for
      example: '2').

Mechanics: Inspect all references to the function or macro and
      remove the parameter.


----------------------------------------------------------
* Rename Included File:
----------------------------------------------------------

Description: Rename an included file and update all references.

Example:

        Before refactoring:

            #include \"header.h\"

        After refactoring:

            #include \"constants.h\"

Refactoring Context: Cursor must be on an include statement for
      the file being renamed.

Input parameter: The new name of the file.

Mechanics: Rename the file and update all '#include' statements
      to refer to the new file.


----------------------------------------------------------
* Move Parameter:
----------------------------------------------------------

Description: Reorder parameter of a function or macro.

Example:

        Before refactoring:

            public int method(int x, int y) {
                if (x<=1)
                    return 1;
                return method(x-1, 0)+method(x-2, 0);
            }

        After refactoring:

            public int method(int y, int x) {
               if (x<=1)
                   return 1;
               return method(0, x-1)+method(0, x-2);
            }


Refactoring Context: Cursor has to be on the name of the function
      or macro.

Input Parameters: Old and new positions of the parameter
      (for example: '1' and '2').

Mechanics: Inspect all references to the method and move the
      parameter from its original position to its new position.



----------------------------------------------------------
* Extract Function:
* Extract Macro:
----------------------------------------------------------

Description: Extract region into new function or macro.

Example:

        Before refactoring:

            int main(int argc, char *argv[]) {
                int i,n,x,y,t;
                n = atoi(args[1]);
                x=0; y=1;
                for (int i=0; i<n; i++) {
                    t=x+y; x=y; y=t;
                }
                System.out.println(\"\" + n + \"-th fib == \" + x);
            }

        After refactoring:

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
                   System.out.println(\"\" + n + \"-th fib == \" + x);
            }


Refactoring Context: The code for extraction has to be selected
      in the editor.


Input Parameters: Name of the new method (function or macro).

Mechanics: Move the region out of the method (function), generate
      wrapper code based on static analysis of the original
      context and generate a call to the new method from the
      code's original position.


----------------------------------------------------------
* Set Target for Next Moving Refactoring:
----------------------------------------------------------

Description: Set target position for moving refactorings.

Refactoring Context: Cursor has to be on the position where the
      variable, function or macro will be moved.

Input Parameters: None.

Mechanics: This action does  not modify source code. It sets
      some internal values for future refactoring.

NOTE: Moving is not implemented yet.


----------------------------------------------------------
* Make Static Function Extern:
----------------------------------------------------------

Description: Make a static function public/extern.

Example:

        TBD.

Refactoring Context: Cursor has to on the name of the function
      be made extern (on its name).

Input Parameters: None.

Mechanics: Remove the 'static' keyword, add an 'extern' declaration
      in an appropriate header file.

Comment: Used as part of moving a function to another file/module.



----------------------------------------------------------
* Undo Last Refactoring:
----------------------------------------------------------

Description: Undo sequence of refactorings.

Mechanics: Undo all changes made in buffers up to and including
      the last refactoring. Optionally also undo previous
      refactorings.


")
    res
))

(provide 'c-xrefdoc)

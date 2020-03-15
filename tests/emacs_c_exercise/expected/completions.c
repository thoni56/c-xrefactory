#include <stdio.h>
#include <time.h>
#include <assert.h>

/*
 Check completion on following demonstration lines!  Each
 demonstration line is preceded by one line of explaining
 commentary. Put cursor at the end of demonstration lines and invoke
 completion by pressing F8.
*/


void completions() {
    struct mylist {int i; struct mylist *next;} *l,*ll;
    struct tm *ptm;

    // simple function name completion
    completions
    ;

    // completion from included files (requires correct include setting)
    sprintf
    ;

    // completion list
    fprintf
    ;
    /* In the proposed list:
         <return>            - select the completion.
         <space>             - inspect definition.
         C-q                 - return and close completion window
         letter & digits     - incremental search
         other characters    - leave completion window
         <escape>            - close completion window, no completion
       also:
         mouse-button12      - select the completion.
         mouse-button3       - pop-up menu for this item.
       Everywhere:
         F7                  - close C-Xrefactory's window
    */

    // structure records completion
    ptm->tm_sec
    ;

    // partial completion
    fpu
    ;

    // works inside complex expressions
    (*(5+(struct tm **)0x13e5f)[0]).tm_min
    ;

    // macro names completions
    assert
    ;


    // works in definitions of used macros
#define MACRO(xx) xx->tm_min
    MACRO(ptm)
    ;


    // also can shorten writing of list loops
    for(l=ll;l!=NULL; 
    // and
    for(l=ll; l!=NULL;l=l->next) {


}


/*
  F5 will bring you back to index
*/

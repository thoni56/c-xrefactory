/*
  Check completion on following demonstration lines! Each
  demonstration line is preceded by one line of explaining
  comment. Put cursor at the end of demonstration lines and invoke
  completion by pressing F8.
*/

int xp1;
int xp2;

typedef int ZPILE;
extern int zprint(ZPILE *, const char *, ...);

void completions() {
    struct mylist {int i; struct mylist *next;} *l,*ll;

    // simple function name completion
    com
    ;

    // completion list
    zp
    ;
    /* In the proposals list:
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
    ll->
    ;

    // partial completion
    x
    ;

    // works inside complex expressions
    (*(5+(struct mylist **)0x13e5f)[0]).
    ;

    // macro names completions
    ass
    ;


    // works in definitions of used macros
#define MACRO(xx) xx->
    MACRO(ptm)
    ;


    // also can shorten writing of list loops
    for(l=ll;
    // and
    for(l=ll; l!=NULL;


}


/*
  F5 will bring you back to index
*/

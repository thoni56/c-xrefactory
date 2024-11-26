#include <stdio.h>
#define xxxx(a) a*a

/*
  Check completion on following demonstration lines!  Each
  demonstration line is preceded by one line of explaining
  commentary. Put cursor at the end of demonstration lines and invoke
  completion by pressing F8.
*/

struct mytm {
    int tm_sec;                 /* Seconds.	[0-60] (1 leap second) */
    int tm_min;                 /* Minutes.	[0-59] */
    int tm_hour;                /* Hours.	[0-23] */
    int tm_mday;                /* Day.		[1-31] */
    int tm_mon;                 /* Month.	[0-11] */
    int tm_year;                /* Year	- 1900.  */
    int tm_wday;                /* Day of week.	[0-6] */
    int tm_yday;                /* Days in year.[0-365]	*/
    int tm_isdst;               /* DST.		[-1/0/1]*/
};

void completions() {
    struct mylist {int i; struct mylist *next;} *l,*ll;
    struct mytm *ptm;

    // simple function name completion
    com
    ;

    // completion from included files (requires correct include setting)
    sp
    ;

    // completion list
    x
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
    ptm->
    ;

    // partial completion
    fpu
    ;

    // works inside complex expressions
    (*(5+(struct mytm **)0x13e5f)[0]).
    ;

    // macro names completions
    xxx
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

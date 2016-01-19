/*

                Welcome in the Xrefactory testing program!

       Please move to the main() function and follow hints in commentaries

*/

#ifndef BORA_BORA

#include <stdio.h>

#define BORO_BORO
#define MACRO_VALUE_TEST        17432
#define MACRO_BODY_TEST(test)   (test->ignoreFlag)

struct testStr {
    int             ignoreFlag;  /* you can nest F6 calls on MACRO_VALUE_TEST
                                    of the following line, now,
                                    then return using two times F5. */
    struct testStr  array[MACRO_VALUE_TEST];     
    struct testStr  *firstSubterm;
    struct testStr  *secondSubterm;
};

typedef struct testStr     S_testStr;

int xfun(int arg1, S_testStr *arg2, float arg3) {
}

int totofun(int arg1, char *arg2) {
}

#if defined(BORO_BORO)


main(int argc, char **argv) {
    int        i,j,ii,ij;
    S_testStr  *pp;
    double     ignoreFlag;


    MACRO_BODY_TEST
    ;
    i+xfun(jj,pp->ignoreFlag+2,3);
    xfun(pp->ignoreFlag,pp,4.5);
    MACRO_BODY_TEST(pp);

    /*

                           HERE WE ARE !

    The first thing  you have to do (if  not yet done) is to  create a new
    Xrefactory project.  To do  this select "Xref->Project->New" item from
    the (X)Emacs  menu bar.  It will  pose you few  questions, just answer
    them  by  default  (proposed)  values. Then,  you  should  invoke  the
    "Xref->Create  Xref   Tags  File"  menu   item  in  order   to  create
    cross-references for this project.  The  F7 key deletes the new window
    after.  Note that any of functions presented in following examples can
    be accessed via  the 'Xref' menu item on the  top of your Emacs/XEmacs
    screen.

    */

    /*
                         CODE COMPLETION.
    */

    /*  positioning the cursor at the end of following lines
        and pressing F8 will insert  the missing part of the function name.
        Pressing F8 one more time gives you the info about the symbol's type
        (after, press F7 to delete the created window)
    */
    to
    ;

    /* following completion may require correct setting of your standard
       include directories. If it does not complete "sprintf", then invoke 
       "Xref->Project->Edit Option" menu item and insert "-I <include dir>" 
       option there (with <include dir> beeing the directory where your 
       compiler's standard header files are stored).
    */

    sp
    ;

    /*  positioning the cursor at the end of following lines
        and pressing F8 will give you the list of possible completions
        of given identifiers.
        Under X-Windows you can now click with middle button on any
        listed symbol in order to chose completion. You can also
        click right mouse button (in the completion window) in order 
        to get a popup menu with few other possible actions, among them
        a possibility to inspect definition places of proposed symbols.
        On some systems simultaneous pressing of both buttons works 
        instead of middle mouse button.
    */
    f
    ;
    i
    ;

    /*  positioning the cursor at the end of following lines
        and pressing F8 will give you information about structure records
        (after, press F7 to delete the created window)
    */
    pp->
    ;

    (*pp).i
    ;

    (*(5+(S_testStr **)0x13e5f)[ii]).
    ;

    /*  positioning the cursor at the end of the following line
        and pressing F8 will insert a partial completion, when pressing
        the F8 one more time, you will get the list of possible 
        completions (after, press F7 to delete the created window)
    */
    ar
    ;


    /*
                         SOURCE BROWSING.
    */

    /*  positioning the cursor on the following identifier
        and pressing F6 will move you to the definition of the
        'ignoreFlag' record. Then using repeatedly the F4
        and F3 you can move to the next and previous
        reference of this symbol. After all you can return by
        pressing F5 key. 
            F6 is pushing references of the selected symbol on a 
        reference stack, F5 pops them. It means, that you can hit
        F6 several times (without returning by F5) in order to inspect 
        several symbols, then pressing several times F5 will put you back
        to the original place.
    */
    pp->ignoreFlag
    ;                      /* structure record ignoreFlag */
    ignoreFlag
    ;                      /* local variable */
    MACRO_VALUE_TEST
    ;                      /* macro values */


    /*  positioning the cursor on an #if((n)def), #elif, #else, #endif
        cpp directive and pressing F6 will move you to the corresponding
        #if directive. You can then used F3 and F4 keys, to inspect
        related directives of the same level. As in the previous case
        the F5 key will return you back to the original place.
    */
#endif
#elif 0
    following 3 lines are never processed, however #if-#else-#fi works
#if 1
#else
#endif
#endif



    /*  You can list all references of
        the given symbol in a separate window. Just set the cursor on
        a symbol and then invoke
        "Xref -> Browsing with Symbol Stack -> Push and Show Top References"
        item from the menu (or invoke 'xref-listCxrefs' elisp function).
        This function is usually bind to 'C-F6' key. You can now move
        directly to any reference by clicking the middle mouse button on
        a specified line of this list. If you have no mouse you can do the
        same by placing the cursor on a reference and then pressing the
        ' ' <SPACE> key. The leftmost character of each line of the list
        indicates the type of
        the reference (*definition, +declaration, .address, ,lvalue,  rvalue).
        As in previous cases, you can move to the previous and next reference
        by F3 and F4 keys, as well as pop references and return back to
        the initial point by the F5 key. You can also restrict references
        by increasing or deacresing applied filter ('+' or '-' keys).
        For example, get the list of all references of the ignoreFlag.
    */
    pp->ignoreFlag
                     /* structure record */
    = ignoreFlag
                     /* local variable */
    ;
}
/*
    You can browse also the 'include <-> included' relation on files.
    Positioning the cursor on the '#include' directive and pressing 
    the F6 key (push references) moves you into the included file, 
    then inspecting other references (F3 and F4) will inspect all 
    includes of this file. Then you can return back by the F5 (pop 
    references) command. 
*/

#include <stdlib.h>

/*  Also when including through a macro expanded to a string (ISO/ANSI C'99) */

#define STDIO "stdio.h"

#include STDIO


main2(int argc, char **argv) {
    int        i,j,ii,ij;
    S_testStr  *pp;
    double     ignoreFlag;

    /*
                         SYMBOL RETRIEVALS.
    */



    /*  positioning the cursor on the following identifier 'scan'
        and invoking "Search symbol in -> Context Completions" Xref 
        menu item, or 'xref-search-in-completions' e-lisp 
        function, will search all possible completions, and displaying
        those containing (case insensitive) the string 'scan'.
        Similarly to completion you can select symbol with middle
        mouse button or get a popup menu with right button.
    */
    scan
    ;

    /*  positioning the cursor on the following identifier 'file'
        and invoking "Search symbol in -> TAG file" Xref menu item, 
        or the 'xref-search-in-tag-file' e-lisp
        function, will search all symbols containing the string 'file' in 
        the TAG file. There will be not only contextually adequate
        completions, but for example also all structure records
        of any structures with the name containing the string 'file'.
        Similarly to other functions you can select a symbol with middle
        mouse button or get a popup menu with right button.

    */
    file
    ;


    /*
                         REFACTORINGS.
    */


    /*                Symbol renaming.
        In order to rename a symbol, set the cursor on it and select the
        "Rename Symbol" menu item from 'Xref' menu (or 
        invoke the 'xref-rename' e-lisp function). For example,
        try to rename the 'ignoreFlag' structure record:
    */
    pp->ignoreFlag
    ;

    ignoreFlag           /* reference to local variable, will NOT be renamed */
       = pp->ignoreFlag  /* structure record, will be renamed */
         + 1.0;



    /*                 Argument manipulations.
       positioning the cursor on the following function name 'xfun'
       and selecting one of argument manipulation refactorings will
       apply this refactoring on all references of the given function.
       You will be prompted for the order of argument in cause,
       so just enter the corresponding number. For example to
       exchange the first and third argument, put the cursor on the
       'xfun' symbol, select the 
       "Argument Manipulations -> Exchange Arguments" menu item,
       insert '1' at the first prompt, insert '3' at the second 
       prompt. Then you should confirm with 'y' application
       of each operation.
       Similarly you can try to delete an argument 
       ("Argument manipulations -> delete argument") or simply browsing 
       the function references with having the cursor positionned on 
       a particular argument ("Argument manipulations -> 
       Manual Argument Edit").
    */

    xfun(1,2,3);


}

/*                 Function extraction.
    In the following function you can select the region marked
    by the //---- comments. Then you can invoke the extraction
    by selecting the "Extract region into a -> Function" menu item,
    or invoking the 'xref-extract-region-to-function' e-lisp
    function. You will be prompted for the name of the new 
    function (say 'fib'). After this the new function having the 
    original region as body will be created.
    The header and footer for the new function will be automatically 
    generated as well as invocation at the original position. 
*/

int fmain(int argc, char **argv) {
    int i,n,x,y,t;
    sscanf(argv[1],"%d",&n);
//----- begining of the selected region here
    x=0; y=1;
    for(i=0; i<n; i++) {
        t = x+y; x=y; y=t;
    }
//----- end of the selected region here
    fprintf("%d-th fib == %d\n", n, x);
}


    /*

    The presentation of  the most used Xrefactory functions  is over.  You
    can now check  Xrefactory on your project.  Do not  forget to open one
    of your files  and to create your project options  by "Xref -> Project
    -> New" menu item!

    You can access interactive help  for any Xrefactory function by typing
    "C-h k"  and then selecting  the corresponding function by  mouse from
    "Xref" Menu!

    */






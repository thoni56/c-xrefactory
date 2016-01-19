/*

               Welcome in the Xrefactory Java testing program!

    Please move to the 'demoFunction' method and follow hints in commentaries.

*/

import java.awt.*;
import javax.swing.*;

class Test {

  static JButton   myButton;
  MyList           mylist;
  Component        component;
  JTree            jtree;

  class MyList extends JList {
    public Component getComponentAt(int x, int y) {
      Component res = null;
      // ...
      return(res);
    }
  }

  int      classVar1;
  double   classVar2;

  int      onlyVarOfThisClassOnO;

  void          funNoArg()            {  }
  int           fun1Arg( int i )      { return(0); }

  int           fun1Arg( double x )   { return(1); }
  static int    fun1StaticNoArg()      { return(1); }
  static int    fun2StaticNoArg()      { return(1); }


  void demoFunction(int demoArg1, String demoArg2) {

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

                        CODE COMPLETION

        positioning the cursor at the end of following lines
        and pressing F8 (complete identifier) will give you the 
        list of all possible completions of given identifiers
        (after, press F7 to delete the created window)
        NOTE that each completion is divided into three 
        fields containing:
        1.) the string proposed for the completion, 
        2.) the class (if any) from where the symbol comes 
            (together with its 'inheritence level') 
        3.) the declaration of the symbol (i.e. its type and profile).

        Completion fields are active, with middle mouse button you can
        select the symbol, the right button offers you a pop up menu
        with several others function. On some systems simultaneous 
        pressing of both buttons works instead of button2.

    */

    fun
    ;
    classVar
    ;
    demo
    ;


    //  positioning the cursor at the end of following lines
    //  and pressing F8 will complete identifiers (as there is
    //  just one possible completion in this context).

    Sys
    ;
    System.o
    ;

    //  positioning the cursor at the end of the following line
    //  and pressing F8 will insert a partial completion, when pressing
    //  F8 one more time you will get the list of possible
    //  completions (after, press F7 to delete new window)

    System.out.pr
    ;

    //  in the following context the F8 key will help you to browse the
    //  CLASSPATH directories structure

    java.
    ;
    java.lang.System.
    ;

//      If the symbol to complete is yet complete an info about its
//      type is displayed. For example positioning the cursor at the
//      end of the following line and pressing F8 will show
//      all posible profiles for the 'println' method.

    System.out.println
    ;


    /*
                    SOURCE BROWSING
    */


    //  positioning the cursor on an identifier
    //  and pressing F6 will move you to the definition of the
    //  symbol. Then using repeatedly 'F4'
    //  and 'F3' keys you can move to the next and previous
    //  reference of the symbol. After all you can return by
    //  pressing 'F5' key.
    //        Symbol's lookup is the one defined in the Java Language 
    //  Specification including all lookup rules and overloading resolution.
    //      F6 is pushing references of the selected symbol on a
    //  reference stack, F5 pops them. You can hit
    //  F6 several times (without returning by F5) in order to inspect
    //  several symbols, then pressing several times F5 will return you back
    //  to the original place.

    classVar1=0;

    fun1Arg(0);                 // fun1Arg( INT )
    fun1Arg(2.0);               // fun1Arg( DOUBLE )

    demoArg1=0;

    demoFunction(demoArg1,"toto");

    // you can go to the definition of this 'Test' class as well:

    Test.
    fun1StaticNoArg();

    //  You can list all references of
    //  the given symbol in a separate window. Just set the cursor on
    //  a symbol and then press "control F6" key or invoke 
    //  "Browsing with Symbol Stack -> Push and Show Top References" 
    //  item from the 'Xref' menu (or invoke 'xref-listCxrefs' elisp 
    //  function). You can now move 
    //  directly to any reference by clicking the middle mouse button on
    //  a specified line of this list. If you have no mouse you can do the
    //  same by placing the cursor on a reference and then pressing 
    //  <SPACE> key. 
    //  As in previous cases, you can move to the previous and next reference
    //  by F3 and F4 keys, as well as pop references and return back to 
    //  the initial point by the F5 key. You can also close the new window
    //  with F7 and continue browsing references with F3-F4 (previous-next).
    //  For example, get the list of all references of the 'Test' symbol.

    Test

    .fun1StaticNoArg();

    // several other functions are available via a popup menu on
    // right mouse button (in the new window).

    /*             BROWSING VIRTUAL METHODS                */

    // if you try to browse a symbol which can't be unambiguously
    // resolved (due to virtuality) Xrefactory asks for
    // manual resolution of applications which you wish to browse.
    // In the next example positioning the cursor on the getComponentAt
    // symbol and pressing "control F6" (list references) will create
    // an additional menu where you can manually resolve applications
    // you desire to browse. The proposed menu contains informations
    // about the number of references available and class subtree
    // for given virtual method. A star '*' before class name indicates
    // that the class defines given method. In the tree you can select
    // references using space or mouse button2, then you can push 
    // selected references by the <CR> key or by selecting "Go" item
    // from pop-up menu on third mouse button. You can also always delete
    // new windows with F7 key.


    component.getComponentAt(0,0);

    // note, that ambiguity resolution does not occur on F6 if the definition
    // of browsed symbol can be unambiguously determined. Check for example
    // following invocations.

    mylist.getComponentAt(0,0);

    jtree.getComponentAt(0,0);



    /*             SYMBOL RETRIEVAL                */


    //  positioning the cursor on the following identifier 'size'
    //  and invoking the "Search symbol in -> Context Completions"
    //  (or 'xref-search-in-completions' e-lisp function)
    //  will search all possible completions, and displaying
    //  those containing (case insensitive) the string 'size'.
    //  (after, press F7 to delete the created window)

    myButton. size
    ;


    /*
                         REFACTORINGS.
    */


    /*                Symbol renaming.
        In order to rename a symbol, set the cursor on it and select the 
        "Rename Symbol" item from 'Xref' menu, (or invoke 
        the 'xref-rename' e-lisp function). For example, try to rename 
        the 'demoArg1' argument starting at the following occurence:
    */

    classVar1 = demoArg1;   


    // You can rename any kind of symbol in the same way,
    // namely variables, fields, methods, classes (using "Rename Class"
    // menu item).

    /*                       Argument manipulations.
       positioning the cursor on a method name 
       and selecting one of argument manipulations will
       modify all references of the given method.
       You can try to exchange arguments of the 'demoFunction' method.
    */

    demoFunction(0,"x");


  }

    /*

    The presentation of  the most used Xrefactory functions  is over.  You
    can now check  Xrefactory on your project.  Do not  forget to open one
    of your files (in preference move the cursor into the class containing
    the main method) and create new project by "Xref -> Project -> New"
    menu item!

    You can access interactive help  for any Xrefactory function by typing
    "C-h k"  and then selecting  the corresponding function by  mouse from
    "Xref" Menu!
    */
}


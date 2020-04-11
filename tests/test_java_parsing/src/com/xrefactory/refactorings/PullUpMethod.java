package com.xrefactory.refactorings;

/*
  1.)   Put  cursor inside  PullUpMethod  class  and  invoke 'C-xref  ->
  Refactor'. Select 'Set Target  for Next Moving Refactoring' from the
  proposed menu.

  2.)  Move  cursor onto  the definition of  the method to  move (onto
  'method' name), invoke 'C-xref ->  Refactor' once more time and select
  'Pull Up Method'.
*/

class PullUpMethod {
    int x = 0;
    int y = 0;

    // set target here:

}

class InferClass extends PullUpMethod {
    int y = 1;

    // put cursor on 'method' name
    void method() {
        while (y < 1) {
            y = 1;
        }

        System.out.println("x == " + x);
        System.out.println("this.x == " + this.x);
        System.out.println("this.y == " + this.y);
        System.out.println("super.y == " + super.y);
    }

    public static void main(String args[]) {
        (new InferClass()).method();
    }
}

/*
  F5 will bring you back to Index
*/

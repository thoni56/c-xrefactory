/*
  Put cursor on a symbol you wish to rename and press F11 or invoke
  'C-xref -> Refactor'. In the proposed menu move to the 'Rename
  Symbol' refactoring and press <return>.  If a name collision is
  detected, use 'C-xref -> Undo Last Refactoring' to undo wrong
  renaming.
*/

static int j2;

void newRenameSymbol() {
    int i2,k;

    // rename local variable 'i'
    for(i2=0; i2<10; i2++) printf(" %d", i2);
    printf("\n");

    // rename the 'renameSymbol' function
    if (0) newRenameSymbol();

#define PRINTJ() printf("j == %d\n", j2)

    // works inside macros, rename for example 'j'
    j2 = 33; PRINTJ();


    // you can rename any kind of symbol, a macro parameter for example
#define PRINT(renameMeToo) printf("%d\n", renameMeToo)

    // renaming 'k' to 'x' will cause name collision
    k = 0;
    {
        int x; x = k; printf("x==%d\n", x);
    }

#define PRINTX() printf("x == %d\n", x);
    // problem can occur also if a symbol inside a macro is refering
    // to various different variables.  Rename for example following
    // 'x' variable
    {
        int x = 0;
        PRINTX();
    }

}

int main() {
    int x = 1;
    newRenameSymbol();
    PRINTX();
}


/*
  (multiple) F5 will bring you back to index
*/

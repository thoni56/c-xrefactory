#include "classfilereader.h"

#include "symbol.h"
#include "filetable.h"



/* **************************************************************** */

Symbol *cfAddCastsToModule(Symbol *memb, Symbol *sup) {
    assert(memb->u.structSpec);
    cctAddSimpleValue(&memb->u.structSpec->casts, sup, 1);
    assert(sup->u.structSpec);
    cctAddCctTree(&memb->u.structSpec->casts, &sup->u.structSpec->casts, 1);
    return(sup);
}


void convertLinkNameToClassFileName(char classFileName[], char *linkName) {
    sprintf(classFileName, "%c%s.class", ZIP_SEPARATOR_CHAR, linkName);
    assert(strlen(classFileName)+1 < MAX_FILE_NAME_SIZE);
}

int javaCreateClassFileItem( Symbol *memb) {
    char ftname[MAX_FILE_NAME_SIZE];
    int fileNumber;

    convertLinkNameToClassFileName(ftname, memb->linkName);
    fileNumber = addFileNameToFileTable(ftname);
    memb->u.structSpec->classFileNumber = fileNumber;

    return fileNumber;
}

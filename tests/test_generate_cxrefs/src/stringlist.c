#include "stringlist.h"

#include <stdlib.h>
#include <string.h>


/* TODO: Use only for unittests since it does not allocate in dedicated memory */
StringList *newStringList(char *string, StringList *next) {
    StringList *new = (StringList *)malloc(sizeof(StringList));
    new->string = strdup(string);
    new->next = next;
    return new;
}

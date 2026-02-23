#include "stringlist.h"

#include <stdlib.h>
#include <string.h>


/* Allocates with malloc — use for temporary lists, tests, etc. */
StringList *newStringList(char *string, StringList *next) {
    StringList *new = (StringList *)malloc(sizeof(StringList));
    new->string = strdup(string);
    new->next = next;
    return new;
}

void freeStringList(StringList *list) {
    while (list != NULL) {
        StringList *next = list->next;
        free(list->string);
        free(list);
        list = next;
    }
}

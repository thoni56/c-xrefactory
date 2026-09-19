#ifndef STRINGLIST_H_INCLUDED
#define STRINGLIST_H_INCLUDED

typedef struct stringList {
    char *string;
    struct stringList *next;
} StringList;


/* TODO: Use only for unittests since it does not allocate in dedicated memory */
extern StringList *newStringList(char *string, StringList *next);

#endif

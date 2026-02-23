#ifndef STRINGLIST_H_INCLUDED
#define STRINGLIST_H_INCLUDED

typedef struct stringList {
    char *string;
    struct stringList *next;
} StringList;


extern StringList *newStringList(char *string, StringList *next);
extern void freeStringList(StringList *list);

#endif

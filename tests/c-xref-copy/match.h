#ifndef MATCH_H_INCLUDED
#define MATCH_H_INCLUDED

#include "referenceableitem.h"
#include "symbol.h"
#include "visibility.h"


typedef struct Match {
    char                 *name;
    char                 *fullName;
    short int             lineCount;
    Visibility            visibility;
    struct reference      reference;
    struct referenceableItem  referenceable;
    struct Match         *next;
} Match;


extern void freeMatch(Match *match);
extern void freeMatches(Match *matches);

extern Match *prependToMatches(Match *matches, char *name, char *fullText, Symbol *symbol,
                               ReferenceableItem *referenceableItem, Reference *dfpos, int includedFileNumber);

extern Match *getMatchOnNthLine(Match *matches, int n);
extern void sortMatchListByName(Match **matches);

#endif

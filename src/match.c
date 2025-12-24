#include "match.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commons.h"
#include "filetable.h"
#include "list.h"
#include "options.h"
#include "referenceableitem.h"
#include "session.h"
#include "symbol.h"


// Will create malloc():ed copies of name and fullName so caller don't have to
Match *newMatch(char *name, char *fullName, int lineCount, Visibility visibility,
                          Reference reference, ReferenceableItem referenceable) {
    Match *match = malloc(sizeof(Match));

    if (name != NULL) {
        match->name = strdup(name);
    } else
        match->name = NULL;

    if (fullName != NULL) {
        match->fullName = strdup(fullName);
    } else
        match->fullName = NULL;

    match->lineCount = lineCount;
    match->visibility = visibility;
    match->reference = reference;
    match->referenceable = referenceable;
    match->next = NULL;

    return match;
}

// If symbol == NULL, then the pos is taken as default position of this ref !!!
// If symbol != NULL && referenceableItem != NULL then reference can be anything...
Match *prependToMatches(Match *matches, char *name, char *fullName, Symbol *symbol,
                        ReferenceableItem *referenceableItem, Reference *reference,
                        int includedFileNumber) {
    Match *match;

    if (referenceableItem != NULL) {
        // probably a 'search in tag' file item
        char *linkName = strdup(referenceableItem->linkName);

        ReferenceableItem item = makeReferenceableItem(linkName, referenceableItem->type,
                                                       referenceableItem->storage, referenceableItem->scope,
                                                       referenceableItem->visibility, referenceableItem->includeFileNumber);

        match = newMatch(name, fullName, 1, referenceableItem->visibility, *reference, item);
    } else if (symbol==NULL) {
        Reference r = *reference;
        r.next = NULL;

        ReferenceableItem item = makeReferenceableItem("", TypeUnknown, StorageDefault,
                                                       AutoScope, VisibilityLocal, NO_FILE_NUMBER);

        match = newMatch(name, fullName, 1, VisibilityLocal, r, item);
    } else {
        Reference r = makeReference(symbol->pos, UsageNone, NULL);
        Visibility visibility;
        Scope scope;
        Storage storage;
        getSymbolCxrefProperties(symbol, &visibility, &scope, &storage);
        char *linkName = strdup(symbol->linkName);

        ReferenceableItem item = makeReferenceableItem(linkName, symbol->type, storage,
                                                       scope, visibility, includedFileNumber);
        match = newMatch(name, fullName, 1, visibility, r, item);
    }
    if (fullName!=NULL) {
        for (int i=0; fullName[i]; i++) {
            if (fullName[i] == '\n')
                match->lineCount++;
        }
    }
    match->next = matches;
    return match;
}

protected void freeMatch(Match *match) {
    free(match->name);
    free(match->fullName);
    if (match->visibility == VisibilityGlobal) {
        assert(match->referenceable.linkName);
        free(match->referenceable.linkName);
    }
    free(match);
}


void freeMatches(Match *matches) {
    Match *tmp;

    while (matches!=NULL) {
        tmp = matches->next;
        freeMatch(matches);
        matches = tmp;
    }
}

Match *getMatchOnNthLine(Match *matches, int n) {
    Match *last, *this;

    this = matches;
    for (int i=1; i<=n && this!=NULL; this=this->next) {
        i += this->lineCount;
        last = this;
    }
    return last;
}

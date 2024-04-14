#include "jslsemact.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "misc.h"
#include "parsers.h"
#include "classcaster.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "filetable.h"
#include "jsemact.h"
#include "symbol.h"
#include "list.h"
#include "stackmemory.h"

#include "log.h"
#include "recyacc.h"

S_jslStat *s_jsl;


void fillJslStat(S_jslStat *jslStat, int pass, int sourceFileNumber, int language, JslTypeTab *typeTab,
                 S_jslClassStat *classStat, SymbolList *waitList, void /*YYSTYPE*/ *savedyylval,
                 void /*S_yyGlobalState*/ *savedYYstate, int yyStateSize, S_jslStat *next) {
    jslStat->pass = pass;
    jslStat->sourceFileNumber = sourceFileNumber;
    jslStat->language = language;
    jslStat->typeTab = typeTab;
    jslStat->classStat = classStat;
    jslStat->waitList = waitList;
    jslStat->savedyylval = savedyylval;
    jslStat->savedYYstate = savedYYstate;
    jslStat->yyStateSize = yyStateSize;
    jslStat->next = next;
}

static void fillJslSymbolList(JslSymbolList *jslSymbolList, struct symbol *d,
                              struct position pos, bool isExplicitlyImported) {
    jslSymbolList->d = d;
    jslSymbolList->position = pos;
    jslSymbolList->isExplicitlyImported = isExplicitlyImported;
    jslSymbolList->next = NULL;
}

static void jslCreateTypeSymbolInList(JslSymbolList *ss, char *name) {
    Symbol *s;

    s = newSymbol(name, name, noPosition);
    s->type = TypeStruct;
    s->storage = StorageNone;
    fillJslSymbolList(ss, s, noPosition, false);
}

/* Used as argument to addToFrame() thus void* argument */
static void jslRemoveNestedClass(void *ddv) {
    JslSymbolList *dd;
    bool deleted;

    dd = (JslSymbolList *) ddv;
    //&fprintf(dumpOut, "removing class %s from jsltab\n", dd->d->name);
    log_debug("removing class %s from jsltab", dd->d->name);
    assert(s_jsl!=NULL);
    deleted = jslTypeTabDeleteExact(s_jsl->typeTab, dd);
    assert(deleted);
}

Symbol *jslTypeSymbolDefinition(char *ttt2, IdList *packid,
                                AddYesNo add, int order, bool isExplicitlyImported) {
    char fqtName[MAX_FILE_NAME_SIZE];
    IdList dd2;
    int index;
    Symbol *smemb;
    JslSymbolList ss, *xss, *memb;
    Position *importPos;
    bool isMember; UNUSED isMember; /* Because log_trace() might not generate any code... */

    jslCreateTypeSymbolInList(&ss, ttt2);
    fillfIdList(&dd2, ttt2, NULL, noPosition, ttt2, TypeStruct, packid);
    javaCreateComposedName(NULL,&dd2,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    smemb = javaFQTypeSymbolDefinition(ttt2, fqtName);
    //&fprintf(communicationChannel, "[jsl] jslTypeSymbolDefinition %s, %s, %s, %s\n", ttt2, fqtName, smemb->name, smemb->linkName);
    if (add == ADD_YES) {
        if (packid!=NULL) importPos = &packid->id.position;
        else importPos = &noPosition;
        xss = stackMemoryAlloc(sizeof(JslSymbolList)); // Or in MEMORY_CF ???
        fillJslSymbolList(xss, smemb, *importPos, isExplicitlyImported);
        /* TODO: Why are we using isMember() and not looking at the result? Side-effect? */
        isMember = jslTypeTabIsMember(s_jsl->typeTab, xss, &index, &memb);
        log_trace("[jsl] jslTypeTabIsMember() returned %s", isMember?"true":"false");
        if (order == ORDER_PREPEND) {
            log_debug("[jsl] prepending class %s to jsltab", smemb->name);
            jslTypeTabPush(s_jsl->typeTab, xss, index);
            addToFrame(jslRemoveNestedClass, xss);
        } else {
            log_debug("[jsl] appending class %s to jsltab", smemb->name);
            jslTypeTabSetLast(s_jsl->typeTab, xss, index);
            addToFrame(jslRemoveNestedClass, xss);
        }
    }
    return(smemb);
}

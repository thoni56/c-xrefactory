#ifndef JSLSEMACT_H_INCLUDED
#define JSLSEMACT_H_INCLUDED

/* JSL = Java Simple Load file... ? */

#include "completion.h"
#include "symbol.h"
#include "jsltypetab.h"


/* ***************** Java simple load file ********************** */


typedef struct jslClassStat {
    struct idList *className;
    struct symbol *thisClass;
    char          *thisPackage;
    int           annonInnerCounter; /* counter for anonymous inner classes*/
    int           functionInnerCounter; /* counter for function inner class*/
    struct jslClassStat	*next;
} S_jslClassStat;


typedef struct jslStat {
    int                              pass;
    int                              sourceFileNumber;
    int                              language;
    struct jslTypeTab                *typeTab;
    struct jslClassStat              *classStat;
    struct symbolList                *waitList;
    void /*YYSTYPE*/                 *savedyylval;
    void /*struct yyGlobalState*/    *savedYYstate;
    int                              yyStateSize;
    Id                               yyIdentBuf[YYIDBUFFER_SIZE]; // pending idents
    struct jslStat                   *next;
} S_jslStat;


extern S_jslStat *s_jsl;


extern void fillJslStat(S_jslStat *jslStat, int pass, int sourceFileNumber, int language, JslTypeTab *typeTab,
                        S_jslClassStat *classStat, SymbolList *waitList, void *savedyylval,
                        void /*S_yyGlobalState*/ *savedYYstate, int yyStateSize, S_jslStat *next);

extern Symbol *jslTypeSymbolDefinition(char *ttt2, IdList *packid,
                                       AddYesNo add, int order, bool isExplicitlyImported);
extern void jslAddSuperClassOrInterfaceByName(Symbol *memb,char *super);

#endif

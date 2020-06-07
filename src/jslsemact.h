#ifndef JSLSEMACT_H
#define JSLSEMACT_H

/* JSL = Java Simple Load file... ? */

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
    struct id                        yyIdentBuf[YYBUFFERED_ID_INDEX]; // pending idents
    struct jslStat                   *next;
} S_jslStat;


extern S_jslStat *s_jsl;


extern S_jslClassStat *newJslClassStat(IdList *className, Symbol *thisClass, char *thisPackage,
                                       S_jslClassStat *next);
extern void fillJslStat(S_jslStat *jslStat, int pass, int sourceFileNumber, int language, S_jslTypeTab *typeTab,
                        S_jslClassStat *classStat, SymbolList *waitList, void *savedyylval,
                        void /*S_yyGlobalState*/ *savedYYstate, int yyStateSize, S_jslStat *next);
extern Symbol *jslTypeSpecifier1(Type t);
extern Symbol *jslTypeSpecifier2(TypeModifier *t);

extern void jslCompleteDeclarator(Symbol *t, Symbol *d);
extern TypeModifier *jslPrependComposedType(TypeModifier *d, Type type);
extern TypeModifier *jslAppendComposedType(TypeModifier **d, Type type);
extern Symbol *jslPrependDirectEnclosingInstanceArgument(Symbol *args);
extern Symbol *jslMethodHeader(unsigned modif, Symbol *type, Symbol *decl, int storage, SymbolList *throws);
extern Symbol *jslTypeNameDefinition(IdList *tname);
extern Symbol *jslTypeSymbolDefinition(char *ttt2, IdList *packid,
                                       AddYesNo add, int order, bool isExplicitlyImported);
extern int jslClassifyAmbiguousTypeName(IdList *name, Symbol **str);
extern void jslAddNestedClassesToJslTypeTab( Symbol *cc, int order);
extern void jslAddSuperNestedClassesToJslTypeTab( Symbol *cc);

extern void jslAddSuperClassOrInterfaceByName(Symbol *memb,char *super);
extern void jslNewClassDefinitionBegin(Id *name,
                                       int accessFlags,
                                       Symbol *anonInterf,
                                       int position);
extern void jslAddDefaultConstructor(Symbol *cl);
extern void jslNewClassDefinitionEnd(void);
extern void jslNewAnonClassDefinitionBegin(Id *interfName);

extern void jslAddSuperClassOrInterface(Symbol *memb,Symbol *supp);
extern void jslAddMapedImportTypeName(char *file,
                                      char *path,
                                      char *pack,
                                      Completions *c,
                                      void *vdirid,
                                      int  *storage);
extern void jslAddAllPackageClassesFromFileTab(IdList *pack);

#endif

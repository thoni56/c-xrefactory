#ifndef JSLSEMACT_H
#define JSLSEMACT_H

#include "proto.h"

extern S_symbol *jslTypeSpecifier1(unsigned t);
extern S_symbol *jslTypeSpecifier2(S_typeModifiers *t);

extern void jslCompleteDeclarator(S_symbol *t, S_symbol *d);
extern S_typeModifiers *jslPrependComposedType(S_typeModifiers *d, unsigned t);
extern S_typeModifiers *jslAppendComposedType(S_typeModifiers **d, unsigned t);
extern S_symbol *jslPrependDirectEnclosingInstanceArgument(S_symbol *args);
extern S_symbol *jslMethodHeader(unsigned modif, S_symbol *type, S_symbol *decl, int storage, S_symbolList *throws);
extern S_symbol *jslTypeNameDefinition(S_idIdentList *tname);
extern S_symbol *jslTypeSymbolDefinition(char *ttt2, S_idIdentList *packid,
                                         int add, int order, int isSingleImportedFlag);
extern int jslClassifyAmbiguousTypeName(S_idIdentList *name, S_symbol **str);
extern void jslAddNestedClassesToJslTypeTab( S_symbol *cc, int order);
extern void jslAddSuperNestedClassesToJslTypeTab( S_symbol *cc);

extern void jslAddSuperClassOrInterfaceByName(S_symbol *memb,char *super);
extern void jslNewClassDefinitionBegin(S_idIdent *name,
                                       int accessFlags,
                                       S_symbol *anonInterf,
                                       int position
                                       );
extern void jslAddDefaultConstructor(S_symbol *cl);
extern void jslNewClassDefinitionEnd(void);
extern void jslNewAnonClassDefinitionBegin(S_idIdent *interfName);

extern void jslAddSuperClassOrInterface(S_symbol *memb,S_symbol *supp);
extern void jslAddMapedImportTypeName(
                                      char *file,
                                      char *path,
                                      char *pack,
                                      S_completions *c,
                                      void *vdirid,
                                      int  *storage
                                      );
extern void jslAddAllPackageClassesFromFileTab(S_idIdentList *pack);


extern S_jslStat *s_jsl;

#endif

#ifndef YYLEX_H
#define YYLEX_H

#include <stdio.h>

#include "strTdef.h"

extern char *yytext;

extern void ppMemInit();
extern void initAllInputs();
extern void initInput(FILE *ff, S_editorBuffer *buffer, char *prepend, char *name);
extern void addIncludeReference(int filenum, S_position *pos);
extern void addThisFileDefineIncludeReference(int filenum);
extern void pushNewInclude(FILE *f, S_editorBuffer *buff, char *name, char *prepend);
extern void popInclude();
extern int addFileTabItem(char *name, int *fileNumber);
extern void addMacroDefinedByOption(char *opt);
extern char *placeIdent();
extern int cachedInputPass(int cpoint, char **cfromto);
extern int cexpyylex();
extern int yylex();

#endif

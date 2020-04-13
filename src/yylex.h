#ifndef YYLEX_H
#define YYLEX_H

#include "proto.h"
#include "editor.h"


typedef struct macroBody {
    short int argn;
    int size;
    char *name;			/* the name of the macro */
    char **args;		/* names of arguments */
    char *body;
} S_macroBody;

typedef struct macroArgTabElem {
    char *name;
    char *linkName;
    int order;
} S_macroArgumentTableElement;

typedef struct lexInput {
    char *currentLexem;
    char *endOfBuffer;
    char *beginningOfBuffer;
    char *macroName;
    char margExpFlag;		/* input Flag */
} S_lexInput;

extern char *yytext;
extern S_lexInput macStack[MACRO_STACK_SIZE];
extern int macroStackIndex;
extern S_lexInput cInput;


extern void fillLexInput(S_lexInput *lexInput, char *currentLexem, char *endOfBuffer,
                         char *beginningOfBuffer, char *macroName, char margExpFlag);
extern void ppMemInit(void);
extern void initAllInputs(void);
extern void initInput(FILE *ff, S_editorBuffer *buffer, char *prepend, char *name);
extern void addIncludeReference(int filenum, S_position *pos);
extern void addThisFileDefineIncludeReference(int filenum);
extern void pushNewInclude(FILE *f, S_editorBuffer *buff, char *name, char *prepend);
extern void popInclude(void);
extern int addFileTabItem(char *name);
extern void addMacroDefinedByOption(char *opt);
extern char *placeIdent(void);
extern int cachedInputPass(int cpoint, char **cfromto);
extern int cexpyylex(void);
extern int yylex(void);

#endif

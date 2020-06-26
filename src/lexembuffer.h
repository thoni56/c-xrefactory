#ifndef _LEXEMBUFFER_H_
#define _LEXEMBUFFER_H_

#include "lexem.h"
#include "characterreader.h"
#include "position.h"


typedef struct lexemBuffer {
    char *next;				/* next to read */
    char *end;				/* pointing *after* last valid char */
    char chars[LEX_BUFF_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int index;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
    CharacterBuffer buffer;
} LexemBuffer;


/* Lexer macros for passing compressed tokens to the parser */
/* In the process of being turned into testable/debuggable functions */

extern void putLexChar(char ch, char **writePointer);
extern void putLexShort(int shortValue, char **writePointer);
extern void putLexToken(Lexem lexem, char **writePointer);
extern void putLexInt(int value, char **writePointer);
extern void putLexCompacted(int value, char **writePointer);
extern void putLexLines(int lines, char **writePointer);

extern unsigned char getLexChar(char **readPointer);
extern int getLexShort(char **readPointer);
extern Lexem getLexToken(char **readPointer);
extern int getLexInt(char **readPointer);
extern int getLexCompacted(char **readPointer);

extern Lexem nextLexToken(char **readPointer);

#ifndef XREF_HUGE

/* NORMAL compacted tokens, HUGE compression is below */
/* Can only store file, line, column < 4194304 */

extern void putLexPosition(int file, int line, int col, char **writePointer);
extern Position getLexPosition(char **readPointer);

#define NextLexPosition(pos,tmpcc) {            \
        char *tmptmpcc = tmpcc;                 \
        pos = getLexPosition(&tmptmpcc);        \
    }

#else

/* HUGE only !!!!!, normal compacted encoding is above */

#error HUGE mode is not supported any more, rewrite your application...

#define PutLexFilePos(xxx,dd) PutLexInt(xxx,dd)
#define PutLexNumPos(xxx,dd) PutLexInt(xxx,dd)
#define PutLexPosition(cfile,cline,idcoll,dd) {             \
        PutLexFilePos(cfile,dd);                            \
        PutLexNumPos(cline,dd);                             \
        PutLexNumPos(idcoll,dd);                            \
        log_trace("push idp %d %d %d",cfile,cline,idcoll);  \
    }

#define GetLexFilePos(xxx,dd) GetLexInt(xxx,dd)
#define GetLexNumPos(xxx,dd) GetLexInt(xxx,dd)
#define GetLexPosition(pos,tmpcc) {             \
        GetLexFilePos(pos.file,tmpcc);          \
        GetLexNumPos(pos.line,tmpcc);           \
        GetLexNumPos(pos.coll,tmpcc);           \
    }

#define NextLexFilePos(dd) NextLexInt(dd)
#define NextLexNumPos(dd) NextLexInt(dd)
#define NextLexPosition(pos,tmpcc) {                        \
        pos.file = NextLexFilePos(tmpcc);                   \
        pos.line = NextLexNumPos(tmpcc+sizeof(short));      \
        pos.coll = NextLexNumPos(tmpcc+2*sizeof(short));    \
    }

#endif

extern void initLexemBuffer(LexemBuffer *buffer, FILE *file);

#endif

#ifndef _LEXEMBUFFER_H_
#define _LEXEMBUFFER_H_

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

/* Common to normal and huge case: */
#define PutLexLine(lines, dd) {                              \
        if (lines!=0) {                                      \
            PutLexToken(LINE_TOK,dd);                        \
            PutLexToken(lines,dd);                           \
        }                                                    \
    }

extern void putLexChar(char ch, char **destination);

#define PutLexToken(xxx,dd) {PutLexShort(xxx,dd);}

#define PutLexShort(xxx,dd) {                   \
        *(dd)++ = ((unsigned)(xxx))%256;        \
        *(dd)++ = ((unsigned)(xxx))/256;        \
}

#define PutLexInt(xxx,dd) {                     \
    unsigned tmp;\
    tmp = xxx;\
    *(dd)++ = tmp%256; tmp /= 256;\
    *(dd)++ = tmp%256; tmp /= 256;\
    *(dd)++ = tmp%256; tmp /= 256;\
    *(dd)++ = tmp%256; tmp /= 256;\
}


#define GetLexChar(xxx,dd) {xxx = *((unsigned char*)dd++);}

#define GetLexToken(xxx,dd) GetLexShort(xxx,dd)

#define GetLexShort(xxx,dd) {                   \
    xxx = *((unsigned char*)dd++);\
    xxx += 256 * *((unsigned char*)dd++);\
}

#define GetLexInt(xxx,dd) {                     \
    xxx = *((unsigned char*)dd++);\
    xxx += 256 * *((unsigned char*)dd++);\
    xxx += 256 * 256 * *((unsigned char*)dd++);\
    xxx += 256 * 256 * 256 * *((unsigned char*)dd++);\
}

#define NextLexChar(dd) (*((unsigned char*)dd))

#define NextLexShort(dd) (                      \
    *((unsigned char*)dd) \
    + 256 * *(((unsigned char*)dd)+1)\
)

#define NextLexToken(dd) (NextLexShort(dd))

#define NextLexInt(dd) (  \
    *((unsigned char*)dd) \
    + 256 * *(((unsigned char*)dd)+1)\
    + 256 * 256 * *(((unsigned char*)dd)+2) \
    + 256 * 256 * 256 * *(((unsigned char*)dd)+3)\
)


#ifndef XREF_HUGE

/* NORMAL compacted tokens, HUGE compression is below */

/* TODO Tests that exercise these and show the difference in
   performance and compression. */

#define PutLexCompacted(xxx,dd) {\
    assert(((unsigned) xxx)<4194304);\
    if (((unsigned)xxx)>=128) {\
        if (((unsigned)xxx)>=16384) {\
            *(dd)++ = ((unsigned)xxx)%128+128;\
            *(dd)++ = ((unsigned)xxx)/128%128+128;\
            *(dd)++ = ((unsigned)xxx)/16384;\
        } else {\
            *(dd)++ = ((unsigned)xxx)%128+128;\
            *(dd)++ = ((unsigned)xxx)/128;\
        }\
    } else {\
        *(dd)++ = ((unsigned char)xxx);\
    }\
}
#define PutLexPosition(cfile,cline,idcoll,dd) {\
    assert(cfile>=0 && cfile<MAX_FILES);\
    PutLexCompacted(cfile,dd);\
    PutLexCompacted(cline,dd);\
    PutLexCompacted(idcoll,dd);\
    log_trace("push idp %d %d %d",cfile,cline,idcoll); \
}

/* Lexems are coded in compacted form in the lexBuffer, that's why
 * the first char is taken and then the second is add 256**(next char)
 * so [275][001] means 275 + 256 * 1 = 513
 */
#define GetLexCompacted(xxx,dd) {\
    xxx = *((unsigned char*)dd++);\
    if (((unsigned)xxx)>=128) {\
        unsigned yyy = *((unsigned char*)dd++);\
        if (yyy >= 128) {\
            xxx = ((unsigned)xxx)-128 + 128 * (yyy-128) + 16384 * *((unsigned char*)dd++);\
        } else {\
            xxx = ((unsigned)xxx)-128 + 128 * yyy;\
        }\
    }\
}
#define GetLexPosition(pos,tmpcc) {\
    GetLexCompacted(pos.file,tmpcc);\
    GetLexCompacted(pos.line,tmpcc);\
    GetLexCompacted(pos.col,tmpcc);\
}

#define NextLexPosition(pos,tmpcc) {\
    char *tmptmpcc = tmpcc;\
    GetLexPosition(pos, tmptmpcc);\
}

#else

/* HUGE only !!!!!, normal compacted encoding is above */

#define PutLexFilePos(xxx,dd) PutLexInt(xxx,dd)
#define PutLexNumPos(xxx,dd) PutLexInt(xxx,dd)
#define PutLexPosition(cfile,cline,idcoll,dd) {\
    PutLexFilePos(cfile,dd);\
    PutLexNumPos(cline,dd);\
    PutLexNumPos(idcoll,dd);\
    log_trace("push idp %d %d %d",cfile,cline,idcoll); \
}

#define GetLexFilePos(xxx,dd) GetLexInt(xxx,dd)
#define GetLexNumPos(xxx,dd) GetLexInt(xxx,dd)
#define GetLexPosition(pos,tmpcc) {\
    GetLexFilePos(pos.file,tmpcc);\
    GetLexNumPos(pos.line,tmpcc);\
    GetLexNumPos(pos.coll,tmpcc);\
}

#define NextLexFilePos(dd) NextLexInt(dd)
#define NextLexNumPos(dd) NextLexInt(dd)
#define NextLexPosition(pos,tmpcc) {\
    pos.file = NextLexFilePos(tmpcc);\
    pos.line = NextLexNumPos(tmpcc+sizeof(short));\
    pos.coll = NextLexNumPos(tmpcc+2*sizeof(short));\
}

#endif

extern void initLexemBuffer(LexemBuffer *buffer, FILE *file);

#endif

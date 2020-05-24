#include "lex.h"
#include "lexmac.h"

#include "globals.h"
#include "commons.h"
#include "parsers.h"
#include "cxref.h"
#include "characterbuffer.h"
#include "yylex.h"
#include "filedescriptor.h"

#include "caching.h"            /* cacheInput() */
#include "jslsemact.h"          /* s_jsl */

#include "log.h"
#include "utils.h"              /* creatingOlcxRefs() */


void gotOnLineCxRefs(Position *ps ) {
    if (creatingOlcxRefs()) {
        s_cache.activeCache = 0;
        s_cxRefPos = *ps;
    }
}

/* ***************************************************************** */
/*                         Lexical Analysis                          */
/* ***************************************************************** */

static int columnPosition(CharacterBuffer *cb, char *cb_lineBegin, int cb_columnOffset) {
    return cb->next - cb_lineBegin + cb_columnOffset - 1;
}

static int absoluteFilePosition(CharacterBuffer *cb, char *cb_end, char *cb_next) {
    return cb->filePos - (cb_end-cb_next) - 1;
}

#define PutLexLine(lines, dd) {                              \
        if (lines!=0) {                                      \
            PutLexToken(LINE_TOK,dd);                        \
            PutLexToken(lines,dd);                           \
        }                                                    \
    }

#define LexGetChar(ch, cb, cb_next) {                                   \
        assert(cb->next == cb_next);                                    \
        if (cb->next >= cb->end) {                                      \
            cb->columnOffset = cb->next - cb->lineBegin;                \
            if (cb->isAtEOF || refillBuffer(cb) == 0) {                 \
                ch = -1;                                                \
                cb->isAtEOF = true;                                     \
            } else {                                                    \
                cb->lineBegin = cb->next;                               \
                ch = *((unsigned char *)cb->next);                      \
                cb->next = ((char *)cb->next) + 1;                      \
            }                                                           \
        } else {                                                        \
            ch = * ((unsigned char *)cb->next);                         \
            cb->next = ((char *)cb->next) + 1;                          \
            cb_next = cb->next;                                         \
        }                                                               \
    }

#define UngetChar(ch, cb) {                                             \
        if (ch == '\n')                                                 \
            log_trace("Ungetting %s('\\n') at cb->next", #ch);          \
        else                                                            \
            log_trace("Ungetting %s('%c') at cb->next", #ch, ch);       \
        *--(cb->next) = ch;                                             \
    }

#define DeleteBlank(ch, cb, cb_next) {                                  \
        while (ch==' '|| ch=='\t' || ch=='\004') {                      \
            assert(cb->next == cb_next);                                \
            LexGetChar(ch, cb, cb_next);                                \
        }                                                               \
    }

#define PassComment(ch, cb, cb_next, cb_end, dd, cb_lineNumber, cb_lineBegin, cb_columnOffset) { \
        assert(cb->next == cb_next);                                    \
        char oldCh;                                                     \
        int line = cb_lineNumber;                                       \
        /*  ******* a block comment ******* */                          \
        LexGetChar(ch, cb, cb_next);                                    \
        if (ch=='\n') {                                                 \
            cb_lineNumber ++;                                           \
            cb_lineBegin = cb_next;                                     \
            cb_columnOffset = 0;                                        \
        }                                                               \
        /* TODO test on cpp directive */                                \
        do {                                                            \
            oldCh = ch;                                                 \
            LexGetChar(ch, cb, cb_next);                                \
            if (ch=='\n') {                                             \
                cb_lineNumber ++;                                       \
                cb_lineBegin = cb_next;                                 \
                cb_columnOffset = 0;                                    \
            }                                                           \
            /* TODO test on cpp directive */                            \
        } while ((oldCh != '*' || ch != '/') && ch != -1);              \
        if (ch == -1)                                                   \
            warningMessage(ERR_ST,"comment through eof");               \
        PutLexLine(cb_lineNumber-line,dd);                              \
        LexGetChar(ch, cb, cb_next);                                    \
    }

#define ConType(ch, cb, cb_next, rlex) {                                \
        rlex = CONSTANT;                                                \
        if (LANGUAGE(LANG_JAVA)) {                                      \
            if (ch=='l' || ch=='L') {                                   \
                rlex = LONG_CONSTANT;                                   \
                LexGetChar(ch, cb, cb_next);                            \
            }                                                           \
        } else {                                                        \
            for(; ch=='l'||ch=='L'||ch=='u'||ch=='U'; ){                \
                if (ch=='l' || ch=='L')                                 \
                    rlex = LONG_CONSTANT;                               \
                LexGetChar(ch, cb, cb_next);                            \
            }                                                           \
        }                                                               \
    }

#define FloatingPointConstant(ch, cb, cb_next, rlex) {                  \
        rlex = DOUBLE_CONSTANT;                                         \
        if (ch == '.') {                                                \
            do {                                                        \
                LexGetChar(ch, cb, cb_next);                            \
            } while (isdigit(ch));                                      \
        }                                                               \
        if (ch == 'e' || ch == 'E') {                                   \
            LexGetChar(ch, cb, cb_next);                                \
            if (ch == '+' || ch=='-')                                   \
                LexGetChar(ch, cb, cb_next);                            \
            while (isdigit(ch))                                         \
                LexGetChar(ch, cb, cb_next);                            \
        }                                                               \
        if (LANGUAGE(LANG_JAVA)) {                                      \
            if (ch == 'f' || ch == 'F' || ch == 'd' || ch == 'D') {     \
                if (ch == 'f' || ch == 'F') rlex = FLOAT_CONSTANT;      \
                LexGetChar(ch, cb, cb_next);                            \
            }                                                           \
        } else {                                                        \
            if (ch == 'f' || ch == 'F' || ch == 'l' || ch == 'L') {     \
                LexGetChar(ch, cb, cb_next);                            \
            }                                                           \
        }                                                               \
    }

#define ProcessIdentifier(ch, cb, dd, cb_next, cb_lineNumber, cb_lineBegin, cb_columnOffset, labelSuffix){ \
        int idcoll;                                                     \
        char *ddd;                                                      \
        assert(cb->next == cb_next);                                    \
        /* ***************  identifier ****************************  */ \
        ddd = dd;                                                       \
        idcoll = columnPosition(cb, cb_lineBegin, cb_columnOffset);     \
        PutLexToken(IDENTIFIER,dd);                                     \
        do {                                                            \
            PutLexChar(ch,dd);                                          \
        identCont##labelSuffix:                                         \
            LexGetChar(ch, cb, cb_next);                                \
        } while (isalpha(ch) || isdigit(ch) || ch=='_' || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))); \
        if (ch == '@' && *(dd-1)=='C') {                                \
            int i,len;                                                  \
            len = strlen(s_editCommunicationString);                    \
            for (i=2;i<len;i++) {                                       \
                LexGetChar(ch, cb, cb_next);                            \
                if (ch != s_editCommunicationString[i])                 \
                    break;                                              \
            }                                                           \
            if (i>=len) {                                               \
                /* it is the place marker */                            \
                dd--; /* delete the C */                                \
                LexGetChar(ch, cb, cb_next);                            \
                if (ch == CC_COMPLETION) {                              \
                    PutLexToken(IDENT_TO_COMPLETE, ddd);                \
                    LexGetChar(ch, cb, cb_next);                        \
                } else if (ch == CC_CXREF) {                            \
                    s_cache.activeCache = 0;                            \
                    fillPosition(&s_cxRefPos, cb->fileNumber, cb_lineNumber, idcoll); \
                    goto identCont##labelSuffix;                        \
                } else errorMessage(ERR_INTERNAL, "unknown communication char"); \
            } else {                                                    \
                /* not a place marker, undo reading */                  \
                for(i--;i>=1;i--) {                                     \
                    UngetChar(ch, cb);                                  \
                    ch = s_editCommunicationString[i];                  \
                }                                                       \
            }                                                           \
        }                                                               \
        PutLexChar(0,dd);                                               \
        PutLexPosition(cb->fileNumber, cb_lineNumber, idcoll, dd);      \
    }

#define HandleCppToken(ch, cb, dd, cb_next, cb_fileNumber, cb_lineNumber, cb_lineBegin) { \
        char *ddd, tt[10];                                              \
        int i, lcoll, scol;                                             \
        lcoll = columnPosition(cb, cb_lineBegin, cb->columnOffset);     \
        LexGetChar(ch, cb, cb_next);                                    \
        DeleteBlank(ch, cb, cb_next);                                   \
        for(i=0; i<9 && (isalpha(ch) || isdigit(ch) || ch=='_') ; i++) { \
            tt[i] = ch;                                                 \
            LexGetChar(ch, cb, cb_next);                                \
        }                                                               \
        tt[i]=0;                                                        \
        if (strcmp(tt,"ifdef") == 0) {                                  \
            PutLexToken(CPP_IFDEF,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"ifndef") == 0) {                          \
            PutLexToken(CPP_IFNDEF,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"if") == 0) {                              \
            PutLexToken(CPP_IF,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"elif") == 0) {                            \
            PutLexToken(CPP_ELIF,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"undef") == 0) {                           \
            PutLexToken(CPP_UNDEF,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"else") == 0) {                            \
            PutLexToken(CPP_ELSE,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"endif") == 0) {                           \
            PutLexToken(CPP_ENDIF,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else if (strcmp(tt,"include") == 0) {                         \
            char endCh;                                                 \
            PutLexToken(CPP_INCLUDE,dd);                                \
            PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd);       \
            DeleteBlank(ch, cb, cb_next);                               \
            if (ch == '\"' || ch == '<') {                              \
                if (ch == '\"') endCh = '\"';                           \
                else endCh = '>';                                       \
                scol = columnPosition(cb, cb_lineBegin, cb->columnOffset); \
                PutLexToken(STRING_LITERAL,dd);                         \
                do {                                                    \
                    PutLexChar(ch,dd);                                  \
                    LexGetChar(ch, cb, cb_next);                        \
                } while (ch!=endCh && ch!='\n');                        \
                PutLexChar(0,dd);                                       \
                PutLexPosition(cb_fileNumber,cb_lineNumber,scol,dd);    \
                if (ch == endCh)                                        \
                    LexGetChar(ch, cb, cb_next);                        \
            }                                                           \
        } else if (strcmp(tt,"define") == 0) {                          \
            ddd = dd;                                                   \
            PutLexToken(CPP_DEFINE0,dd);                                \
            PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd);       \
            DeleteBlank(ch, cb, cb_next);                               \
            NOTE_NEW_LEXEM_POSITION(cb, lb, cb_lineNumber, cb_lineBegin); \
            ProcessIdentifier(ch, cb, dd, cb_next, cb_lineNumber, cb_lineBegin, cb->columnOffset,lab1); \
            if (ch == '(') {                                            \
                PutLexToken(CPP_DEFINE,ddd);                            \
            }                                                           \
        } else if (strcmp(tt,"pragma") == 0) {                          \
            PutLexToken(CPP_PRAGMA,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        } else {                                                        \
            PutLexToken(CPP_LINE,dd); PutLexPosition(cb_fileNumber,cb_lineNumber,lcoll,dd); \
        }                                                               \
    }

#define CommentaryBegRef(cb, cb_next, cb_lineNumber, cb_lineBegin) {    \
        if (s_opt.taskRegime==RegimeHtmlGenerate && s_opt.htmlNoColors==0) { \
            int lcoll;                                                  \
            Position pos;                                               \
            char ttt[TMP_STRING_SIZE];                                  \
            lcoll = columnPosition(cb, cb_lineBegin, cb->columnOffset) - 1; \
            fillPosition(&pos, cb->fileNumber, cb_lineNumber, lcoll);   \
            sprintf(ttt,"%x/*", cb->fileNumber);                        \
            addTrivialCxReference(ttt, TypeComment, StorageDefault, &pos, UsageDefined); \
        }                                                               \
    }

#define CommentaryEndRef(cb, cb_next, cb_lineNumber, cb_lineBegin, jdoc) { \
        if (s_opt.taskRegime==RegimeHtmlGenerate) {                     \
            int lcoll;                                                  \
            Position pos;                                               \
            char ttt[TMP_STRING_SIZE];                                  \
            lcoll = columnPosition(cb, cb_lineBegin, cb->columnOffset); \
            fillPosition(&pos, cb->fileNumber, cb_lineNumber, lcoll);   \
            sprintf(ttt,"%x/*", cb->fileNumber);                        \
            if (s_opt.htmlNoColors==0) {                                \
                addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageUsed); \
            }                                                           \
            if (jdoc) {                                                 \
                pos.col -= 2;                                           \
                addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageJavaDoc); \
            }                                                           \
        }                                                               \
    }

#define NOTE_NEW_LEXEM_POSITION(cb, lb, cb_lineNumber, cb_lineBegin) { \
        int index = lb->index % LEX_POSITIONS_RING_SIZE;                \
        lb->fileOffsetRing[index] = absoluteFilePosition(cb, cb->end, cb->next);  \
        lb->positionRing[index].file = cb->fileNumber;                  \
        lb->positionRing[index].line = cb_lineNumber;                   \
        lb->positionRing[index].col = columnPosition(cb, cb_lineBegin, cb->columnOffset); \
        lb->index ++;                                                   \
    }

#define PUT_EMPTY_COMPLETION_ID(cb, dd, cb_lineNumber, cb_columnOffset, cb_lineBegin, cb_fileNumber, len) { \
        PutLexToken(IDENT_TO_COMPLETE, dd);                             \
        PutLexChar(0, dd);                                              \
        PutLexPosition(cb_fileNumber, cb_lineNumber,                    \
                       columnPosition(cb, cb_lineBegin, cb_columnOffset) - (len), dd); \
    }

bool getLexBuf(S_lexBuf *lb) {
    int ch;
    CharacterBuffer *cb;
    char *cb_next, *cb_end;
    int cb_lineNumber;
    int cb_columnOffset;
    char *cb_lineBegin;

    char *cc, *dd, *lmax, *lexStartDd;
    unsigned chval=0;
    int rlex;
    int line,size,cb_fileNumber,lexStartCol, lexStartFilePos, column;

    /* first test whether the input is cached */
    if (s_cache.activeCache && inStacki==0 && macroStackIndex==0) {
        cacheInput();
        s_cache.lexcc = lb->chars;
    }

    lmax = lb->chars + LEX_BUFF_SIZE - MAX_LEXEM_SIZE;
    for(dd=lb->chars,cc=lb->next; cc<lb->end; cc++,dd++)
        *dd = *cc;
    lb->next = lb->chars;

    cb = &lb->buffer;
    cb_lineNumber = cb->lineNumber;
    cb_lineBegin = cb->lineBegin;
    cb_columnOffset = cb->columnOffset;

    cb_next = cb->next;
    cb_end = cb->end;
    cb_fileNumber = cb->fileNumber;

    LexGetChar(ch, cb, cb_next);

    do {
        DeleteBlank(ch, cb, cb_next);
        if (dd >= lmax) {
            UngetChar(ch, cb); cb_next = cb->next; /* TODO cb_next */
            break;
        }
        NOTE_NEW_LEXEM_POSITION(cb, lb, cb_lineNumber, cb_lineBegin);
        /*  yytext = ccc; */
        lexStartDd = dd;
        lexStartCol = columnPosition(cb, cb_lineBegin, cb_columnOffset);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            ProcessIdentifier(ch, cb, dd, cb_next, cb_lineNumber, cb_lineBegin, cb_columnOffset, lab2);
            goto nextLexem;
        } else if (isdigit(ch)) {
            /* ***************   number *******************************  */
            long unsigned val=0;
            lexStartFilePos = absoluteFilePosition(cb, cb_end, cb_next);
            if (ch=='0') {
                LexGetChar(ch, cb, cb_next);
                if (ch=='x' || ch=='X') {
                    /* hexa */
                    LexGetChar(ch, cb, cb_next);
                    while (isdigit(ch)||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F')) {
                        if (ch>='a') val = val*16+ch-'a'+10;
                        else if (ch>='A') val = val*16+ch-'A'+10;
                        else val = val*16+ch-'0';
                        LexGetChar(ch, cb, cb_next);
                    }
                } else {
                    /* octal */
                    while (isdigit(ch) && ch<='8') {
                        val = val*8+ch-'0';
                        LexGetChar(ch, cb, cb_next);
                    }
                }
            } else {
                /* decimal */
                while (isdigit(ch)) {
                    val = val*10+ch-'0';
                    LexGetChar(ch, cb, cb_next);
                }
            }
            if (ch == '.' || ch=='e' || ch=='E'
                || ((ch=='d' || ch=='D'|| ch=='f' || ch=='F') && LANGUAGE(LANG_JAVA))) {
                /* floating point */
                FloatingPointConstant(ch, cb, cb_next, rlex);
                PutLexToken(rlex,dd);
                PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                PutLexInt(absoluteFilePosition(cb, cb_end, cb_next)-lexStartFilePos, dd);
                goto nextLexem;
            }
            /* integer */
            ConType(ch, cb, cb_next, rlex);
            PutLexToken(rlex,dd);
            PutLexInt(val,dd);
            PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
            PutLexInt(absoluteFilePosition(cb, cb_end, cb_next)-lexStartFilePos, dd);
            goto nextLexem;
        } else switch (ch) {
                /* ************   special character *********************  */
            case '.':
                lexStartFilePos = absoluteFilePosition(cb, cb_end, cb_next);
                LexGetChar(ch, cb, cb_next);
                if (ch == '.' && LANGUAGE(LANG_C|LANG_YACC)) {
                    LexGetChar(ch, cb, cb_next);
                    if (ch == '.') {
                        LexGetChar(ch, cb, cb_next);
                        PutLexToken(ELIPSIS,dd);
                        PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    } else {
                        UngetChar(ch, cb);
                        ch = '.';
                    }
                    PutLexToken('.',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    UngetChar(ch, cb); cb_next = cb->next; /* TODO cb_next */
                    ch = '.';
                    FloatingPointConstant(ch, cb, cb_next, rlex);
                    PutLexToken(rlex,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    PutLexInt(absoluteFilePosition(cb, cb_end, cb_next)-lexStartFilePos, dd);
                    goto nextLexem;
                } else {
                    PutLexToken('.',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '-':
                LexGetChar(ch, cb, cb_next);
                if (ch=='=') {
                    PutLexToken(SUB_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else if (ch=='-') {
                    PutLexToken(DEC_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    LexGetChar(ch, cb, cb_next);
                    PutLexToken(PTR_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);goto nextLexem;
                } else {PutLexToken('-',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);goto nextLexem;}

            case '+':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(ADD_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else if (ch == '+') {
                    PutLexToken(INC_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else {
                    PutLexToken('+',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '>':
                LexGetChar(ch, cb, cb_next);
                if (ch == '>') {
                    LexGetChar(ch, cb, cb_next);
                    if(ch=='>' && LANGUAGE(LANG_JAVA)) {
                        LexGetChar(ch, cb, cb_next);
                        if(ch=='=') {
                            PutLexToken(URIGHT_ASSIGN,dd);
                            PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                            LexGetChar(ch, cb, cb_next);
                            goto nextLexem;
                        } else {
                            PutLexToken(URIGHT_OP,dd);
                            PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        PutLexToken(RIGHT_ASSIGN,dd);
                        PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                        LexGetChar(ch, cb, cb_next);
                        goto nextLexem;
                    } else {
                        PutLexToken(RIGHT_OP,dd);
                        PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    PutLexToken(GE_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else {
                    PutLexToken('>',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '<':
                LexGetChar(ch, cb, cb_next);
                if (ch == '<') {
                    LexGetChar(ch, cb, cb_next);
                    if(ch=='=') {
                        PutLexToken(LEFT_ASSIGN,dd);
                        PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                        LexGetChar(ch, cb, cb_next);
                        goto nextLexem;
                    } else {
                        PutLexToken(LEFT_OP,dd);
                        PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    PutLexToken(LE_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else {
                    PutLexToken('<',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '*':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(MUL_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else {
                    PutLexToken('*',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '%':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(MOD_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      PutLexToken(YACC_PERC_LPAR,dd);
                      PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                      LexGetChar(ch, cb, cb_next, cb_end, cb_lineBegin);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      PutLexToken(YACC_PERC_RPAR,dd);
                      PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                      LexGetChar(ch, cb, cb_next, cb_end, cb_lineBegin);
                      goto nextLexem;
                  }
                  &*/
                else {
                    PutLexToken('%',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '&':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(AND_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else if (ch == '&') {
                    PutLexToken(AND_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                }
                else if (ch == '*') {
                    LexGetChar(ch, cb, cb_next);
                    if (ch == '/') {
                        /* a code comment, ignore */
                        LexGetChar(ch, cb, cb_next);
                        CommentaryEndRef(cb, cb_next, cb_lineNumber, cb_lineBegin, 0);
                        goto nextLexem;
                    } else {
                        UngetChar(ch, cb);
                        ch = '*';
                        PutLexToken('&',dd);
                        PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    }
                } else {
                    PutLexToken('&',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '^':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(XOR_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else {
                    PutLexToken('^',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '|':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(OR_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                } else if (ch == '|') {
                    PutLexToken(OR_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                } else {
                    PutLexToken('|',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '=':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(EQ_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                } else {
                    PutLexToken('=',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '!':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(NE_OP,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                } else {PutLexToken('!',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case ':':
                LexGetChar(ch, cb, cb_next);
                PutLexToken(':',dd);
                PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                goto nextLexem;

            case '\'':
                chval = 0;
                lexStartFilePos = absoluteFilePosition(cb,cb_end,cb_next);
                do {
                    LexGetChar(ch, cb, cb_next);
                    while (ch=='\\') {
                        LexGetChar(ch, cb, cb_next);
                        /* TODO escape sequences */
                        LexGetChar(ch, cb, cb_next);
                    }
                    if (ch != '\'') chval = chval * 256 + ch;
                } while (ch != '\'' && ch != '\n');
                if (ch=='\'') {
                    PutLexToken(CHAR_LITERAL,dd);
                    PutLexInt(chval,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    PutLexInt(absoluteFilePosition(cb, cb_end, cb_next)-lexStartFilePos, dd);
                    LexGetChar(ch, cb, cb_next);
                }
                goto nextLexem;

            case '\"':
                line = cb_lineNumber;
                size = 0;
                PutLexToken(STRING_LITERAL,dd);
                do {
                    LexGetChar(ch, cb, cb_next);
                    size ++;
                    if (ch!='\"' && size<MAX_LEXEM_SIZE-10)
                        PutLexChar(ch,dd);
                    if (ch=='\\') {
                        LexGetChar(ch, cb, cb_next);
                        size ++;
                        if (size < MAX_LEXEM_SIZE-10) PutLexChar(ch,dd);
                        /* TODO escape sequences */
                        if (ch == '\n') {cb_lineNumber ++;
                            cb_lineBegin = cb_next;
                            cb_columnOffset = 0;}
                        ch = 0;
                    }
                    if (ch == '\n') {
                        cb_lineNumber ++;
                        cb_lineBegin = cb_next;
                        cb_columnOffset = 0;
                        if (s_opt.strictAnsi && (s_opt.debug || s_opt.show_errors)) {
                            warningMessage(ERR_ST,"string constant through end of line");
                        }
                    }
                    // in Java CR LF can't be a part of string, even there
                    // are benchmarks making Xrefactory coredump if CR or LF
                    // is a part of strings
                } while (ch != '\"' && (ch != '\n' || !s_opt.strictAnsi) && ch != -1);
                if (ch == -1 && s_opt.taskRegime!=RegimeEditServer) {
                    warningMessage(ERR_ST,"string constant through EOF");
                }
                PutLexChar(0,dd);
                PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                PutLexLine(cb_lineNumber-line,dd);
                LexGetChar(ch, cb, cb_next);
                goto nextLexem;

            case '/':
                LexGetChar(ch, cb, cb_next);
                if (ch == '=') {
                    PutLexToken(DIV_ASSIGN,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                    LexGetChar(ch, cb, cb_next);
                    goto nextLexem;
                } else if (ch=='*') {
                    int javadoc=0;
                    CommentaryBegRef(cb, cb_next, cb_lineNumber, cb_lineBegin);
                    LexGetChar(ch, cb, cb_next);
                    if (ch == '&') {
                        /* a program comment, ignore and continue with next lexem */
                        LexGetChar(ch, cb, cb_next);
                        goto nextLexem;
                    } else {
                        if (ch=='*' && LANGUAGE(LANG_JAVA))
                            javadoc = 1;
                        UngetChar(ch, cb); cb_next = cb->next; /* TODO cb_next */
                        ch = '*';
                    }   /* !!! COPY BLOCK TO '/n' */
                    PassComment(ch, cb, cb_next, cb_end, dd, cb_lineNumber, cb_lineBegin, cb_columnOffset);
                    CommentaryEndRef(cb, cb_next, cb_lineNumber, cb_lineBegin, javadoc);
                    goto nextLexem;
                } else if (ch=='/' && s_opt.cpp_comment) {
                    /*  ******* a // comment ******* */
                    CommentaryBegRef(cb, cb_next, cb_lineNumber, cb_lineBegin);
                    LexGetChar(ch, cb, cb_next);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        LexGetChar(ch, cb, cb_next);
                        CommentaryEndRef(cb, cb_next, cb_lineNumber, cb_lineBegin, 0);
                        goto nextLexem;
                    }
                    line = cb_lineNumber;
                    while (ch!='\n' && ch != -1) {
                        LexGetChar(ch, cb, cb_next);
                        if (ch == '\\') {
                            LexGetChar(ch, cb, cb_next);
                            if (ch=='\n') {
                                cb_lineNumber ++;
                                cb_lineBegin = cb_next;
                                cb_columnOffset = 0;}
                            LexGetChar(ch, cb, cb_next);
                        }
                    }
                    CommentaryEndRef(cb, cb_next, cb_lineNumber, cb_lineBegin, 0);
                    PutLexLine(cb_lineNumber-line,dd);
                } else {
                    PutLexToken('/',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '\\':
                LexGetChar(ch, cb, cb_next);
                if (ch == '\n') {
                    cb_lineNumber ++;
                    cb_lineBegin = cb_next;
                    cb_columnOffset = 0;
                    PutLexLine(1, dd);
                    LexGetChar(ch, cb, cb_next);
                } else {
                    PutLexToken('\\',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '\n':
                column = columnPosition(cb, cb_lineBegin, cb_columnOffset);
                if (column >= MAX_REFERENCABLE_COLUMN) {
                    fatalError(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                if (cb_lineNumber >= MAX_REFERENCABLE_LINE) {
                    fatalError(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                cb_lineNumber ++;
                cb_lineBegin = cb_next;
                cb_columnOffset = 0;
                LexGetChar(ch, cb, cb_next);
                DeleteBlank(ch, cb, cb_next);
                if (ch == '/') {
                    LexGetChar(ch, cb, cb_next);
                    if (ch == '*') {
                        CommentaryBegRef(cb, cb_next, cb_lineNumber, cb_lineBegin);
                        LexGetChar(ch, cb, cb_next);
                        if (ch == '&') {
                            /* ****** a code comment, ignore */
                            LexGetChar(ch, cb, cb_next);
                        } else {
                            int javadoc=0;
                            if (ch == '*' && LANGUAGE(LANG_JAVA)) javadoc = 1;
                            UngetChar(ch, cb); cb_next = cb->next;
                            ch = '*';
                            PassComment(ch, cb, cb_next, cb_end, dd, cb_lineNumber, cb_lineBegin, cb_columnOffset);
                            CommentaryEndRef(cb, cb_next, cb_lineNumber, cb_lineBegin, javadoc);
                            DeleteBlank(ch, cb, cb_next);
                        }
                    } else {
                        UngetChar(ch, cb); cb_next = cb->next; /* TODO cb_next */
                        ch = '/';
                    }
                }
                PutLexToken('\n',dd);
                PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                if (ch == '#' && LANGUAGE(LANG_C|LANG_YACC)) {
                    NOTE_NEW_LEXEM_POSITION(cb, lb, cb_lineNumber, cb_lineBegin);
                    HandleCppToken(ch, cb, dd, cb_next, cb_fileNumber, cb_lineNumber, cb_lineBegin);
                }
                goto nextLexem;

            case '#':
                LexGetChar(ch, cb, cb_next);
                if (ch == '#') {
                    LexGetChar(ch, cb, cb_next);
                    PutLexToken(CPP_COLLATION,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                } else {
                    PutLexToken('#',dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    PutLexToken(ch,dd);
                    PutLexPosition(cb_fileNumber, cb_lineNumber, lexStartCol, dd);
                }
                LexGetChar(ch, cb, cb_next);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (s_opt.taskRegime == RegimeEditServer) {
            int pi,len,lastlex,parChar,apos;
            Position *ps;
            int pos1,currentLexemPosition;
            pi = (lb->index-1) % LEX_POSITIONS_RING_SIZE;
            ps = & lb->positionRing[pi];
            currentLexemPosition = lb->fileOffsetRing[pi];
            if (    cb_fileNumber == s_olOriginalFileNumber
                    && cb_fileNumber != s_noneFileIndex
                    && cb_fileNumber != -1
                    && s_jsl==NULL
                    ) {
                if (s_opt.server_operation == OLO_EXTRACT && lb->index>=2) {
                    DeleteBlank(ch, cb, cb_next);
                    pos1 = absoluteFilePosition(cb, cb_end, cb_next);
                    //&idcoll = columnPosition(cb, cb_lineBegin, cb_columnOffset);
                    //&fprintf(dumpOut,":pos1==%d, olCursorPos==%d, olMarkPos==%d\n",pos1,s_opt.olCursorPos,s_opt.olMarkPos);
                    // all this is very, very HACK!!!
                    if (pos1 >= s_opt.olCursorPos && ! s_cps.marker1Flag) {
                        if (LANGUAGE(LANG_JAVA)) parChar = ';';
                        else {
                            if (s_cps.marker2Flag) parChar='}';
                            else parChar = '{';
                        }
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(OL_MARKER_TOKEN,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        s_cps.marker1Flag=1;
                    } else if (pos1 >= s_opt.olMarkPos && ! s_cps.marker2Flag){
                        if (LANGUAGE(LANG_JAVA)) parChar = ';';
                        else {
                            if (s_cps.marker1Flag) parChar='}';
                            else parChar = '{';
                        }
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(';',dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(OL_MARKER_TOKEN,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        PutLexToken(parChar,dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        s_cps.marker2Flag=1;
                    }
                } else if (s_opt.server_operation == OLO_COMPLETION
                           ||  s_opt.server_operation == OLO_SEARCH) {
                    DeleteBlank(ch, cb, cb_next);
                    apos = absoluteFilePosition(cb, cb_end, cb_next);
                    if (currentLexemPosition < s_opt.olCursorPos
                        && (apos >= s_opt.olCursorPos
                            || (ch == -1 && apos+1 == s_opt.olCursorPos))) {
                        //&sprintf(tmpBuff,"currentLexemPosition, s_opt.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d\n",currentLexemPosition, s_opt.olCursorPos, apos, ch);ppcGenTmpBuff();
                        //&fprintf(dumpOut,":check\n");fflush(dumpOut);
                        lastlex = NextLexToken(lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            len = s_opt.olCursorPos-currentLexemPosition;
                            //&fprintf(dumpOut,":check %s[%d] <-> %d\n", lexStartDd+TOKEN_SIZE, len,strlen(lexStartDd+TOKEN_SIZE));fflush(dumpOut);
                            if (len <= strlen(lexStartDd+TOKEN_SIZE)) {
                                if (s_opt.server_operation == OLO_SEARCH) {
                                    char *ddd;
                                    ddd = lexStartDd;
                                    PutLexToken(IDENT_TO_COMPLETE, ddd);
                                } else {
                                    dd = lexStartDd;
                                    PutLexToken(IDENT_TO_COMPLETE, dd);
                                    dd += len;
                                    PutLexChar(0,dd);
                                    PutLexPosition(ps->file,ps->line,ps->col,dd);
                                }
                                //&fprintf(dumpOut,":ress %s\n", lexStartDd+TOKEN_SIZE);fflush(dumpOut);
                            } else {
                                // completion after an identifier
                                PUT_EMPTY_COMPLETION_ID(cb, dd, cb_lineNumber,
                                                        cb_columnOffset, cb_lineBegin,
                                                        cb_fileNumber,
                                                        apos-s_opt.olCursorPos);
                            }
                        } else if ((lastlex == LINE_TOK || lastlex == STRING_LITERAL)
                                   && (apos-s_opt.olCursorPos != 0)) {
                            // completion inside special lexems, do
                            // NO COMPLETION
                        } else {
                            // completion after another lexem
                            PUT_EMPTY_COMPLETION_ID(cb, dd, cb_lineNumber,
                                                    cb_columnOffset,cb_lineBegin,
                                                    cb_fileNumber,
                                                    apos-s_opt.olCursorPos);
                        }
                    }
                    // TODO, make this in a more standard way, !!!
                } else {
                    //&fprintf(dumpOut,":testing %d <= %d <= %d\n", currentLexemPosition, s_opt.olCursorPos, absoluteFilePosition(cb, cb_end, cb_next));
                    if (currentLexemPosition <= s_opt.olCursorPos
                        && absoluteFilePosition(cb, cb_end, cb_next) >= s_opt.olCursorPos) {
                        gotOnLineCxRefs( ps);
                        lastlex = NextLexToken(lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            strcpy(s_olstring, lexStartDd+TOKEN_SIZE);
                        }
                    }
                    if (LANGUAGE(LANG_JAVA)) {
                        // there is a problem with this, when browsing at CPP construction
                        // that is why I restrict it to Java language! It is usefull
                        // only for Java refactorings
                        DeleteBlank(ch, cb, cb_next);
                        apos = absoluteFilePosition(cb, cb_end, cb_next);
                        if (apos >= s_opt.olCursorPos && ! s_cps.marker1Flag) {
                            PutLexToken(OL_MARKER_TOKEN,dd);
                            PutLexPosition(ps->file,ps->line,ps->col,dd);
                            s_cps.marker1Flag=1;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    cb->next = cb_next;
    cb->end = cb_end;
    cb->lineNumber = cb_lineNumber;
    cb->lineBegin = cb_lineBegin;
    cb->columnOffset = cb_columnOffset;
    lb->end = dd;

    if (lb->end == lb->chars)
        return false;
    else
        return true;
}

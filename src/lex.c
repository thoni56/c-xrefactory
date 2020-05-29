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

static int columnPosition(CharacterBuffer *cb) {
    return cb->next - cb->lineBegin + cb->columnOffset - 1;
}


static int absoluteFilePosition(CharacterBuffer *cb) {
    return cb->filePos - (cb->end - cb->next) - 1;
}


static int getChar(CharacterBuffer *cb) {
    int ch;
    if (cb->next >= cb->end) {
        /* No more characters in buffer? */
        if (cb->isAtEOF) {
            ch = -1;
        } else if (!refillBuffer(cb)) {
            ch = -1;
            cb->isAtEOF = true;
        } else {
            /* TODO This never happens! Why? */
            cb->lineBegin = cb->next;
            ch = *((unsigned char *)cb->next);
            cb->next++;
        }
    } else {
        ch = * ((unsigned char *)cb->next);
        cb->next++;
    }

    return ch;
}


static void ungetChar(CharacterBuffer *cb, int ch) {
    if (ch == '\n')
        log_trace("Ungetting ('\\n')");
    else
        log_trace("Ungetting ('%c')", ch);
    *--(cb->next) = ch;
}


static int skipBlanks(CharacterBuffer *cb, int ch) {
    while (ch==' '|| ch=='\t' || ch=='\004') {
        ch = getChar(cb);
    }
    return ch;
}


static void passComment(CharacterBuffer *cb) {
    int ch;
    char oldCh;

    /*  ******* a block comment ******* */
    ch = getChar(cb);
    if (ch=='\n') {
        cb->lineNumber ++;
        cb->lineBegin = cb->next;
        cb->columnOffset = 0;
    }
    /* TODO test on cpp directive */
    do {
        oldCh = ch;
        ch = getChar(cb);
        if (ch=='\n') {
            cb->lineNumber ++;
            cb->lineBegin = cb->next;
            cb->columnOffset = 0;
        }
        /* TODO test on cpp directive */
    } while ((oldCh != '*' || ch != '/') && ch != -1);
    if (ch == -1)
        warningMessage(ERR_ST,"comment through eof");
}


#define ConType(ch, cb, rlex) {                                         \
        rlex = CONSTANT;                                                \
        if (LANGUAGE(LANG_JAVA)) {                                      \
            if (ch=='l' || ch=='L') {                                   \
                rlex = LONG_CONSTANT;                                   \
                ch = getChar(cb);                                       \
            }                                                           \
        } else {                                                        \
            for(; ch=='l'||ch=='L'||ch=='u'||ch=='U'; ){                \
                if (ch=='l' || ch=='L')                                 \
                    rlex = LONG_CONSTANT;                               \
                ch = getChar(cb);                                       \
            }                                                           \
        }                                                               \
    }

#define FloatingPointConstant(ch, cb, lexem) {                          \
        lexem = DOUBLE_CONSTANT;                                        \
        if (ch == '.') {                                                \
            do {                                                        \
                ch = getChar(cb);                                       \
            } while (isdigit(ch));                                      \
        }                                                               \
        if (ch == 'e' || ch == 'E') {                                   \
            ch = getChar(cb);                                           \
            if (ch == '+' || ch=='-')                                   \
                ch = getChar(cb);                                       \
            while (isdigit(ch))                                         \
                ch = getChar(cb);                                       \
        }                                                               \
        if (LANGUAGE(LANG_JAVA)) {                                      \
            if (ch == 'f' || ch == 'F' || ch == 'd' || ch == 'D') {     \
                if (ch == 'f' || ch == 'F')                             \
                    lexem = FLOAT_CONSTANT;                             \
                ch = getChar(cb);                                       \
            }                                                           \
        } else {                                                        \
            if (ch == 'f' || ch == 'F' || ch == 'l' || ch == 'L') {     \
                ch = getChar(cb);                                       \
            }                                                           \
        }                                                               \
    }

#define ProcessIdentifier(ch, cb, dd, labelSuffix) {                    \
        int idcoll;                                                     \
        char *ddd;                                                      \
        /* ***************  identifier ****************************  */ \
        ddd = dd;                                                       \
        idcoll = columnPosition(cb);                     \
        PutLexToken(IDENTIFIER,dd);                                     \
        do {                                                            \
            PutLexChar(ch,dd);                                          \
        identCont##labelSuffix:                                         \
            ch = getChar(cb);                                           \
        } while (isalpha(ch) || isdigit(ch) || ch=='_' || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))); \
        if (ch == '@' && *(dd-1)=='C') {                                \
            int i,len;                                                  \
            /* NO TEST COVERAGE FOR THIS... Don't know what it is...*/  \
            assert(0);                                                  \
            len = strlen(s_editCommunicationString);                    \
            for (i=2;i<len;i++) {                                       \
                ch = getChar(cb);                                       \
                if (ch != s_editCommunicationString[i])                 \
                    break;                                              \
            }                                                           \
            if (i>=len) {                                               \
                /* it is the place marker */                            \
                dd--; /* delete the C */                                \
                ch = getChar(cb);                                       \
                if (ch == CC_COMPLETION) {                              \
                    PutLexToken(IDENT_TO_COMPLETE, ddd);                \
                    ch = getChar(cb);                                   \
                } else if (ch == CC_CXREF) {                            \
                    s_cache.activeCache = 0;                            \
                    fillPosition(&s_cxRefPos, cb->fileNumber, cb->lineNumber, idcoll); \
                    goto identCont##labelSuffix;                        \
                } else errorMessage(ERR_INTERNAL, "unknown communication char"); \
            } else {                                                    \
                /* not a place marker, undo reading */                  \
                for(i--;i>=1;i--) {                                     \
                    ungetChar(cb, ch);                                  \
                    ch = s_editCommunicationString[i];                  \
                }                                                       \
            }                                                           \
        }                                                               \
        PutLexChar(0,dd);                                               \
        PutLexPosition(cb->fileNumber, cb->lineNumber, idcoll, dd);     \
    }

#define CommentaryBegRef(cb) {                                          \
        if (s_opt.taskRegime==RegimeHtmlGenerate && !s_opt.htmlNoColors) { \
            int lcoll;                                                  \
            Position pos;                                               \
            char ttt[TMP_STRING_SIZE];                                  \
            lcoll = columnPosition(cb) - 1; \
            fillPosition(&pos, cb->fileNumber, cb->lineNumber, lcoll);   \
            sprintf(ttt,"%x/*", cb->fileNumber);                        \
            addTrivialCxReference(ttt, TypeComment, StorageDefault, &pos, UsageDefined); \
        }                                                               \
    }

#define CommentaryEndRef(cb, jdoc) {                                    \
        if (s_opt.taskRegime==RegimeHtmlGenerate) {                     \
            int lcoll;                                                  \
            Position pos;                                               \
            char ttt[TMP_STRING_SIZE];                                  \
            lcoll = columnPosition(cb); \
            fillPosition(&pos, cb->fileNumber, cb->lineNumber, lcoll);  \
            sprintf(ttt,"%x/*", cb->fileNumber);                        \
            if (!s_opt.htmlNoColors) {                                  \
                addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageUsed); \
            }                                                           \
            if (jdoc) {                                                 \
                pos.col -= 2;                                           \
                addTrivialCxReference(ttt,TypeComment,StorageDefault, &pos, UsageJavaDoc); \
            }                                                           \
        }                                                               \
    }

#define NOTE_NEW_LEXEM_POSITION(cb, lb) {                               \
        int index = lb->index % LEX_POSITIONS_RING_SIZE;                \
        lb->fileOffsetRing[index] = absoluteFilePosition(cb);           \
        lb->positionRing[index].file = cb->fileNumber;                  \
        lb->positionRing[index].line = cb->lineNumber;                  \
        lb->positionRing[index].col = columnPosition(cb);               \
        lb->index ++;                                                   \
    }

#define PUT_EMPTY_COMPLETION_ID(cb, dd, len) {                          \
        PutLexToken(IDENT_TO_COMPLETE, dd);                             \
        PutLexChar(0, dd);                                              \
        PutLexPosition(cb->fileNumber, cb->lineNumber,                  \
                       columnPosition(cb) - (len), dd);                 \
    }


bool getLexBuf(S_lexBuf *lb) {
    int ch;
    CharacterBuffer *cb;
    char *cc, *dd, *lmax, *lexStartDd;
    unsigned chval=0;
    int rlex;
    int line, size, lexStartCol, lexStartFilePos, column;

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
    cb->lineNumber = cb->lineNumber;
    cb->fileNumber = cb->fileNumber;

    ch = getChar(cb);
    do {
        ch = skipBlanks(cb, ch);
        if (dd >= lmax) {
            ungetChar(cb, ch);
            break;
        }
        NOTE_NEW_LEXEM_POSITION(cb, lb);
        /*  yytext = ccc; */
        lexStartDd = dd;
        lexStartCol = columnPosition(cb);
        log_trace("lexStartCol = %d", lexStartCol);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            ProcessIdentifier(ch, cb, dd, lab2);
            goto nextLexem;
        } else if (isdigit(ch)) {
            /* ***************   number *******************************  */
            long unsigned val=0;
            lexStartFilePos = absoluteFilePosition(cb);
            if (ch=='0') {
                ch = getChar(cb);
                if (ch=='x' || ch=='X') {
                    /* hexa */
                    ch = getChar(cb);
                    while (isdigit(ch)||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F')) {
                        if (ch>='a') val = val*16+ch-'a'+10;
                        else if (ch>='A') val = val*16+ch-'A'+10;
                        else val = val*16+ch-'0';
                        ch = getChar(cb);
                    }
                } else {
                    /* octal */
                    while (isdigit(ch) && ch<='8') {
                        val = val*8+ch-'0';
                        ch = getChar(cb);
                    }
                }
            } else {
                /* decimal */
                while (isdigit(ch)) {
                    val = val*10+ch-'0';
                    ch = getChar(cb);
                }
            }
            if (ch == '.' || ch=='e' || ch=='E'
                || ((ch=='d' || ch=='D'|| ch=='f' || ch=='F') && LANGUAGE(LANG_JAVA))) {
                /* floating point */
                FloatingPointConstant(ch, cb, rlex);
                PutLexToken(rlex,dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                PutLexInt(absoluteFilePosition(cb)-lexStartFilePos, dd);
                goto nextLexem;
            }
            /* integer */
            ConType(ch, cb, rlex);
            PutLexToken(rlex,dd);
            PutLexInt(val,dd);
            PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
            PutLexInt(absoluteFilePosition(cb)-lexStartFilePos, dd);
            goto nextLexem;
        } else switch (ch) {
                /* ************   special character *********************  */
            case '.':
                lexStartFilePos = absoluteFilePosition(cb);
                ch = getChar(cb);
                if (ch == '.' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    if (ch == '.') {
                        ch = getChar(cb);
                        PutLexToken(ELIPSIS,dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    PutLexToken('.',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    FloatingPointConstant(ch, cb, rlex);
                    PutLexToken(rlex,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    PutLexInt(absoluteFilePosition(cb)-lexStartFilePos, dd);
                    goto nextLexem;
                } else {
                    PutLexToken('.',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    PutLexToken(SUB_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    PutLexToken(DEC_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    PutLexToken(PTR_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                } else {PutLexToken('-',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(ADD_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    PutLexToken(INC_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    PutLexToken('+',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '>':
                ch = getChar(cb);
                if (ch == '>') {
                    ch = getChar(cb);
                    if(ch=='>' && LANGUAGE(LANG_JAVA)) {
                        ch = getChar(cb);
                        if(ch=='=') {
                            PutLexToken(URIGHT_ASSIGN,dd);
                            PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                            ch = getChar(cb);
                            goto nextLexem;
                        } else {
                            PutLexToken(URIGHT_OP,dd);
                            PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        PutLexToken(RIGHT_ASSIGN,dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        PutLexToken(RIGHT_OP,dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    PutLexToken(GE_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    PutLexToken('>',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if(ch=='=') {
                        PutLexToken(LEFT_ASSIGN,dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        PutLexToken(LEFT_OP,dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    PutLexToken(LE_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    PutLexToken('<',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(MUL_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    PutLexToken('*',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(MOD_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      PutLexToken(YACC_PERC_LPAR,dd);
                      PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      PutLexToken(YACC_PERC_RPAR,dd);
                      PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    PutLexToken('%',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(AND_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    PutLexToken(AND_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                else if (ch == '*') {
                    ch = getChar(cb);
                    if (ch == '/') {
                        /* a code comment, ignore */
                        ch = getChar(cb);
                        CommentaryEndRef(cb, 0);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '*';
                        PutLexToken('&',dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                        goto nextLexem;
                    }
                } else {
                    PutLexToken('&',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(XOR_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    PutLexToken('^',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(OR_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                } else if (ch == '|') {
                    PutLexToken(OR_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                } else {
                    PutLexToken('|',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(EQ_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                } else {
                    PutLexToken('=',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(NE_OP,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                } else {PutLexToken('!',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                PutLexToken(':',dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                goto nextLexem;

            case '\'':
                chval = 0;
                lexStartFilePos = absoluteFilePosition(cb);
                do {
                    ch = getChar(cb);
                    while (ch=='\\') {
                        ch = getChar(cb);
                        /* TODO escape sequences */
                        ch = getChar(cb);
                    }
                    if (ch != '\'') chval = chval * 256 + ch;
                } while (ch != '\'' && ch != '\n');
                if (ch=='\'') {
                    PutLexToken(CHAR_LITERAL,dd);
                    PutLexInt(chval,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    PutLexInt(absoluteFilePosition(cb)-lexStartFilePos, dd);
                    ch = getChar(cb);
                }
                goto nextLexem;

            case '\"':
                line = cb->lineNumber;
                size = 0;
                PutLexToken(STRING_LITERAL,dd);
                do {
                    ch = getChar(cb);
                    size ++;
                    if (ch!='\"' && size<MAX_LEXEM_SIZE-10)
                        PutLexChar(ch,dd);
                    if (ch=='\\') {
                        ch = getChar(cb);
                        size ++;
                        if (size < MAX_LEXEM_SIZE-10) PutLexChar(ch,dd);
                        /* TODO escape sequences */
                        if (ch == '\n') {cb->lineNumber ++;
                            cb->lineBegin = cb->next;
                            cb->columnOffset = 0;
                        }
                        ch = 0;
                    }
                    if (ch == '\n') {
                        cb->lineNumber ++;
                        cb->lineBegin = cb->next;
                        cb->columnOffset = 0;
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
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                PutLexLine(cb->lineNumber-line,dd);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    PutLexToken(DIV_ASSIGN,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='*') {
                    bool isJavadoc=false;
                    CommentaryBegRef(cb);
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* a program comment, ignore and continue with next lexem */
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        if (ch=='*' && LANGUAGE(LANG_JAVA))
                            isJavadoc = true;
                        ungetChar(cb, ch);
                        ch = '*';
                    }   /* !!! COPY BLOCK TO '/n' */

                    passComment(cb);
                    PutLexLine(cb->lineNumber-line,dd);
                    ch = getChar(cb);

                    CommentaryEndRef(cb, isJavadoc);
                    goto nextLexem;
                } else if (ch=='/' && s_opt.cpp_comment) {
                    /*  ******* a // comment ******* */
                    CommentaryBegRef(cb);
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        ch = getChar(cb);
                        CommentaryEndRef(cb, 0);
                        goto nextLexem;
                    }
                    line = cb->lineNumber;
                    while (ch!='\n' && ch != -1) {
                        ch = getChar(cb);
                        if (ch == '\\') {
                            ch = getChar(cb);
                            if (ch=='\n') {
                                cb->lineNumber ++;
                                cb->lineBegin = cb->next;
                                cb->columnOffset = 0;
                            }
                            ch = getChar(cb);
                        }
                    }
                    CommentaryEndRef(cb, 0);
                    PutLexLine(cb->lineNumber-line,dd);
                } else {
                    PutLexToken('/',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    cb->lineNumber ++;
                    cb->lineBegin = cb->next;
                    cb->columnOffset = 0;
                    PutLexLine(1, dd);
                    ch = getChar(cb);
                } else {
                    PutLexToken('\\',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case '\n':
                column = columnPosition(cb);
                if (column >= MAX_REFERENCABLE_COLUMN) {
                    fatalError(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                if (cb->lineNumber >= MAX_REFERENCABLE_LINE) {
                    fatalError(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                cb->lineNumber ++;
                cb->lineBegin = cb->next;
                cb->columnOffset = 0;
                ch = getChar(cb);
                ch = skipBlanks(cb, ch);
                if (ch == '/') {
                    ch = getChar(cb);
                    if (ch == '*') {
                        CommentaryBegRef(cb);
                        ch = getChar(cb);
                        if (ch == '&') {
                            /* ****** a code comment, ignore */
                            ch = getChar(cb);
                        } else {
                            int javadoc=0;
                            if (ch == '*' && LANGUAGE(LANG_JAVA))
                                javadoc = 1;
                            ungetChar(cb, ch);
                            ch = '*';

                            passComment(cb);
                            PutLexLine(cb->lineNumber-line,dd);
                            ch = getChar(cb);

                            CommentaryEndRef(cb, javadoc);
                            ch = skipBlanks(cb, ch);
                        }
                    } else {
                        ungetChar(cb, ch);
                        ch = '/';
                    }
                }
                PutLexToken('\n',dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                if (ch == '#' && LANGUAGE(LANG_C|LANG_YACC)) {
                    NOTE_NEW_LEXEM_POSITION(cb, lb);
                    //&        HandleCppToken(ch, cb, dd, cb->next, cb->fileNumber, cb->lineNumber, cb->lineBegin);
                    // #define HandleCppToken(ch, cb, dd, cb->next, cb->fileNumber, cb->lineNumber, cb->lineBegin) {
                    {
                        char *ddd, tt[10];
                        int i, lcoll, scol;
                        lcoll = columnPosition(cb);
                        ch = getChar(cb);
                        ch = skipBlanks(cb, ch);
                        for(i=0; i<9 && (isalpha(ch) || isdigit(ch) || ch=='_') ; i++) {
                            tt[i] = ch;
                            ch = getChar(cb);
                        }
                        tt[i]=0;
                        if (strcmp(tt,"ifdef") == 0) {
                            PutLexToken(CPP_IFDEF,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"ifndef") == 0) {
                            PutLexToken(CPP_IFNDEF,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"if") == 0) {
                            PutLexToken(CPP_IF,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"elif") == 0) {
                            PutLexToken(CPP_ELIF,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"undef") == 0) {
                            PutLexToken(CPP_UNDEF,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd); \
                        } else if (strcmp(tt,"else") == 0) {
                            PutLexToken(CPP_ELSE,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd); \
                        } else if (strcmp(tt,"endif") == 0) {
                            PutLexToken(CPP_ENDIF,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd); \
                        } else if (strcmp(tt,"include") == 0) {
                            char endCh;
                            PutLexToken(CPP_INCLUDE,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                            ch = skipBlanks(cb, ch);
                            if (ch == '\"' || ch == '<') {
                                if (ch == '\"') endCh = '\"';
                                else endCh = '>';
                                scol = columnPosition(cb);
                                PutLexToken(STRING_LITERAL,dd);
                                do {
                                    PutLexChar(ch,dd);
                                    ch = getChar(cb);
                                } while (ch!=endCh && ch!='\n');
                                PutLexChar(0,dd);
                                PutLexPosition(cb->fileNumber,cb->lineNumber,scol,dd);
                                if (ch == endCh)
                                    ch = getChar(cb);
                            }
                        } else if (strcmp(tt,"define") == 0) {
                            ddd = dd;
                            PutLexToken(CPP_DEFINE0,dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                            ch = skipBlanks(cb, ch);
                            NOTE_NEW_LEXEM_POSITION(cb, lb);
                            ProcessIdentifier(ch, cb, dd,lab1);
                            if (ch == '(') {
                                PutLexToken(CPP_DEFINE,ddd);
                            }
                        } else if (strcmp(tt,"pragma") == 0) {
                            PutLexToken(CPP_PRAGMA,dd); PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else {
                            PutLexToken(CPP_LINE,dd); PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        }
                    }
                }
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    PutLexToken(CPP_COLLATION,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                } else {
                    PutLexToken('#',dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    PutLexToken(ch,dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexStartCol, dd);
                }
                ch = getChar(cb);
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
            if (    cb->fileNumber == s_olOriginalFileNumber
                    && cb->fileNumber != s_noneFileIndex
                    && cb->fileNumber != -1
                    && s_jsl==NULL
                    ) {
                if (s_opt.server_operation == OLO_EXTRACT && lb->index>=2) {
                    ch = skipBlanks(cb, ch);
                    pos1 = absoluteFilePosition(cb);
                    log_trace(":pos1==%d, olCursorPos==%d, olMarkPos==%d",pos1,s_opt.olCursorPos,s_opt.olMarkPos);
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
                    ch = skipBlanks(cb, ch);
                    apos = absoluteFilePosition(cb);
                    if (currentLexemPosition < s_opt.olCursorPos
                        && (apos >= s_opt.olCursorPos
                            || (ch == -1 && apos+1 == s_opt.olCursorPos))) {
                        log_trace("currentLexemPosition, s_opt.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d",currentLexemPosition, s_opt.olCursorPos, apos, ch);
                        lastlex = NextLexToken(lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            len = s_opt.olCursorPos-currentLexemPosition;
                            log_trace(":check %s[%d] <-> %d", lexStartDd+TOKEN_SIZE, len,strlen(lexStartDd+TOKEN_SIZE));
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
                                log_trace(":ress %s", lexStartDd+TOKEN_SIZE);
                            } else {
                                // completion after an identifier
                                PUT_EMPTY_COMPLETION_ID(cb, dd,
                                                        apos-s_opt.olCursorPos);
                            }
                        } else if ((lastlex == LINE_TOK || lastlex == STRING_LITERAL)
                                   && (apos-s_opt.olCursorPos != 0)) {
                            // completion inside special lexems, do
                            // NO COMPLETION
                        } else {
                            // completion after another lexem
                            PUT_EMPTY_COMPLETION_ID(cb, dd,
                                                    apos-s_opt.olCursorPos);
                        }
                    }
                    // TODO, make this in a more standard way, !!!
                } else {
                    if (currentLexemPosition <= s_opt.olCursorPos
                        && absoluteFilePosition(cb) >= s_opt.olCursorPos) {
                        gotOnLineCxRefs(ps);
                        lastlex = NextLexToken(lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            strcpy(s_olstring, lexStartDd+TOKEN_SIZE);
                        }
                    }
                    if (LANGUAGE(LANG_JAVA)) {
                        // there is a problem with this, when browsing at CPP construction
                        // that is why I restrict it to Java language! It is usefull
                        // only for Java refactorings
                        ch = skipBlanks(cb, ch);
                        apos = absoluteFilePosition(cb);
                        if (apos >= s_opt.olCursorPos && ! s_cps.marker1Flag) {
                            PutLexToken(OL_MARKER_TOKEN, dd);
                            PutLexPosition(ps->file,ps->line,ps->col,dd);
                            s_cps.marker1Flag=1;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    cb->lineNumber = cb->lineNumber;
    lb->end = dd;

    if (lb->end == lb->chars)
        return false;
    else
        return true;
}

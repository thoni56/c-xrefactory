#include "lexer.h"

#include "globals.h"
#include "commons.h"
#include "cxref.h"
#include "yylex.h"
#include "filedescriptor.h"

#include "caching.h"            /* s_cache && cacheInput() */
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


static Lexem constantType(CharacterBuffer *cb, int *ch) {
    Lexem rlex;

    rlex = CONSTANT;
    if (LANGUAGE(LANG_JAVA)) {
        if (*ch=='l' || *ch=='L') {
            rlex = LONG_CONSTANT;
            *ch = getChar(cb);
        }
    } else {
        for(; *ch=='l'||*ch=='L'||*ch=='u'||*ch=='U'; ){
            if (*ch=='l' || *ch=='L')
                rlex = LONG_CONSTANT;
            *ch = getChar(cb);
        }
    }

    return rlex;
}


static Lexem floatingPointConstant(CharacterBuffer *cb, int *chPointer) {
    int ch = *chPointer;
    Lexem rlex;

    rlex = DOUBLE_CONSTANT;
    if (ch == '.') {
        do {
            ch = getChar(cb);
        } while (isdigit(ch));
    }
    if (ch == 'e' || ch == 'E') {
        ch = getChar(cb);
        if (ch == '+' || ch=='-')
            ch = getChar(cb);
        while (isdigit(ch))
            ch = getChar(cb);
    }
    if (LANGUAGE(LANG_JAVA)) {
        if (ch == 'f' || ch == 'F' || ch == 'd' || ch == 'D') {
            if (ch == 'f' || ch == 'F')
                rlex = FLOAT_CONSTANT;
            ch = getChar(cb);
        }
    } else {
        if (ch == 'f' || ch == 'F' || ch == 'l' || ch == 'L') {
            ch = getChar(cb);
        }
    }
    *chPointer = ch;

    return rlex;
}


#define ProcessIdentifier(ch, cb, dd, labelSuffix) {                    \
        int idcoll;                                                     \
        char *ddd;                                                      \
        /* ***************  identifier ****************************  */ \
        ddd = dd;                                                       \
        idcoll = columnPosition(cb);                                    \
        putLexToken(IDENTIFIER, &dd);                                   \
        do {                                                            \
            putLexChar(ch, &dd);                                        \
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
                    putLexToken(IDENT_TO_COMPLETE, &ddd);                \
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
        putLexChar(0, &dd);                                             \
        PutLexPosition(cb->fileNumber, cb->lineNumber, idcoll, dd);     \
    }

#define CommentBeginReference(cb) {                                     \
        if (options.taskRegime==RegimeHtmlGenerate && !options.htmlNoColors) { \
            int lcoll;                                                  \
            Position pos;                                               \
            char ttt[TMP_STRING_SIZE];                                  \
            lcoll = columnPosition(cb) - 1;                             \
            fillPosition(&pos, cb->fileNumber, cb->lineNumber, lcoll);  \
            sprintf(ttt,"%x/*", cb->fileNumber);                        \
            addTrivialCxReference(ttt, TypeComment, StorageDefault, &pos, UsageDefined); \
        }                                                               \
    }

#define CommentEndReference(cb, jdoc) {                                 \
        if (options.taskRegime==RegimeHtmlGenerate) {                   \
            int lcoll;                                                  \
            Position pos;                                               \
            char ttt[TMP_STRING_SIZE];                                  \
            lcoll = columnPosition(cb);                                 \
            fillPosition(&pos, cb->fileNumber, cb->lineNumber, lcoll);  \
            sprintf(ttt,"%x/*", cb->fileNumber);                        \
            if (!options.htmlNoColors) {                                \
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
        putLexToken(IDENT_TO_COMPLETE, &dd);                            \
        putLexChar(0, &dd);                                             \
        PutLexPosition(cb->fileNumber, cb->lineNumber,                  \
                       columnPosition(cb) - (len), dd);                 \
    }


bool getLexem(LexemBuffer *lb) {
    int ch;
    CharacterBuffer *cb;
    char *cc, *lmax, *lexStartDd;
    char *dd;                   /* TODO: Destination, a.k.a where to put lexems? Maybe? */
    unsigned chval=0;
    Lexem rlex;
    int line, size, lexemStartingColumn, lexStartFilePos, column;

    /* first test whether the input is cached */
    if (s_cache.activeCache && includeStackPointer==0 && macroStackIndex==0) {
        cacheInput();
        s_cache.lexcc = lb->chars;
    }

    lmax = lb->chars + LEX_BUFF_SIZE - MAX_LEXEM_SIZE;
    for(dd=lb->chars,cc=lb->next; cc<lb->end; cc++,dd++)
        *dd = *cc;
    lb->next = lb->chars;

    cb = &lb->buffer;
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
        lexemStartingColumn = columnPosition(cb);
        log_trace("lexStartCol = %d", lexemStartingColumn);
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
                rlex = floatingPointConstant(cb, &ch);
                putLexToken(rlex, &dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                goto nextLexem;
            }
            /* integer */
            rlex = constantType(cb, &ch);
            putLexToken(rlex, &dd);
            putLexInt(val, &dd);
            PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
            putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
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
                        putLexToken(ELIPSIS, &dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    putLexToken('.', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    rlex = floatingPointConstant(cb, &ch);
                    putLexToken(rlex, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                    goto nextLexem;
                } else {
                    putLexToken('.', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    putLexToken(SUB_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    putLexToken(DEC_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    putLexToken(PTR_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                } else {
                    putLexToken('-', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(ADD_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    putLexToken(INC_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('+', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '>':
                ch = getChar(cb);
                if (ch == '>') {
                    ch = getChar(cb);
                    if(ch=='>' && LANGUAGE(LANG_JAVA)) {
                        ch = getChar(cb);
                        if(ch=='=') {
                            putLexToken(URIGHT_ASSIGN, &dd);
                            PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                            ch = getChar(cb);
                            goto nextLexem;
                        } else {
                            putLexToken(URIGHT_OP, &dd);
                            PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        putLexToken(RIGHT_ASSIGN, &dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(RIGHT_OP, &dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(GE_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('>', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if(ch=='=') {
                        putLexToken(LEFT_ASSIGN, &dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(LEFT_OP, &dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(LE_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('<', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MUL_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('*', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MOD_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      putLexToken(YACC_PERC_LPAR, &dd);
                      PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      putLexToken(YACC_PERC_RPAR, &dd);
                      PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    putLexToken('%', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(AND_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    putLexToken(AND_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                else if (ch == '*') {
                    ch = getChar(cb);
                    if (ch == '/') {
                        /* a code comment, ignore */
                        ch = getChar(cb);
                        CommentEndReference(cb, 0);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '*';
                        putLexToken('&', &dd);
                        PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                        goto nextLexem;
                    }
                } else {
                    putLexToken('&', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(XOR_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('^', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(OR_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                } else if (ch == '|') {
                    putLexToken(OR_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('|', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(EQ_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('=', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(NE_OP, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                } else {putLexToken('!', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                putLexToken(':', &dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
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
                    putLexToken(CHAR_LITERAL, &dd);
                    putLexInt(chval, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                    ch = getChar(cb);
                }
                goto nextLexem;

            case '\"':
                line = cb->lineNumber;
                size = 0;
                putLexToken(STRING_LITERAL, &dd);
                do {
                    ch = getChar(cb);
                    size ++;
                    if (ch!='\"' && size<MAX_LEXEM_SIZE-10)
                        putLexChar(ch, &dd);
                    if (ch=='\\') {
                        ch = getChar(cb);
                        size ++;
                        if (size < MAX_LEXEM_SIZE-10)
                            putLexChar(ch, &dd);
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
                        if (options.strictAnsi && (options.debug || options.show_errors)) {
                            warningMessage(ERR_ST,"string constant through end of line");
                        }
                    }
                    // in Java CR LF can't be a part of string, even there
                    // are benchmarks making Xrefactory coredump if CR or LF
                    // is a part of strings
                } while (ch != '\"' && (ch != '\n' || !options.strictAnsi) && ch != -1);
                if (ch == -1 && options.taskRegime!=RegimeEditServer) {
                    warningMessage(ERR_ST,"string constant through EOF");
                }
                putLexChar(0, &dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                putLexLines(cb->lineNumber-line, &dd);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(DIV_ASSIGN, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='*') {
                    bool isJavadoc=false;
                    CommentBeginReference(cb);
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

                    int line = cb->lineNumber;
                    passComment(cb);
                    putLexLines(cb->lineNumber-line, &dd);
                    ch = getChar(cb);
                    CommentEndReference(cb, isJavadoc);
                    goto nextLexem;

                } else if (ch=='/' && options.cpp_comment) {
                    /*  ******* a // comment ******* */
                    CommentBeginReference(cb);
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        ch = getChar(cb);
                        CommentEndReference(cb, 0);
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
                    CommentEndReference(cb, 0);
                    putLexLines(cb->lineNumber-line, &dd);
                } else {
                    putLexToken('/', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    cb->lineNumber ++;
                    cb->lineBegin = cb->next;
                    cb->columnOffset = 0;
                    putLexLines(1, &dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('\\', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
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
                        CommentBeginReference(cb);
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
                            int line = cb->lineNumber;
                            passComment(cb);
                            putLexLines(cb->lineNumber-line, &dd);
                            ch = getChar(cb);
                            CommentEndReference(cb, javadoc);
                            ch = skipBlanks(cb, ch);
                        }
                    } else {
                        ungetChar(cb, ch);
                        ch = '/';
                    }
                }
                putLexToken('\n', &dd);
                PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
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
                            putLexToken(CPP_IFDEF, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"ifndef") == 0) {
                            putLexToken(CPP_IFNDEF, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"if") == 0) {
                            putLexToken(CPP_IF, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"elif") == 0) {
                            putLexToken(CPP_ELIF, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else if (strcmp(tt,"undef") == 0) {
                            putLexToken(CPP_UNDEF, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd); \
                        } else if (strcmp(tt,"else") == 0) {
                            putLexToken(CPP_ELSE, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd); \
                        } else if (strcmp(tt,"endif") == 0) {
                            putLexToken(CPP_ENDIF, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd); \
                        } else if (strcmp(tt,"include") == 0) {
                            char endCh;
                            putLexToken(CPP_INCLUDE, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                            ch = skipBlanks(cb, ch);
                            if (ch == '\"' || ch == '<') {
                                if (ch == '\"') endCh = '\"';
                                else endCh = '>';
                                scol = columnPosition(cb);
                                putLexToken(STRING_LITERAL, &dd);
                                do {
                                    putLexChar(ch, &dd);
                                    ch = getChar(cb);
                                } while (ch!=endCh && ch!='\n');
                                putLexChar(0, &dd);
                                PutLexPosition(cb->fileNumber,cb->lineNumber,scol,dd);
                                if (ch == endCh)
                                    ch = getChar(cb);
                            }
                        } else if (strcmp(tt,"define") == 0) {
                            ddd = dd;
                            putLexToken(CPP_DEFINE0, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                            ch = skipBlanks(cb, ch);
                            NOTE_NEW_LEXEM_POSITION(cb, lb);
                            ProcessIdentifier(ch, cb, dd,lab1);
                            if (ch == '(') {
                                putLexToken(CPP_DEFINE, &ddd);
                            }
                        } else if (strcmp(tt,"pragma") == 0) {
                            putLexToken(CPP_PRAGMA, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        } else {
                            putLexToken(CPP_LINE, &dd);
                            PutLexPosition(cb->fileNumber,cb->lineNumber,lcoll,dd);
                        }
                    }
                }
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    putLexToken(CPP_COLLATION, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                } else {
                    putLexToken('#', &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    putLexToken(ch, &dd);
                    PutLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, dd);
                }
                ch = getChar(cb);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (options.taskRegime == RegimeEditServer) {
            int pi,len,lastlex,parChar,apos;
            Position *ps;
            int pos1,currentLexemPosition;
            pi = (lb->index-1) % LEX_POSITIONS_RING_SIZE;
            ps = & lb->positionRing[pi];
            currentLexemPosition = lb->fileOffsetRing[pi];
            if (    cb->fileNumber == s_olOriginalFileNumber
                    && cb->fileNumber != noFileIndex
                    && cb->fileNumber != -1
                    && s_jsl==NULL
                    ) {
                if (options.server_operation == OLO_EXTRACT && lb->index>=2) {
                    ch = skipBlanks(cb, ch);
                    pos1 = absoluteFilePosition(cb);
                    log_trace(":pos1==%d, olCursorPos==%d, olMarkPos==%d",pos1,options.olCursorPos,options.olMarkPos);
                    // all this is very, very HACK!!!
                    if (pos1 >= options.olCursorPos && ! s_cps.marker1Flag) {
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (s_cps.marker2Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(';', &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(';', &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(OL_MARKER_TOKEN, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        s_cps.marker1Flag=1;
                    } else if (pos1 >= options.olMarkPos && ! s_cps.marker2Flag){
                        if (LANGUAGE(LANG_JAVA)) parChar = ';';
                        else {
                            if (s_cps.marker1Flag) parChar='}';
                            else parChar = '{';
                        }
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(';', &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(';', &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(OL_MARKER_TOKEN, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        putLexToken(parChar, &dd);
                        PutLexPosition(ps->file,ps->line,ps->col,dd);
                        s_cps.marker2Flag=1;
                    }
                } else if (options.server_operation == OLO_COMPLETION
                           ||  options.server_operation == OLO_SEARCH) {
                    ch = skipBlanks(cb, ch);
                    apos = absoluteFilePosition(cb);
                    if (currentLexemPosition < options.olCursorPos
                        && (apos >= options.olCursorPos
                            || (ch == -1 && apos+1 == options.olCursorPos))) {
                        log_trace("currentLexemPosition, s_opt.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d",currentLexemPosition, options.olCursorPos, apos, ch);
                        lastlex = nextLexToken(&lexStartDd);
                        if (lastlex == IDENTIFIER) {
                            len = options.olCursorPos-currentLexemPosition;
                            log_trace(":check %s[%d] <-> %d", lexStartDd+TOKEN_SIZE, len,strlen(lexStartDd+TOKEN_SIZE));
                            if (len <= strlen(lexStartDd+TOKEN_SIZE)) {
                                if (options.server_operation == OLO_SEARCH) {
                                    char *ddd;
                                    ddd = lexStartDd;
                                    putLexToken(IDENT_TO_COMPLETE, &ddd);
                                } else {
                                    dd = lexStartDd;
                                    putLexToken(IDENT_TO_COMPLETE, &dd);
                                    dd += len;
                                    putLexChar(0, &dd);
                                    PutLexPosition(ps->file,ps->line,ps->col,dd);
                                }
                                log_trace(":ress %s", lexStartDd+TOKEN_SIZE);
                            } else {
                                // completion after an identifier
                                PUT_EMPTY_COMPLETION_ID(cb, dd,
                                                        apos-options.olCursorPos);
                            }
                        } else if ((lastlex == LINE_TOK || lastlex == STRING_LITERAL)
                                   && (apos-options.olCursorPos != 0)) {
                            // completion inside special lexems, do
                            // NO COMPLETION
                        } else {
                            // completion after another lexem
                            PUT_EMPTY_COMPLETION_ID(cb, dd,
                                                    apos-options.olCursorPos);
                        }
                    }
                    // TODO, make this in a more standard way, !!!
                } else {
                    if (currentLexemPosition <= options.olCursorPos
                        && absoluteFilePosition(cb) >= options.olCursorPos) {
                        gotOnLineCxRefs(ps);
                        lastlex = nextLexToken(&lexStartDd);
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
                        if (apos >= options.olCursorPos && ! s_cps.marker1Flag) {
                            putLexToken(OL_MARKER_TOKEN, &dd);
                            PutLexPosition(ps->file,ps->line,ps->col,dd);
                            s_cps.marker1Flag=1;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    lb->end = dd;

    if (lb->end == lb->chars)
        return false;
    else
        return true;
}

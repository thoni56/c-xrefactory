#include "lexer.h"

#include "globals.h"
#include "options.h"
#include "commons.h"
#include "yylex.h"
#include "filedescriptor.h"
#include "filetable.h"

#include "caching.h"            /* cache && cacheInput() */
#include "jslsemact.h"          /* s_jsl */

#include "log.h"
#include "misc.h"              /* creatingOlcxRefs() */


void gotOnLineCxRefs(Position *position) {
    if (requiresCreatingRefs(options.serverOperation)) {
        cache.active = false;
        cxRefPosition = *position;
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
        cb->lineBegin = cb->nextUnread;
        cb->columnOffset = 0;
    }
    /* TODO test on cpp directive */
    do {
        oldCh = ch;
        ch = getChar(cb);
        if (ch=='\n') {
            cb->lineNumber ++;
            cb->lineBegin = cb->nextUnread;
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


static void processIdentifier(int *chP, CharacterBuffer *cb, char **destinationP) {
    int column;

    column = columnPosition(cb);
    putLexToken(IDENTIFIER, destinationP);
    do {
        putLexChar(*chP, destinationP);
        *chP = getChar(cb);
    } while (isalpha(*chP) || isdigit(*chP) || *chP == '_'
             || (*chP == '$' && (LANGUAGE(LANG_YACC) || LANGUAGE(LANG_JAVA))));
    putLexChar(0, destinationP);
    putLexPosition(cb->fileNumber, cb->lineNumber, column, destinationP);
}

static void noteNewLexemPosition(CharacterBuffer *cb, LexemBuffer *lb) {
    int index = lb->index % LEX_POSITIONS_RING_SIZE;
    lb->fileOffsetRing[index] = absoluteFilePosition(cb);
    lb->positionRing[index].file = cb->fileNumber;
    lb->positionRing[index].line = cb->lineNumber;
    lb->positionRing[index].col = columnPosition(cb);
    lb->index++;
}


static void put_empty_completion_id(CharacterBuffer *cb, char **destinationP, int len) {
    putLexToken(IDENT_TO_COMPLETE, destinationP);
    putLexChar(0, destinationP);
    putLexPosition(cb->fileNumber, cb->lineNumber,
                   columnPosition(cb) - len, destinationP);
}

bool getLexemFromLexer(LexemBuffer *lb) {
    int ch;
    CharacterBuffer *cb;
    char *lmax, *lexStartDd;
    char *dd;                   /* TODO: Destination, a.k.a where to put lexems? Maybe? */
    Lexem lexem;
    int line, column, size;
    int lexemStartingColumn, lexStartFilePos;

    /* first test whether the input is cached */
    /* TODO: why do we need to know this? */
    if (cache.active && includeStackPointer==0 && macroStackIndex==0) {
        cacheInput();
        cache.lexcc = lb->lexemStream;
    }

    lmax = lb->lexemStream + LEX_BUFF_SIZE - MAX_LEXEM_SIZE;
    dd = lb->lexemStream;
    for (char *cc = lb->next; cc < lb->end; cc++, dd++)
        *dd = *cc;
    lb->next = lb->lexemStream;

    cb = &lb->buffer;
    ch = getChar(cb);
    do {
        ch = skipBlanks(cb, ch);
        if (dd >= lmax) {
            ungetChar(cb, ch);
            break;
        }
        noteNewLexemPosition(cb, lb);
        /*  yytext = ccc; */
        lexStartDd = dd;
        lexemStartingColumn = columnPosition(cb);
        log_trace("lexStartCol = %d", lexemStartingColumn);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            // ProcessIdentifier(ch, cb, dd, lab2);
            processIdentifier(&ch, cb, &dd);
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
                lexem = floatingPointConstant(cb, &ch);
                putLexToken(lexem, &dd);
                putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                goto nextLexem;
            }
            /* integer */
            lexem = constantType(cb, &ch);
            putLexToken(lexem, &dd);
            putLexInt(val, &dd);
            putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
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
                        putLexToken(ELLIPSIS, &dd);
                        putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    putLexToken('.', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    lexem = floatingPointConstant(cb, &ch);
                    putLexToken(lexem, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                    goto nextLexem;
                } else {
                    putLexToken('.', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    putLexToken(SUB_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    putLexToken(DEC_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    putLexToken(PTR_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                } else {
                    putLexToken('-', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(ADD_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    putLexToken(INC_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('+', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
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
                            putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                            ch = getChar(cb);
                            goto nextLexem;
                        } else {
                            putLexToken(URIGHT_OP, &dd);
                            putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        putLexToken(RIGHT_ASSIGN, &dd);
                        putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(RIGHT_OP, &dd);
                        putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(GE_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('>', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if(ch=='=') {
                        putLexToken(LEFT_ASSIGN, &dd);
                        putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(LEFT_OP, &dd);
                        putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(LE_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('<', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MUL_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('*', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MOD_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      putLexToken(YACC_PERC_LPAR, &dd);
                      putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      putLexToken(YACC_PERC_RPAR, &dd);
                      putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    putLexToken('%', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(AND_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    putLexToken(AND_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                else if (ch == '*') {
                    ch = getChar(cb);
                    if (ch == '/') {
                        /* a code comment, ignore */
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '*';
                        putLexToken('&', &dd);
                        putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                        goto nextLexem;
                    }
                } else {
                    putLexToken('&', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(XOR_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('^', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(OR_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else if (ch == '|') {
                    putLexToken(OR_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('|', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(EQ_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('=', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(NE_OP, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else {putLexToken('!', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                putLexToken(':', &dd);
                putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                goto nextLexem;

            case '\'': {
                unsigned chval = 0;

                lexStartFilePos = absoluteFilePosition(cb);
                do {
                    ch = getChar(cb);
                    while (ch == '\\') {
                        ch = getChar(cb);
                        /* TODO escape sequences */
                        ch = getChar(cb);
                    }
                    if (ch != '\'')
                        chval = chval * 256 + ch;
                } while (ch != '\'' && ch != '\n');
                if (ch == '\'') {
                    putLexToken(CHAR_LITERAL, &dd);
                    putLexInt(chval, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    putLexInt(absoluteFilePosition(cb) - lexStartFilePos, &dd);
                    ch = getChar(cb);
                }
                goto nextLexem;
            }

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
                            cb->lineBegin = cb->nextUnread;
                            cb->columnOffset = 0;
                        }
                        ch = 0;
                    }
                    if (ch == '\n') {
                        cb->lineNumber ++;
                        cb->lineBegin = cb->nextUnread;
                        cb->columnOffset = 0;
                        if (options.strictAnsi && (options.debug || options.errors)) {
                            warningMessage(ERR_ST,"string constant through end of line");
                        }
                    }
                    // in Java CR LF can't be a part of string, even there
                    // are benchmarks making Xrefactory coredump if CR or LF
                    // is a part of strings
                } while (ch != '\"' && (ch != '\n' || !options.strictAnsi) && ch != -1);
                if (ch == -1 && options.mode!=ServerMode) {
                    warningMessage(ERR_ST,"string constant through EOF");
                }
                putLexChar(0, &dd);
                putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                putLexLines(cb->lineNumber-line, &dd);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(DIV_ASSIGN, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='*') {
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* a program comment, ignore and continue with next lexem */
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '*';
                    }   /* !!! COPY BLOCK TO '/n' */

                    int line = cb->lineNumber;
                    passComment(cb);
                    putLexLines(cb->lineNumber-line, &dd);
                    ch = getChar(cb);
                    goto nextLexem;

                } else if (ch=='/') {
                    /*  ******* a // comment ******* */
                    ch = getChar(cb);
                    if (ch == '&') {
                        /* ****** a program comment, ignore */
                        ch = getChar(cb);
                        goto nextLexem;
                    }
                    line = cb->lineNumber;
                    while (ch!='\n' && ch != -1) {
                        ch = getChar(cb);
                        if (ch == '\\') {
                            ch = getChar(cb);
                            if (ch=='\n') {
                                cb->lineNumber ++;
                                cb->lineBegin = cb->nextUnread;
                                cb->columnOffset = 0;
                            }
                            ch = getChar(cb);
                        }
                    }
                    putLexLines(cb->lineNumber-line, &dd);
                } else {
                    putLexToken('/', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    cb->lineNumber ++;
                    cb->lineBegin = cb->nextUnread;
                    cb->columnOffset = 0;
                    putLexLines(1, &dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('\\', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '\n':
                column = columnPosition(cb);
                if (column >= MAX_REFERENCABLE_COLUMN) {
                    FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                if (cb->lineNumber >= MAX_REFERENCABLE_LINE) {
                    FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                cb->lineNumber ++;
                cb->lineBegin = cb->nextUnread;
                cb->columnOffset = 0;
                ch = getChar(cb);
                ch = skipBlanks(cb, ch);
                if (ch == '/') {
                    ch = getChar(cb);
                    if (ch == '*') {
                        ch = getChar(cb);
                        if (ch == '&') {
                            /* ****** a code comment, ignore */
                            ch = getChar(cb);
                        } else {
                            ungetChar(cb, ch);
                            ch = '*';
                            int line = cb->lineNumber;
                            passComment(cb);
                            putLexLines(cb->lineNumber-line, &dd);
                            ch = getChar(cb);
                            ch = skipBlanks(cb, ch);
                        }
                    } else {
                        ungetChar(cb, ch);
                        ch = '/';
                    }
                }
                putLexToken('\n', &dd);
                putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                if (ch == '#' && LANGUAGE(LANG_C|LANG_YACC)) {
                    noteNewLexemPosition(cb, lb);
                    //&        HandleCppToken(ch, cb, dd, cb->nextUnread, cb->fileNumber, cb->lineNumber, cb->lineBegin);
                    // #define HandleCppToken(ch, cb, dd, cb->next, cb->fileNumber, cb->lineNumber, cb->lineBegin) {
                    {
                        char *ddd, tt[30];
                        int i, lcoll, scol;
                        lcoll = columnPosition(cb);
                        ch = getChar(cb);
                        ch = skipBlanks(cb, ch);
                        for(i=0; i<sizeof(tt)-1 && (isalpha(ch) || isdigit(ch) || ch=='_') ; i++) {
                            tt[i] = ch;
                            ch = getChar(cb);
                        }
                        tt[i]=0;
                        if (strcmp(tt,"ifdef") == 0) {
                            putLexToken(CPP_IFDEF, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                        } else if (strcmp(tt,"ifndef") == 0) {
                            putLexToken(CPP_IFNDEF, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                        } else if (strcmp(tt,"if") == 0) {
                            putLexToken(CPP_IF, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                        } else if (strcmp(tt,"elif") == 0) {
                            putLexToken(CPP_ELIF, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                        } else if (strcmp(tt,"undef") == 0) {
                            putLexToken(CPP_UNDEF, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd); \
                        } else if (strcmp(tt,"else") == 0) {
                            putLexToken(CPP_ELSE, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd); \
                        } else if (strcmp(tt,"endif") == 0) {
                            putLexToken(CPP_ENDIF, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd); \
                        } else if (strcmp(tt,"include") == 0 || strcmp(tt, "include_next") == 0) {
                            char endCh;
                            if (strcmp(tt, "include") == 0)
                                putLexToken(CPP_INCLUDE, &dd);
                            else
                                putLexToken(CPP_INCLUDE_NEXT, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
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
                                putLexPosition(cb->fileNumber,cb->lineNumber,scol, &dd);
                                if (ch == endCh)
                                    ch = getChar(cb);
                            }
                        } else if (strcmp(tt,"define") == 0) {
                            ddd = dd;
                            putLexToken(CPP_DEFINE0, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                            ch = skipBlanks(cb, ch);
                            noteNewLexemPosition(cb, lb);
                            processIdentifier(&ch, cb, &dd);
                            if (ch == '(') {
                                putLexToken(CPP_DEFINE, &ddd);
                            }
                        } else if (strcmp(tt,"pragma") == 0) {
                            putLexToken(CPP_PRAGMA, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                        } else {
                            putLexToken(CPP_LINE, &dd);
                            putLexPosition(cb->fileNumber,cb->lineNumber,lcoll, &dd);
                        }
                    }
                }
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    putLexToken(CPP_COLLATION, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                } else {
                    putLexToken('#', &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    putLexToken(ch, &dd);
                    putLexPosition(cb->fileNumber, cb->lineNumber, lexemStartingColumn, &dd);
                }
                ch = getChar(cb);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (options.mode == ServerMode) {
            int pi,len,lastlex,parChar,apos;
            Position *position;
            int pos1,currentLexemPosition;
            pi = (lb->index-1) % LEX_POSITIONS_RING_SIZE;
            position = &lb->positionRing[pi];
            currentLexemPosition = lb->fileOffsetRing[pi];
            if (cb->fileNumber == olOriginalFileIndex && cb->fileNumber != noFileIndex
                && cb->fileNumber != -1 && s_jsl == NULL) {
                if (options.serverOperation == OLO_EXTRACT && lb->index>=2) {
                    ch = skipBlanks(cb, ch);
                    pos1 = absoluteFilePosition(cb);
                    log_trace(":pos1==%d, olCursorPos==%d, olMarkPos==%d",pos1,options.olCursorPos,options.olMarkPos);
                    // all this is very, very HACK!!!
                    if (pos1 >= options.olCursorPos && !parsedInfo.marker1Flag) {
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (parsedInfo.marker2Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(';', &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(';', &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(OL_MARKER_TOKEN, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        parsedInfo.marker1Flag = true;
                    } else if (pos1 >= options.olMarkPos && !parsedInfo.marker2Flag){
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (parsedInfo.marker1Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(';', &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(';', &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(OL_MARKER_TOKEN, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        putLexToken(parChar, &dd);
                        putLexPosition(position->file,position->line,position->col, &dd);
                        parsedInfo.marker2Flag = true;
                    }
                } else if (options.serverOperation == OLO_COMPLETION
                           ||  options.serverOperation == OLO_SEARCH) {
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
                                if (options.serverOperation == OLO_SEARCH) {
                                    char *ddd;
                                    ddd = lexStartDd;
                                    putLexToken(IDENT_TO_COMPLETE, &ddd);
                                } else {
                                    dd = lexStartDd;
                                    putLexToken(IDENT_TO_COMPLETE, &dd);
                                    dd += len;
                                    putLexChar(0, &dd);
                                    putLexPosition(position->file,position->line,position->col, &dd);
                                }
                                log_trace(":ress %s", lexStartDd+TOKEN_SIZE);
                            } else {
                                // completion after an identifier
                                put_empty_completion_id(cb, &dd,
                                                        apos-options.olCursorPos);
                            }
                        } else if ((lastlex == LINE_TOKEN || lastlex == STRING_LITERAL)
                                   && (apos-options.olCursorPos != 0)) {
                            // completion inside special lexems, do
                            // NO COMPLETION
                        } else {
                            // completion after another lexem
                            put_empty_completion_id(cb, &dd,
                                                    apos-options.olCursorPos);
                        }
                    }
                    // TODO, make this in a more standard way, !!!
                } else {
                    if (currentLexemPosition <= options.olCursorPos
                        && absoluteFilePosition(cb) >= options.olCursorPos) {
                        gotOnLineCxRefs(position);
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
                        if (apos >= options.olCursorPos && !parsedInfo.marker1Flag) {
                            putLexToken(OL_MARKER_TOKEN, &dd);
                            putLexPosition(position->file,position->line,position->col, &dd);
                            parsedInfo.marker1Flag = true;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    lb->end = dd;

    if (lb->end == lb->lexemStream)
        return false;
    else
        return true;
}

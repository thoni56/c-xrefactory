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


static void processIdentifier(int *chP, char **destinationP, LexemBuffer *lb) {
    int column;

    column = columnPosition(&lb->buffer);
    putLexToken(IDENTIFIER, destinationP);
    do {
        putLexChar(*chP, destinationP);
        *chP = getChar(&lb->buffer);
    } while (isalpha(*chP) || isdigit(*chP) || *chP == '_'
             || (*chP == '$' && (LANGUAGE(LANG_YACC) || LANGUAGE(LANG_JAVA))));
    putLexChar(0, destinationP);
    putLexPosition(lb->buffer.fileNumber, lb->buffer.lineNumber, column, destinationP);
}

static void noteNewLexemPosition(LexemBuffer *lb) {
    int index = lb->index % LEX_POSITIONS_RING_SIZE;
    lb->fileOffsetRing[index] = absoluteFilePosition(&lb->buffer);
    lb->positionRing[index].file = lb->buffer.fileNumber;
    lb->positionRing[index].line = lb->buffer.lineNumber;
    lb->positionRing[index].col = columnPosition(&lb->buffer);
    lb->index++;
}


static void putEmptyCompletionId(LexemBuffer *lb, char **destinationP, int len) {
    putLexToken(IDENT_TO_COMPLETE, destinationP);
    putLexChar(0, destinationP);
    putLexPosition(lb->buffer.fileNumber, lb->buffer.lineNumber,
                   columnPosition(&lb->buffer) - len, destinationP);
}

protected void shiftRemainingLexems(LexemBuffer *lb) {
    int remaining = lb->end - lb->next;
    char *src = lb->next;
    char *dest = lb->lexemStream;

    for (int i = 0; i < remaining; i++)
        *dest++ = *src++;

    lb->next = lb->lexemStream;
    lb->end = &lb->lexemStream[remaining];
}

bool getLexemFromLexer(LexemBuffer *lb) {
    int ch;
    CharacterBuffer *cb;
    char *lexemLimit, *lexStartDd;
    char *dd;                   /* TODO: It works to replace this with #define dd (lb->end) */
    Lexem lexem;
    int line, column, size;
    int lexemStartingColumn, lexStartFilePos;

    /* first test whether the input is cached */
    /* TODO: why do we need to know this? */
    if (cache.active && includeStackPointer==0 && macroStackIndex==0) {
        cacheInput();
        cache.lexcc = lb->lexemStream;
    }

    shiftRemainingLexems(lb);

    lexemLimit = lb->end + LEXEM_BUFFER_SIZE - MAX_LEXEM_SIZE;

    dd = lb->end;
    cb = &lb->buffer;
    ch = getChar(cb);
    do {
        ch = skipBlanks(cb, ch);
        /* Space for more lexems? */
        if (dd >= lexemLimit) {
            ungetChar(cb, ch);
            break;
        }
        noteNewLexemPosition(lb);
        lexStartDd = dd;
        lexemStartingColumn = columnPosition(cb);
        log_trace("lexStartCol = %d", lexemStartingColumn);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            processIdentifier(&ch, &dd, lb);
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
                putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                goto nextLexem;
            }
            /* integer */
            lexem = constantType(cb, &ch);
            putLexToken(lexem, &dd);
            putLexInt(val, &dd);
            putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
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
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    putLexToken('.', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    lexem = floatingPointConstant(cb, &ch);
                    putLexToken(lexem, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &dd);
                    goto nextLexem;
                } else {
                    putLexToken('.', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    putLexToken(SUB_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    putLexToken(DEC_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    putLexToken(PTR_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                } else {
                    putLexToken('-', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(ADD_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    putLexToken(INC_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('+', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
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
                            putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                            ch = getChar(cb);
                            goto nextLexem;
                        } else {
                            putLexToken(URIGHT_OP, &dd);
                            putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        putLexToken(RIGHT_ASSIGN, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(RIGHT_OP, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(GE_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('>', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if(ch=='=') {
                        putLexToken(LEFT_ASSIGN, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(LEFT_OP, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(LE_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('<', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MUL_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('*', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MOD_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      putLexToken(YACC_PERC_LPAR, &dd);
                      putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      putLexToken(YACC_PERC_RPAR, &dd);
                      putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    putLexToken('%', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(AND_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    putLexToken(AND_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
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
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                        goto nextLexem;
                    }
                } else {
                    putLexToken('&', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(XOR_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('^', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(OR_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else if (ch == '|') {
                    putLexToken(OR_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('|', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(EQ_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else {
                    putLexToken('=', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(NE_OP, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    ch = getChar(cb);
                } else {putLexToken('!', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                putLexToken(':', &dd);
                putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
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
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                    putLexInt(absoluteFilePosition(cb) - lexStartFilePos, &dd);
                    ch = getChar(cb);
                }
                goto nextLexem;
            }

            case '\"':
                line = lineNumberFrom(lb);
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
                        if (ch == '\n') {
                            lb->buffer.lineNumber++;
                            cb->lineBegin = cb->nextUnread;
                            cb->columnOffset = 0;
                        }
                        ch = 0;
                    }
                    if (ch == '\n') {
                        lb->buffer.lineNumber++;
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
                putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                putLexLines(lineNumberFrom(lb)-line, &dd, lb);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(DIV_ASSIGN, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
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

                    int line = lineNumberFrom(lb);
                    passComment(cb);
                    putLexLines(lineNumberFrom(lb)-line, &dd, lb);
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
                    line = lineNumberFrom(lb);
                    while (ch!='\n' && ch != -1) {
                        ch = getChar(cb);
                        if (ch == '\\') {
                            ch = getChar(cb);
                            if (ch=='\n') {
                                lb->buffer.lineNumber++;
                                cb->lineBegin = cb->nextUnread;
                                cb->columnOffset = 0;
                            }
                            ch = getChar(cb);
                        }
                    }
                    putLexLines(lineNumberFrom(lb)-line, &dd, lb);
                } else {
                    putLexToken('/', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    lb->buffer.lineNumber++;
                    cb->lineBegin = cb->nextUnread;
                    cb->columnOffset = 0;
                    putLexLines(1, &dd, lb);
                    ch = getChar(cb);
                } else {
                    putLexToken('\\', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case '\n':
                column = columnPosition(cb);
                if (column >= MAX_REFERENCABLE_COLUMN) {
                    FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                if (lineNumberFrom(lb) >= MAX_REFERENCABLE_LINE) {
                    FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file", XREF_EXIT_ERR);
                }
                lb->buffer.lineNumber++;
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
                            int line = lineNumberFrom(lb);
                            passComment(cb);
                            putLexLines(lineNumberFrom(lb)-line, &dd, lb);
                            ch = getChar(cb);
                            ch = skipBlanks(cb, ch);
                        }
                    } else {
                        ungetChar(cb, ch);
                        ch = '/';
                    }
                }
                putLexToken('\n', &dd);
                putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                if (ch == '#' && LANGUAGE(LANG_C | LANG_YACC)) {
                    char preprocessorWord[30];
                    int  i, column, scol;
                    noteNewLexemPosition(lb);
                    column = columnPosition(cb);
                    ch     = getChar(cb);
                    ch     = skipBlanks(cb, ch);
                    for (i = 0; i < sizeof(preprocessorWord) - 1 && (isalpha(ch) || isdigit(ch) || ch == '_');
                         i++) {
                        preprocessorWord[i] = ch;
                        ch                  = getChar(cb);
                    }
                    preprocessorWord[i] = 0;
                    if (strcmp(preprocessorWord, "ifdef") == 0) {
                        putLexToken(CPP_IFDEF, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "ifndef") == 0) {
                        putLexToken(CPP_IFNDEF, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "if") == 0) {
                        putLexToken(CPP_IF, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "elif") == 0) {
                        putLexToken(CPP_ELIF, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "undef") == 0) {
                        putLexToken(CPP_UNDEF, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "else") == 0) {
                        putLexToken(CPP_ELSE, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "endif") == 0) {
                        putLexToken(CPP_ENDIF, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else if (strcmp(preprocessorWord, "include") == 0
                               || strcmp(preprocessorWord, "include_next") == 0) {
                        char endCh;
                        if (strcmp(preprocessorWord, "include") == 0)
                            putLexToken(CPP_INCLUDE, &dd);
                        else
                            putLexToken(CPP_INCLUDE_NEXT, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                        ch = skipBlanks(cb, ch);
                        if (ch == '\"' || ch == '<') {
                            if (ch == '\"')
                                endCh = '\"';
                            else
                                endCh = '>';
                            scol = columnPosition(cb);
                            putLexToken(STRING_LITERAL, &dd);
                            do {
                                putLexChar(ch, &dd);
                                ch = getChar(cb);
                            } while (ch != endCh && ch != '\n');
                            putLexChar(0, &dd);
                            putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), scol, &dd);
                            if (ch == endCh)
                                ch = getChar(cb);
                        }
                    } else if (strcmp(preprocessorWord, "define") == 0) {
                        char *savedDd = dd;
                        putLexToken(CPP_DEFINE0, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                        ch = skipBlanks(cb, ch);
                        noteNewLexemPosition(lb);
                        processIdentifier(&ch, &dd, lb);
                        if (ch == '(') {
                            putLexToken(CPP_DEFINE, &savedDd);
                        }
                    } else if (strcmp(preprocessorWord, "pragma") == 0) {
                        putLexToken(CPP_PRAGMA, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    } else {
                        putLexToken(CPP_LINE, &dd);
                        putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), column, &dd);
                    }
                }
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    putLexToken(CPP_COLLATION, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                } else {
                    putLexToken('#', &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    putLexToken(ch, &dd);
                    putLexPosition(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &dd);
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
            if (fileNumberFrom(lb) == olOriginalFileIndex && fileNumberFrom(lb) != noFileIndex
                && fileNumberFrom(lb) != -1 && s_jsl == NULL) {
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
                        log_trace("currentLexemPosition, options.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d",currentLexemPosition, options.olCursorPos, apos, ch);
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
                                putEmptyCompletionId(lb, &dd,
                                                        apos-options.olCursorPos);
                            }
                        } else if ((lastlex == LINE_TOKEN || lastlex == STRING_LITERAL)
                                   && (apos-options.olCursorPos != 0)) {
                            // completion inside special lexems, do
                            // NO COMPLETION
                        } else {
                            // completion after another lexem
                            putEmptyCompletionId(lb, &dd,
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

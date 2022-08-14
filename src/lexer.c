#include "lexer.h"

#include "globals.h"
#include "lexembuffer.h"
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


static void processIdentifier(int *chP, LexemBuffer *lb) {
    int column;

    column = columnPosition(&lb->buffer);
    putLexToken(IDENTIFIER, &(lb->end));
    do {
        putLexChar(lb, *chP);
        *chP = getChar(&lb->buffer);
    } while (isalpha(*chP) || isdigit(*chP) || *chP == '_'
             || (*chP == '$' && (LANGUAGE(LANG_YACC) || LANGUAGE(LANG_JAVA))));
    putLexChar(lb, 0);
    putLexPositionFields(lb->buffer.fileNumber, lb->buffer.lineNumber, column, &(lb->end));
}

static void noteNewLexemPosition(LexemBuffer *lb) {
    int index = lb->ringIndex % LEX_POSITIONS_RING_SIZE;
    lb->fileOffsetRing[index] = absoluteFilePosition(&lb->buffer);
    lb->positionRing[index].file = lb->buffer.fileNumber;
    lb->positionRing[index].line = lb->buffer.lineNumber;
    lb->positionRing[index].col = columnPosition(&lb->buffer);
    lb->ringIndex++;
}


static void putEmptyCompletionId(LexemBuffer *lb, int len) {
    putLexToken(IDENT_TO_COMPLETE, &(lb->end));
    putLexChar(lb, 0);
    putLexPositionFields(lb->buffer.fileNumber, lb->buffer.lineNumber,
                         columnPosition(&lb->buffer) - len, &(lb->end));
}

protected void shiftAnyRemainingLexems(LexemBuffer *lb) {
    int remaining = lb->end - lb->next;
    char *src = lb->next;
    char *dest = lb->lexemStream;

    for (int i = 0; i < remaining; i++)
        *dest++ = *src++;

    lb->next = lb->lexemStream;
    lb->end = &lb->lexemStream[remaining];
}

static int handleCppToken(LexemBuffer *lb) {
    int   ch;
    char  preprocessorWord[30];
    int   i, column;

    noteNewLexemPosition(lb);
    column = columnPosition(&lb->buffer);
    ch     = getChar(&lb->buffer);
    ch     = skipBlanks(&lb->buffer, ch);
    for (i = 0; i < sizeof(preprocessorWord) - 1 && (isalpha(ch) || isdigit(ch) || ch == '_'); i++) {
        preprocessorWord[i] = ch;
        ch                  = getChar(&lb->buffer);
    }
    preprocessorWord[i] = 0;
    if (strcmp(preprocessorWord, "ifdef") == 0) {
        putLexToken(CPP_IFDEF, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "ifndef") == 0) {
        putLexToken(CPP_IFNDEF, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "if") == 0) {
        putLexToken(CPP_IF, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "elif") == 0) {
        putLexToken(CPP_ELIF, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "undef") == 0) {
        putLexToken(CPP_UNDEF, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "else") == 0) {
        putLexToken(CPP_ELSE, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "endif") == 0) {
        putLexToken(CPP_ENDIF, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else if (strcmp(preprocessorWord, "include") == 0 || strcmp(preprocessorWord, "include_next") == 0) {
        char endCh;
        if (strcmp(preprocessorWord, "include") == 0)
            putLexToken(CPP_INCLUDE, &(lb->end));
        else
            putLexToken(CPP_INCLUDE_NEXT, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
        ch = skipBlanks(&lb->buffer, ch);
        if (ch == '\"' || ch == '<') {
            int scol;
            if (ch == '\"')
                endCh = '\"';
            else
                endCh = '>';
            scol = columnPosition(&lb->buffer);
            putLexToken(STRING_LITERAL, &(lb->end));
            do {
                putLexChar(lb, ch);
                ch = getChar(&lb->buffer);
            } while (ch != endCh && ch != '\n');
            putLexChar(lb, 0);
            putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), scol, &(lb->end));
            if (ch == endCh)
                ch = getChar(&lb->buffer);
        }
    } else if (strcmp(preprocessorWord, "define") == 0) {
        char *backpatchLexemP = *&(lb->end);
        putLexToken(CPP_DEFINE0, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
        ch = skipBlanks(&lb->buffer, ch);
        noteNewLexemPosition(lb);
        processIdentifier(&ch, lb);
        if (ch == '(') {
            /* Backpatch the current lexem code (CPP_DEFINE0) with discovered CPP_DEFINE */
            putLexToken(CPP_DEFINE, &backpatchLexemP);
        }
    } else if (strcmp(preprocessorWord, "pragma") == 0) {
        putLexToken(CPP_PRAGMA, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    } else {
        putLexToken(CPP_LINE, &(lb->end));
        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), column, &(lb->end));
    }

    return ch;
}

static void handleCompletionOrSearch(LexemBuffer *lb, char *startOfCurrentLexem, Position position,
                                     int currentLexemPosition, int *chP) {
    *chP     = skipBlanks(&lb->buffer, *chP);
    int apos = absoluteFilePosition(&lb->buffer);

    if (currentLexemPosition < options.olCursorPos
        && (apos >= options.olCursorPos || (*chP == -1 && apos + 1 == options.olCursorPos))) {
        log_trace("currentLexemPosition, options.olCursorPos, ABS_FILE_POS, ch == %d, %d, %d, %d",
                  currentLexemPosition, options.olCursorPos, apos, *chP);
        Lexem thisLexToken = nextLexToken(&startOfCurrentLexem);
        if (thisLexToken == IDENTIFIER) {
            int len = options.olCursorPos - currentLexemPosition;
            log_trace(":check %s[%d] <-> %d", startOfCurrentLexem + TOKEN_SIZE, len,
                      strlen(startOfCurrentLexem + TOKEN_SIZE));
            if (len <= strlen(startOfCurrentLexem + TOKEN_SIZE)) {
                /* Need to backpatch the current lexem to a COMPLETE lexem */
                char *backpatchP = startOfCurrentLexem;
                /* We want to overwrite with an IDENT_TO_COMPLETE,
                 * which we don't want to know how many bytes it
                 * takes, so we use ...WithPointer() which advances... */
                putLexTokenWithPointer(IDENT_TO_COMPLETE, &backpatchP);
                if (options.serverOperation == OLO_COMPLETION) {
                    /* And for completion we need to terminate the identifier where the cursor is */
                    /* Move to position cursor is on in already written identifier */
                    /* We can use the backpatchP since it has moved to begining of string */
                    setLexemStreamEnd(lb, backpatchP + len);
                    /* Terminate identifier here */
                    putLexChar(lb, 0);
                    /* And write the position */
                    putLexPosition(lb, position);
                }
                log_trace(":ress %s", startOfCurrentLexem + TOKEN_SIZE);
            } else {
                // completion after an identifier
                putEmptyCompletionId(lb, apos - options.olCursorPos);
            }
        } else if ((thisLexToken == LINE_TOKEN || thisLexToken == STRING_LITERAL)
                   && (apos != options.olCursorPos)) {
            // completion inside special lexems, do
            // NO COMPLETION
        } else {
            // completion after another lexem
            putEmptyCompletionId(lb, apos - options.olCursorPos);
        }
    }
}

bool getLexemFromLexer(LexemBuffer *lb) {
    int ch;
    char *lexemLimit, *startOfCurrentLexem;
    Lexem lexem;
    int line, column, size;
    int lexemStartingColumn, lexStartFilePos;
    CharacterBuffer *cb = &lb->buffer;

    /* first test whether the input is cached */
    /* TODO: why do we need to know this? */
    if (cache.active && includeStackPointer==0 && macroStackIndex==0) {
        cacheInput();
        cache.lexcc = lb->lexemStream;
    }

    shiftAnyRemainingLexems(lb);

    lexemLimit = lb->end + LEXEM_BUFFER_SIZE - MAX_LEXEM_SIZE;

    ch = getChar(cb);
    do {
        ch = skipBlanks(cb, ch);
        /* Space for more lexems? */
        if (lb->end >= lexemLimit) {
            ungetChar(cb, ch);
            break;
        }
        noteNewLexemPosition(lb);
        startOfCurrentLexem = lb->end;
        lexemStartingColumn = columnPosition(cb);
        log_trace("lexStartCol = %d", lexemStartingColumn);
        if (ch == '_' || isalpha(ch) || (ch=='$' && (LANGUAGE(LANG_YACC)||LANGUAGE(LANG_JAVA)))) {
            processIdentifier(&ch, lb);
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
                putLexToken(lexem, &(lb->end));
                putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &(lb->end));
                goto nextLexem;
            }
            /* integer */
            lexem = constantType(cb, &ch);
            putLexToken(lexem, &(lb->end));
            putLexInt(val, &(lb->end));
            putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
            putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &(lb->end));
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
                        putLexToken(ELLIPSIS, &(lb->end));
                        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    putLexToken('.', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    lexem = floatingPointConstant(cb, &ch);
                    putLexToken(lexem, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    putLexInt(absoluteFilePosition(cb)-lexStartFilePos, &(lb->end));
                    goto nextLexem;
                } else {
                    putLexToken('.', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    putLexToken(SUB_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    putLexToken(DEC_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    putLexToken(PTR_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                } else {
                    putLexToken('-', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(ADD_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    putLexToken(INC_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('+', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '>':
                ch = getChar(cb);
                if (ch == '>') {
                    ch = getChar(cb);
                    if(ch=='>' && LANGUAGE(LANG_JAVA)) {
                        ch = getChar(cb);
                        if(ch=='=') {
                            putLexToken(URIGHT_ASSIGN, &(lb->end));
                            putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                            ch = getChar(cb);
                            goto nextLexem;
                        } else {
                            putLexToken(URIGHT_OP, &(lb->end));
                            putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                            goto nextLexem;
                        }
                    } else if(ch=='=') {
                        putLexToken(RIGHT_ASSIGN, &(lb->end));
                        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(RIGHT_OP, &(lb->end));
                        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(GE_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('>', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if(ch=='=') {
                        putLexToken(LEFT_ASSIGN, &(lb->end));
                        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexToken(LEFT_OP, &(lb->end));
                        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexToken(LE_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('<', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MUL_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('*', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(MOD_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      putLexToken(YACC_PERC_LPAR, &(lb->end));
                      putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      putLexToken(YACC_PERC_RPAR, &(lb->end));
                      putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    putLexToken('%', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(AND_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    putLexToken(AND_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
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
                        putLexToken('&', &(lb->end));
                        putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                        goto nextLexem;
                    }
                } else {
                    putLexToken('&', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(XOR_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexToken('^', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(OR_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                } else if (ch == '|') {
                    putLexToken(OR_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                } else {
                    putLexToken('|', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(EQ_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                } else {
                    putLexToken('=', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(NE_OP, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    ch = getChar(cb);
                } else {putLexToken('!', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                putLexToken(':', &(lb->end));
                putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
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
                    putLexToken(CHAR_LITERAL, &(lb->end));
                    putLexInt(chval, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                    putLexInt(absoluteFilePosition(cb) - lexStartFilePos, &(lb->end));
                    ch = getChar(cb);
                }
                goto nextLexem;
            }

            case '\"':
                line = lineNumberFrom(lb);
                size = 0;
                putLexToken(STRING_LITERAL, &(lb->end));
                do {
                    ch = getChar(cb);
                    size ++;
                    if (ch!='\"' && size<MAX_LEXEM_SIZE-10)
                        putLexChar(lb, ch);
                    if (ch=='\\') {
                        ch = getChar(cb);
                        size ++;
                        if (size < MAX_LEXEM_SIZE-10)
                            putLexChar(lb, ch);
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
                putLexChar(lb, 0);
                putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                putLexLines(lb, lineNumberFrom(lb)-line);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexToken(DIV_ASSIGN, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
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
                    putLexLines(lb, lineNumberFrom(lb)-line);
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
                    putLexLines(lb, lineNumberFrom(lb)-line);
                } else {
                    putLexToken('/', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    lb->buffer.lineNumber++;
                    cb->lineBegin = cb->nextUnread;
                    cb->columnOffset = 0;
                    putLexLines(lb, 1);
                    ch = getChar(cb);
                } else {
                    putLexToken('\\', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
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
                            putLexLines(lb, lineNumberFrom(lb)-line);
                            ch = getChar(cb);
                            ch = skipBlanks(cb, ch);
                        }
                    } else {
                        ungetChar(cb, ch);
                        ch = '/';
                    }
                }
                putLexToken('\n', &(lb->end));
                putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                if (ch == '#' && LANGUAGE(LANG_C | LANG_YACC)) {
                    ch = handleCppToken(lb);
                }
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    putLexToken(CPP_COLLATION, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                } else {
                    putLexToken('#', &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    putLexToken(ch, &(lb->end));
                    putLexPositionFields(fileNumberFrom(lb), lineNumberFrom(lb), lexemStartingColumn, &(lb->end));
                }
                ch = getChar(cb);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (options.mode == ServerMode) {
            int pi, parChar;
            Position position;
            int currentLexemPosition;

            /* Since lb->index is incremented *after* adding, we need to subtract 1 to get current */
            pi = (lb->ringIndex-1) % LEX_POSITIONS_RING_SIZE;
            currentLexemPosition = lb->fileOffsetRing[pi];
            position = lb->positionRing[pi];

            if (fileNumberFrom(lb) == olOriginalFileIndex && fileNumberFrom(lb) != noFileIndex
                && fileNumberFrom(lb) != -1 && s_jsl == NULL) {
                if (options.serverOperation == OLO_EXTRACT && lb->ringIndex>=2) { /* TODO: WTF does "lb->index >= 2" mean? */
                    ch = skipBlanks(cb, ch);
                    int apos = absoluteFilePosition(cb);
                    log_trace(":pos1==%d, olCursorPos==%d, olMarkPos==%d",apos,options.olCursorPos,options.olMarkPos);
                    // all this is very, very HACK!!!
                    if (apos >= options.olCursorPos && !parsedInfo.marker1Flag) {
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (parsedInfo.marker2Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(';', &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(';', &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(OL_MARKER_TOKEN, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        parsedInfo.marker1Flag = true;
                    } else if (apos >= options.olMarkPos && !parsedInfo.marker2Flag){
                        if (LANGUAGE(LANG_JAVA))
                            parChar = ';';
                        else {
                            if (parsedInfo.marker1Flag)
                                parChar='}';
                            else
                                parChar = '{';
                        }
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(';', &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(';', &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(OL_MARKER_TOKEN, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        putLexToken(parChar, &(lb->end));
                        putLexPosition(lb, position);
                        parsedInfo.marker2Flag = true;
                    }
                } else if (options.serverOperation == OLO_COMPLETION
                           ||  options.serverOperation == OLO_SEARCH) {
                    handleCompletionOrSearch(lb, startOfCurrentLexem, position, currentLexemPosition, &ch);
                } else {
                    if (currentLexemPosition <= options.olCursorPos
                        && absoluteFilePosition(cb) >= options.olCursorPos) {
                        gotOnLineCxRefs(&position);
                        Lexem lastlex = nextLexToken(&startOfCurrentLexem);
                        if (lastlex == IDENTIFIER) {
                            strcpy(s_olstring, startOfCurrentLexem+TOKEN_SIZE);
                        }
                    }
                    if (LANGUAGE(LANG_JAVA)) {
                        // there is a problem with this, when browsing at CPP construction
                        // that is why I restrict it to Java language! It is usefull
                        // only for Java refactorings
                        ch = skipBlanks(cb, ch);
                        int apos = absoluteFilePosition(cb);
                        if (apos >= options.olCursorPos && !parsedInfo.marker1Flag) {
                            putLexToken(OL_MARKER_TOKEN, &(lb->end));
                            putLexPosition(lb, position);
                            parsedInfo.marker1Flag = true;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    if (lb->end == lb->lexemStream)
        return false;
    else
        return true;
}

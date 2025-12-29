#include "lexer.h"

#include <ctype.h>
#include <string.h>

#include "commons.h"
#include "filetable.h"
#include "globals.h"
#include "lexem.h"
#include "lexembuffer.h"
#include "log.h"
#include "misc.h"              /* requiresCreatingRefs() */
#include "options.h"
#include "parsing.h"
#include "server.h"


void gotOnLineCxRefs(Position position) {
    if (needsReferenceAtCursor(parsingConfig.operation)) {
        cxRefPosition = position;
    }
}

/* ***************************************************************** */
/*                         Lexical Analysis                          */
/* ***************************************************************** */

static void scanComment(CharacterBuffer *cb) {
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


static LexemCode scanConstantType(CharacterBuffer *cb, int *ch) {
    LexemCode lexem = CONSTANT;
    for (; *ch=='l'||*ch=='L'||*ch=='u'||*ch=='U'; ) {
        if (*ch=='l' || *ch=='L')
            lexem = LONG_CONSTANT;
        *ch = getChar(cb);
    }

    return lexem;
}


static LexemCode scanFloatingPointConstant(CharacterBuffer *cb, int *chPointer) {
    int ch = *chPointer;
    LexemCode rlex;

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
    if (ch == 'f' || ch == 'F' || ch == 'l' || ch == 'L') {
        ch = getChar(cb);
    }
    *chPointer = ch;

    return rlex;
}


static void noteNewLexemPosition(LexemBuffer *lb, CharacterBuffer *cb) {
    lb->fileOffset    = fileOffsetFor(cb);
    lb->position.file = cb->fileNumber;
    lb->position.line = cb->lineNumber;
    lb->position.col  = columnPosition(cb);
}

static LexemCode preprocessorLexemFromString(const char *preprocessorWord) {
    if (strcmp(preprocessorWord, "ifdef") == 0) {
        return CPP_IFDEF;
    } else if (strcmp(preprocessorWord, "ifndef") == 0) {
        return CPP_IFNDEF;
    } else if (strcmp(preprocessorWord, "if") == 0) {
        return CPP_IF;
    } else if (strcmp(preprocessorWord, "elif") == 0) {
        return CPP_ELIF;
    } else if (strcmp(preprocessorWord, "undef") == 0) {
        return CPP_UNDEF;
    } else if (strcmp(preprocessorWord, "else") == 0) {
        return CPP_ELSE;
    } else if (strcmp(preprocessorWord, "endif") == 0) {
        return CPP_ENDIF;
    } else if (strcmp(preprocessorWord, "pragma") == 0) {
        return CPP_PRAGMA;
    } else if (strcmp(preprocessorWord, "include") == 0) {
        return CPP_INCLUDE;
    } else if (strcmp(preprocessorWord, "include_next") == 0) {
        return CPP_INCLUDE_NEXT;
    } else if (strcmp(preprocessorWord, "define") == 0) {
        return CPP_DEFINE0;
    } else {
        // Some other un-interesting preprocessor line, like #error or #message...
        return CPP_LINE;
    }
}

static int extractPreprocessorWord(CharacterBuffer *cb, char preprocessorWord[]) {
    int ch;
    ch = getChar(cb);
    ch = skipBlanks(cb, ch);
    for (int i = 0; isalpha(ch) || isdigit(ch) || ch == '_'; i++) {
        preprocessorWord[i] = ch;
        preprocessorWord[i+1] = 0;
        ch = getChar(cb);
    }
    return ch;
}

static int processCppToken(CharacterBuffer *cb, LexemBuffer *lb) {
    int   ch;
    char  preprocessorWord[30];
    int   column;

    noteNewLexemPosition(lb, cb);

    column = columnPosition(cb);
    ch     = extractPreprocessorWord(cb, preprocessorWord);

    LexemCode lexem = preprocessorLexemFromString(preprocessorWord);
    switch (lexem) {
    case CPP_IFDEF:
    case CPP_IFNDEF:
    case CPP_IF:
    case CPP_ELIF:
    case CPP_UNDEF:
    case CPP_ELSE:
    case CPP_ENDIF:
    case CPP_PRAGMA:
    case CPP_LINE:
        putLexemWithColumn(lb, lexem, cb, column);
        break;
    case CPP_INCLUDE:
    case CPP_INCLUDE_NEXT:
        putLexemWithColumn(lb, lexem, cb, column);
        ch = putIncludeString(lb, cb, ch);
        break;
    case CPP_DEFINE0: {
        saveBackpatchPosition(lb);
        putLexemWithColumn(lb, lexem, cb, column);
        ch = skipBlanks(cb, ch);
        noteNewLexemPosition(lb, cb);
        ch = putIdentifierLexem(lb, cb, ch);
        if (ch == '(') {
            /* Discovered parameters so backpatch the previous lexem
             * code (CPP_DEFINE0) to indicate that this is not a text
             * replacement but a macro "function" */
            backpatchLexemCode(lb, CPP_DEFINE);
        }
        break;
    }
    default:
        errorMessage(ERR_ST, "Unexpected preprocessor line");
        break;
    }

    return ch;
}

static bool lexemStartsBeforeCursor(int fileOffsetForCurrentLexem) {
    return fileOffsetForCurrentLexem < options.olCursorOffset;
}

static bool lexemEndsAfterCursor(int currentOffset) {
    return currentOffset >= options.olCursorOffset;
}

static bool cursorIsAfterLastLexemInFile(int currentOffset) {
    return currentOffset + 1 == options.olCursorOffset;
}

/* Turn an identifier into a COMPLETE-lexem, return next character to process */
static void processCompletionOrSearch(CharacterBuffer *characterBuffer, LexemBuffer *lb, Position position,
                                     int fileOffsetForCurrentLexem, int deltaOffset, LexemCode thisLexemCode) {
    int currentOffset = fileOffsetFor(characterBuffer);

    if (lexemStartsBeforeCursor(fileOffsetForCurrentLexem)
        && (lexemEndsAfterCursor(currentOffset) || (characterBuffer->isAtEOF && cursorIsAfterLastLexemInFile(currentOffset)))) {
        log_debug("offset for current lexem == %d", fileOffsetForCurrentLexem);
        log_debug("options.olCursorOffset == %d", options.olCursorOffset);
        log_debug("currentOffset == %d", currentOffset);
        if (thisLexemCode == IDENTIFIER) {
            if (deltaOffset <= strlenOfBackpatchedIdentifier(lb)) {
                /* We need to backpatch the current IDENTIFIER with an IDENT_TO_COMPLETE */
                backpatchLexemCode(lb, IDENT_TO_COMPLETE);
                if (parsingConfig.operation == PARSER_OP_COMPLETION) {
                    /* And for completion we need to terminate the identifier where the cursor is */
                    /* Move to position cursor is on in the already written identifier */
                    /* We can use the backpatchP since it has moved to begining of string */
                    moveLexemStreamWriteToBackpatchPositonWithOffset(lb, deltaOffset);
                    terminateLexemString(lb);
                    /* And continue with writing the position */
                    putLexemPosition(lb, position);
                }
            } else {
                // completion after an identifier
                putCompletionLexem(lb, characterBuffer, currentOffset - options.olCursorOffset);
            }
        } else if ((thisLexemCode == LINE_TOKEN || thisLexemCode == STRING_LITERAL)
                   && (currentOffset != options.olCursorOffset)) {
            // completion inside special lexems, do
            // NO COMPLETION
        } else {
            // completion after another lexem
            putCompletionLexem(lb, characterBuffer, currentOffset - options.olCursorOffset);
        }
    }
}

static int scanIntegerValue(CharacterBuffer *cb, int ch, unsigned long *valueP) {
    unsigned long integerValue = 0;
    if (ch == '0') {
        ch = getChar(cb);
        if (ch == 'x' || ch == 'X') {
            /* hexadecimal */
            ch = getChar(cb);
            while (isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
                if (ch >= 'a')
                    integerValue = integerValue * 16 + ch - 'a' + 10;
                else if (ch >= 'A')
                    integerValue = integerValue * 16 + ch - 'A' + 10;
                else
                    integerValue = integerValue * 16 + ch - '0';
                ch = getChar(cb);
            }
        } else {
            /* octal */
            while (isdigit(ch) && ch <= '8') {
                integerValue = integerValue * 8 + ch - '0';
                ch           = getChar(cb);
            }
        }
    } else {
        /* decimal */
        while (isdigit(ch)) {
            integerValue = integerValue * 10 + ch - '0';
            ch           = getChar(cb);
        }
    }
    *valueP = integerValue;
    return ch;
}

static int handleCharConstant(CharacterBuffer *cb, LexemBuffer *lb, int lexemStartingColumn) {
    int      fileOffsetForLexemStart, ch;
    unsigned chval = 0;

    fileOffsetForLexemStart = fileOffsetFor(cb);
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
        putCharLiteralLexem(lb, cb, lexemStartingColumn, fileOffsetFor(cb) - fileOffsetForLexemStart,
                            chval);
        ch = getChar(cb);
    }

    return ch;
}

static int handleBlockComment(CharacterBuffer *cb, LexemBuffer *lb) {
    int ch = getChar(cb);
    if (ch == '&') {
        /* Block comment with a '&' is commented code that should be
         * analysed anyway, so ignore and continue with next lexem */
        return getChar(cb);
    } else {
        ungetChar(cb, ch);
        ch = '*';
    } /* !!! COPY BLOCK TO '/n' */

    int line = lineNumberFrom(cb);
    scanComment(cb);
    putLexemLines(lb, lineNumberFrom(cb) - line);

    return getChar(cb);
}

static int handleLineComment(CharacterBuffer *cb, LexemBuffer *lb) {
    int ch = getChar(cb);
    if (ch == '&') {
        // A line comment with a '&' is commented code that should be analysed anyway, so ignore
        ch = getChar(cb);
        return ch;
    }
    int line = lineNumberFrom(cb);
    while (ch != '\n' && ch != -1) {
        ch = getChar(cb);
        if (ch == '\\') {
            ch = getChar(cb);
            if (ch == '\n') {
                cb->lineNumber++;
                cb->lineBegin    = cb->nextUnread;
                cb->columnOffset = 0;
            }
            ch = getChar(cb);
        }
    }
    putLexemLines(lb, lineNumberFrom(cb) - line);

    return ch;
}

static int handleNewline(CharacterBuffer *cb, LexemBuffer *lb, int lexemStartingColumn) {
    int ch;
    if (columnPosition(cb) >= MAX_REFERENCABLE_COLUMN) {
        FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_COLUMN, read TROUBLES in README file",
                    XREF_EXIT_ERR);
    }
    if (lineNumberFrom(cb) >= MAX_REFERENCABLE_LINE) {
        FATAL_ERROR(ERR_ST, "position over MAX_REFERENCABLE_LINE, read TROUBLES in README file",
                    XREF_EXIT_ERR);
    }
    cb->lineNumber++;
    cb->lineBegin    = cb->nextUnread;
    cb->columnOffset = 0;

    ch = getChar(cb);
    ch = skipBlanks(cb, ch);

    /* And why is this here and not using the "standard" code for comment? */
    if (ch == '/') {
        ch = getChar(cb);
        if (ch == '*') {
            ch = getChar(cb);
            if (ch == '&') {
                /* ****** a code comment, ignore */
                ch = getChar(cb);
            } else {
                ungetChar(cb, ch);
                ch       = '*';
                int line = lineNumberFrom(cb);
                scanComment(cb);
                putLexemLines(lb, lineNumberFrom(cb) - line);
                ch = getChar(cb);
                ch = skipBlanks(cb, ch);
            }
        } else {
            ungetChar(cb, ch);
            ch = '/';
        }
    }
    putLexemWithColumn(lb, '\n', cb, lexemStartingColumn);
    if (ch == '#' && LANGUAGE(LANG_C | LANG_YACC)) {
        ch = processCppToken(cb, lb);
    }

    return ch;
}

static void putDoubleParenthesisSemicolonsAMarkerAndDoubleParenthesis(LexemBuffer *lb, int parChar,
                                                                      Position position) {
    /* TODO Why do we put this contrived sequence of lexems? */
    putLexemCodeWithPosition(lb, parChar, position);
    putLexemCodeWithPosition(lb, ';', position);
    putLexemCodeWithPosition(lb, OL_MARKER_TOKEN, position);
    putLexemCodeWithPosition(lb, parChar, position);
}

static int skipPossibleStringPrefix(CharacterBuffer *cb, int ch) {
    if (ch == 'L' || ch == 'u' || ch == 'U') {
        int secondCharacter = getChar(cb);
        if (secondCharacter == '"') {
            return secondCharacter;
        } else {
            if (secondCharacter == '8') {
                int thirdCharacter = getChar(cb);
                if (thirdCharacter == '"')
                    return thirdCharacter;
                else
                    ungetChar(cb, thirdCharacter);
            }
            ungetChar(cb, secondCharacter);
        }
    }
    return ch;
}

bool buildLexemFromCharacters(CharacterBuffer *cb, LexemBuffer *lb) {
    LexemCode lexem;
    int lexemStartingColumn;
    int fileOffsetForLexemStart;

    shiftAnyRemainingLexems(lb);

    char *lexemLimit = getLexemStreamWrite(lb) + LEXEM_BUFFER_SIZE - MAX_LEXEM_SIZE;

    int ch = getChar(cb);
    do {
        ch = skipBlanks(cb, ch);
        /* Space for more lexems? */
        if ((char *)getLexemStreamWrite(lb) >= lexemLimit) {
            ungetChar(cb, ch);
            break;
        }
        noteNewLexemPosition(lb, cb);
        char *startOfCurrentLexem = getLexemStreamWrite(lb);
        saveBackpatchPosition(lb);
        lexemStartingColumn = columnPosition(cb);
        log_debug("lexemStartingColumn = %d", lexemStartingColumn);

        ch = skipPossibleStringPrefix(cb, ch);

        if (ch == '_' || isalpha(ch) || (ch=='$' && LANGUAGE(LANG_YACC))) {
            ch = putIdentifierLexem(lb, cb, ch);
            lexem = IDENTIFIER;
            goto nextLexem;
        } else if (isdigit(ch)) {
            /* ***************   number *******************************  */
            long unsigned integerValue=0;
            fileOffsetForLexemStart = fileOffsetFor(cb);
            ch = scanIntegerValue(cb, ch, &integerValue);
            if (ch == '.' || ch == 'e' || ch == 'E') {
                /* floating point */
                lexem = scanFloatingPointConstant(cb, &ch);
                putFloatingPointLexem(lb, lexem, cb, lexemStartingColumn, fileOffsetForLexemStart);
                goto nextLexem;
            }
            /* integer */
            lexem = scanConstantType(cb, &ch);
            putIntegerLexem(lb, lexem, integerValue, cb, lexemStartingColumn, fileOffsetForLexemStart);
            goto nextLexem;
        } else switch (ch) {
                /* ************   special character *********************  */
            case '.':
                fileOffsetForLexemStart = fileOffsetFor(cb);
                ch = getChar(cb);
                if (ch == '.' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    if (ch == '.') {
                        ch = getChar(cb);
                        putLexemWithColumn(lb, ELLIPSIS, cb, lexemStartingColumn);
                        goto nextLexem;
                    } else {
                        ungetChar(cb, ch);
                        ch = '.';
                    }
                    putLexemWithColumn(lb, '.', cb, lexemStartingColumn);
                    goto nextLexem;
                } else if (isdigit(ch)) {
                    /* floating point constant */
                    ungetChar(cb, ch);
                    ch = '.';
                    lexem = scanFloatingPointConstant(cb, &ch);
                    putFloatingPointLexem(lb, lexem, cb, lexemStartingColumn, fileOffsetForLexemStart);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '.', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '-':
                ch = getChar(cb);
                if (ch=='=') {
                    putLexemWithColumn(lb, SUB_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='-') {
                    putLexemWithColumn(lb, DEC_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='>' && LANGUAGE(LANG_C|LANG_YACC)) {
                    ch = getChar(cb);
                    putLexemWithColumn(lb, PTR_OP, cb, lexemStartingColumn);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '-', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '+':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, ADD_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '+') {
                    putLexemWithColumn(lb, INC_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '+', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '>':
                ch = getChar(cb);
                if (ch == '>') {
                    ch = getChar(cb);
                    if (ch=='=') {
                        putLexemWithColumn(lb, RIGHT_ASSIGN, cb, lexemStartingColumn);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexemWithColumn(lb, RIGHT_OP, cb, lexemStartingColumn);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexemWithColumn(lb, GE_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '>', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '<':
                ch = getChar(cb);
                if (ch == '<') {
                    ch = getChar(cb);
                    if (ch=='=') {
                        putLexemWithColumn(lb, LEFT_ASSIGN, cb, lexemStartingColumn);
                        ch = getChar(cb);
                        goto nextLexem;
                    } else {
                        putLexemWithColumn(lb, LEFT_OP, cb, lexemStartingColumn);
                        goto nextLexem;
                    }
                } else if (ch == '=') {
                    putLexemWithColumn(lb, LE_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '<', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '*':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, MUL_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '*', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '%':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, MOD_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                }
                /*&
                  else if (LANGUAGE(LANG_YACC) && ch == '{') {
                      putLexemWithColumn(lb, YACC_PERC_LPAR, cb, lexemStartingColumn);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  else if (LANGUAGE(LANG_YACC) && ch == '}') {
                      putLexemWithColumn(lb, YACC_PERC_RPAR, cb, lexemStartingColumn);
                      ch = getChar(cb);
                      goto nextLexem;
                  }
                  &*/
                else {
                    putLexemWithColumn(lb, '%', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '&':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, AND_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch == '&') {
                    putLexemWithColumn(lb, AND_OP, cb, lexemStartingColumn);
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
                        putLexemWithColumn(lb, '&', cb, lexemStartingColumn);
                        goto nextLexem;
                    }
                } else {
                    putLexemWithColumn(lb, '&', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '^':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, XOR_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else {
                    putLexemWithColumn(lb, '^', cb, lexemStartingColumn);
                    goto nextLexem;
                }

            case '|':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, OR_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                } else if (ch == '|') {
                    putLexemWithColumn(lb, OR_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                } else {
                    putLexemWithColumn(lb, '|', cb, lexemStartingColumn);
                }
                goto nextLexem;

            case '=':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, EQ_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                } else {
                    putLexemWithColumn(lb, '=', cb, lexemStartingColumn);
                }
                goto nextLexem;

            case '!':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, NE_OP, cb, lexemStartingColumn);
                    ch = getChar(cb);
                } else {
                    putLexemWithColumn(lb, '!', cb, lexemStartingColumn);
                }
                goto nextLexem;

            case ':':
                ch = getChar(cb);
                putLexemWithColumn(lb, ':', cb, lexemStartingColumn);
                goto nextLexem;

            case '\'': {
                ch = handleCharConstant(cb, lb, lexemStartingColumn);
                goto nextLexem;
            }

            case '\"':
                putStringConstantLexem(lb, cb, lexemStartingColumn);
                ch = getChar(cb);
                goto nextLexem;

            case '/':
                ch = getChar(cb);
                if (ch == '=') {
                    putLexemWithColumn(lb, DIV_ASSIGN, cb, lexemStartingColumn);
                    ch = getChar(cb);
                    goto nextLexem;
                } else if (ch=='*') {
                    ch = handleBlockComment(cb, lb);
                    goto nextLexem;

                } else if (ch=='/') {
                    ch = handleLineComment(cb, lb);
                } else {
                    putLexemWithColumn(lb, '/', cb, lexemStartingColumn);
                }
                goto nextLexem;

            case '\\':
                ch = getChar(cb);
                if (ch == '\n') {
                    cb->lineNumber++;
                    cb->lineBegin = cb->nextUnread;
                    cb->columnOffset = 0;
                    putLexemLines(lb, 1);
                    ch = getChar(cb);
                } else {
                    putLexemWithColumn(lb, '\\', cb, lexemStartingColumn);
                }
                goto nextLexem;

            case '\n':
                ch = handleNewline(cb, lb, lexemStartingColumn);
                goto nextLexem;

            case '#':
                ch = getChar(cb);
                if (ch == '#') {
                    ch = getChar(cb);
                    putLexemWithColumn(lb, CPP_COLLATION, cb, lexemStartingColumn);
                } else {
                    putLexemWithColumn(lb, '#', cb, lexemStartingColumn);
                }
                goto nextLexem;

            case -1:
                /* ** probably end of file ** */
                goto nextLexem;

            default:
                if (ch >= 32) {         /* small chars ignored */
                    putLexemWithColumn(lb, ch, cb, lexemStartingColumn);
                }
                ch = getChar(cb);
                goto nextLexem;
            }
        assert(0);
    nextLexem:
        if (options.mode == ServerMode) {
            int parChar;
            Position position;
            int currentLexemFileOffset = lb->fileOffset;
            position = lb->position;

            if (fileNumberFrom(cb) == originalFileNumber && fileNumberFrom(cb) != NO_FILE_NUMBER
                && fileNumberFrom(cb) != -1) {
                if (parsingConfig.operation == PARSER_OP_EXTRACT) {
                    ch = skipBlanks(cb, ch);
                    int apos = fileOffsetFor(cb);
                    log_debug(":pos1==%d, olCursorOffset==%d, olMarkOffset==%d",apos,options.olCursorOffset,options.olMarkOffset);
                    // all this is very, very HACK!!!
                    if (apos >= options.olCursorOffset && !parsedInfo.blockMarker1Set) {
                        if (parsedInfo.blockMarker2Set)
                            parChar='}';
                        else
                            parChar = '{';
                        putDoubleParenthesisSemicolonsAMarkerAndDoubleParenthesis(lb, parChar, position);
                        parsedInfo.blockMarker1Set = true;
                    } else if (apos >= options.olMarkOffset && !parsedInfo.blockMarker2Set){
                        if (parsedInfo.blockMarker1Set)
                            parChar='}';
                        else
                            parChar = '{';
                        putDoubleParenthesisSemicolonsAMarkerAndDoubleParenthesis(lb, parChar, position);
                        parsedInfo.blockMarker2Set = true;
                    }
                } else if (parsingConfig.operation == PARSER_OP_COMPLETION) {
                    ch = skipBlanks(cb, ch);
                    lexem = peekLexemCodeAt(startOfCurrentLexem);
                    processCompletionOrSearch(cb, lb, position, currentLexemFileOffset,
                                              options.olCursorOffset - currentLexemFileOffset, lexem);
                } else {
                    if (currentLexemFileOffset <= options.olCursorOffset
                        && fileOffsetFor(cb) >= options.olCursorOffset
                    ) {
                        if (needsReferenceAtCursor(parsingConfig.operation)) {
                            cxRefPosition = position;
                        }
                    }
                    if (parsingConfig.operation == PARSER_OP_VALIDATE_MOVE_TARGET) {
                        // TODO: Figure out what the problem with this
                        // is for C. Marian's comment below indicate
                        // CPP problem, but if we will try to
                        // implement "Move Function" for C, we need to
                        // be able to do this. There is no test for
                        // MOVE_FUNCTION yet...

                        //if (LANGUAGE(LANG_JAVA)) {
                        // there is a problem with this, when browsing at CPP construction
                        // that is why I restrict it to Java language! It is usefull
                        // only for Java refactorings
                        ch = skipBlanks(cb, ch);
                        int apos = fileOffsetFor(cb);
                        if (apos >= options.olCursorOffset && !parsedInfo.blockMarker1Set) {
                            putLexemCodeWithPosition(lb, OL_MARKER_TOKEN, position);
                            parsedInfo.blockMarker1Set = true;
                        }
                    }
                }
            }
        }
    } while (ch != -1);

    if ((char *)getLexemStreamWrite(lb) == lb->lexemStream)
        return false;
    else
        return true;
}

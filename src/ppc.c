#include "ppc.h"

#include <string.h>

#include "commons.h"
#include "globals.h"
#include "protocol.h"
#include "filetable.h"
#include "options.h"


static int ppcIndentOffset = 0;

static size_t utf8CharacterLength(char *string) {
    size_t len = 0;
    while (*string) {
        len += (*string & 0xC0) != 0x80;  // Count only UTF-8 leading bytes
        string++;
    }
    return len;
}

static void ppcGenOffsetPosition(char *fn, int offset) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d %s=%ld>%s</%s>\n",
            PPC_OFFSET_POSITION,
            PPCA_OFFSET, offset,
            PPCA_LEN, (unsigned long)strlen(fn), fn,
            PPC_OFFSET_POSITION);
}

static void ppcGenMarker(EditorMarker *m) {
    ppcGenOffsetPosition(m->buffer->fileName, m->offset);
}

static void ppcGenPosition(Position position) {
    char *fn;
    fn = getFileItemWithFileNumber(position.file)->name;
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d %s=%d %s=%ld>%s</%s>\n",
            PPC_LC_POSITION,
            PPCA_LINE, position.line, PPCA_COL, position.col,
            PPCA_LEN, (unsigned long)strlen(fn), fn,
            PPC_LC_POSITION);
}

void ppcSynchronize(void) {
    fprintf(stdout, "<%s>\n", PPC_SYNCHRO_RECORD);
    fflush(stdout);
}

void ppcIndent(void) {
    for (int i=0; i<ppcIndentOffset; i++)
        fputc(' ', outputFile);
}

void ppcGotoPosition(Position position) {
    ppcBegin(PPC_GOTO);
    ppcGenPosition(position);
    ppcEnd(PPC_GOTO);
}

void ppcGotoMarker(EditorMarker *pos) {
    ppcBegin(PPC_GOTO);
    ppcGenMarker(pos);
    ppcEnd(PPC_GOTO);
}

void ppcGotoOffsetPosition(char *fname, int offset) {
    ppcBegin(PPC_GOTO);
    ppcGenOffsetPosition(fname, offset);
    ppcEnd(PPC_GOTO);
}

void ppcBegin(char *kind) {
    ppcIndent();
    fprintf(outputFile, "<%s>\n", kind);
    ppcIndentOffset++;
}

void ppcBeginWithStringAttribute(char *kind, char *attr, char *val) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%s>", kind, attr, val);
    ppcIndentOffset++;
}

void ppcEnd(char *kind) {
    ppcIndentOffset--;
    ppcIndent();
    fprintf(outputFile, "</%s>\n", kind);
}

void ppcBeginWithNumericValue(char *kind, int val) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d>\n", kind, PPCA_VALUE, val);
}

void ppcBeginWithNumericValueAndAttribute(char *kind, int value, char *attr, char *attrVal) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d %s=%s>\n", kind, PPCA_VALUE, value, attr, attrVal);
}

void ppcBeginWithTwoNumericAttributes(char *kind, char *attr1, int val1, char *attr2, int val2) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d %s=%d>\n", kind, attr1, val1, attr2, val2);
}

void ppcBeginAllCompletions(int nofocus, int len) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d %s=%d>", PPC_ALL_COMPLETIONS, PPCA_NO_FOCUS, nofocus, PPCA_LEN, len);
}

void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d %s=%ld>%s</%s>\n", kind,
            attr, val, PPCA_LEN, (unsigned long)strlen(message),
            message, kind);
}

void ppcValueRecord(char *kind, int val,char *message) {
    ppcIndent();
    ppcGenRecordWithNumeric(kind, PPCA_VALUE, val, message);
}

void ppcGenRecord(char *kind, char *message) {
    assert(strlen(message) < MAX_PPC_RECORD_SIZE-1);
    ppcIndent();
    fprintf(outputFile, "<%s %s=%ld>%s</%s>\n", kind, PPCA_LEN, utf8CharacterLength(message),
            message, kind);
}

void ppcDisplaySelection(char *message, int messageType) {
    ppcIndent();
    fprintf(outputFile, "<%s %s=%ld %s=%d %s=%d>%s</%s>\n", PPC_DISPLAY_RESOLUTION,
            PPCA_LEN, (unsigned long)strlen(message),
            PPCA_MTYPE, messageType,
            PPCA_CONTINUE, 1,
            message, PPC_DISPLAY_RESOLUTION);
}

void ppcReplace(char *file, int offset, char *oldName, int oldLen, char *newName) {
    ppcGotoOffsetPosition(file, offset);
    ppcBegin(PPC_REFACTORING_REPLACEMENT);
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d>", PPC_STRING_VALUE, PPCA_LEN, oldLen);
    for (int i=0; i<oldLen; i++)
        putc(oldName[i], outputFile);
    fprintf(outputFile, "</%s> ", PPC_STRING_VALUE);
    ppcGenRecord(PPC_STRING_VALUE, newName);
    ppcEnd(PPC_REFACTORING_REPLACEMENT);
}

void ppcPreCheck(EditorMarker *pos, int oldLen) {
    char    *bufferedText;
    bufferedText = pos->buffer->allocation.text + pos->offset;
    ppcGotoMarker(pos);
    ppcIndent();
    fprintf(outputFile, "<%s %s=%d>", PPC_REFACTORING_PRECHECK, PPCA_LEN, oldLen);
    for (int i=0; i<oldLen; i++)
        putc(bufferedText[i], outputFile);
    fprintf(outputFile, "</%s>\n", PPC_REFACTORING_PRECHECK);
}

void ppcReferencePreCheck(Reference *r, char *text) {
    ppcGotoPosition(r->position);
    ppcIndent();
    fprintf(outputFile, "<%s %s=%ld>", PPC_REFACTORING_PRECHECK, PPCA_LEN,
            (unsigned long)strlen(text));
    fprintf(outputFile, "%s", text);
    fprintf(outputFile, "</%s>\n", PPC_REFACTORING_PRECHECK);
}

void ppcWarning(char *message) {
    ppcGenRecord(PPC_WARNING, message);
}

void ppcBottomWarning(char *message) {
    ppcGenRecordWithNumeric(PPC_BOTTOM_WARNING, PPCA_BEEP, 1, message);
}

void ppcBottomInformation(char *message) {
    ppcGenRecord(PPC_BOTTOM_INFORMATION, message);
    //&ppcGenRecord(PPC_INFORMATION,message);
    fflush(outputFile);
}

void ppcIndicateNoReference(void) {
    assert(options.xref2);
    ppcBottomInformation("No reference");
}

void ppcAskConfirmation(char *message) {
    ppcGenRecord(PPC_ASK_CONFIRMATION, message);
}

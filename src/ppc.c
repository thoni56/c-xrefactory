#include "ppc.h"

#include "globals.h"
#include "protocol.h"


static int ppcIndentOffset = 0;

void ppcGenSynchroRecord(void) {
    fprintf(stdout, "<%s>\n", PPC_SYNCHRO_RECORD);
    fflush(stdout);
}

void ppcIndent(void) {
    int i;
    for(i=0; i<ppcIndentOffset; i++)
        fputc(' ', communicationChannel);
}

void ppcGenPosition(Position *p) {
    char *fn;
    assert(p!=NULL);
    fn = fileTable.tab[p->file]->name;
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d %s=%d %s=%ld>%s</%s>\n",
            PPC_LC_POSITION,
            PPCA_LINE, p->line, PPCA_COL, p->col,
            PPCA_LEN, (unsigned long)strlen(fn), fn,
            PPC_LC_POSITION);
    //&ppcGenRecord(PPC_FILE, fileTable.tab[p->file]->name);
    //&ppcGenNumericRecord(PPC_LINE, p->line,"");
    //&ppcGenNumericRecord(PPC_COL, p->col,"");
}

void ppcGenGotoPositionRecord(Position *p) {
    ppcGenRecordBegin(PPC_GOTO);
    ppcGenPosition(p);
    ppcGenRecordEnd(PPC_GOTO);
}

void ppcGenGotoMarkerRecord(EditorMarker *pos) {
    ppcGenRecordBegin(PPC_GOTO);
    ppcGenMarker(pos);
    ppcGenRecordEnd(PPC_GOTO);
}

void ppcGenOffsetPosition(char *fn, int offset) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d %s=%ld>%s</%s>\n",
            PPC_OFFSET_POSITION,
            PPCA_OFFSET, offset,
            PPCA_LEN, (unsigned long)strlen(fn), fn,
            PPC_OFFSET_POSITION);
    //&ppcGenRecord(PPC_FILE, m->buffer->name);
    //&ppcGenNumericRecord(PPC_OFFSET, m->offset,"");
}

void ppcGenMarker(EditorMarker *m) {
    ppcGenOffsetPosition(m->buffer->name, m->offset);
}

void ppcGenGotoOffsetPosition(char *fname, int offset) {
    ppcGenRecordBegin(PPC_GOTO);
    ppcGenOffsetPosition(fname, offset);
    ppcGenRecordEnd(PPC_GOTO);
}

void ppcGenRecordBegin(char *kind) {
    ppcIndent();
    fprintf(communicationChannel, "<%s>\n", kind);
    ppcIndentOffset ++;
}

void ppcGenRecordWithAttributeBegin(char *kind, char *attr, char *val) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%s>", kind, attr, val);
    ppcIndentOffset ++;
}

void ppcGenRecordWithNumAttributeBegin(char *kind, char *attr, int val) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d>", kind, attr, val);
    ppcIndentOffset ++;
}

void ppcGenRecordEnd(char *kind) {
    ppcIndentOffset --;
    ppcIndent();
    fprintf(communicationChannel, "</%s>\n", kind);
}

void ppcGenNumericRecordBegin(char *kind, int val) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d>\n", kind, PPCA_VALUE, val);
}

void ppcGenWithNumericAndRecordBegin(char *kind, int val, char *attr, char *attrVal) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d %s=%s>\n", kind, PPCA_VALUE, val, attr, attrVal);
}

void ppcGenTwoNumericAndRecordBegin(char *kind, char *attr1, int val1, char *attr2, int val2) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d %s=%d>\n", kind, attr1, val1, attr2, val2);
}

void ppcGenAllCompletionsRecordBegin(int nofocus, int len) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d %s=%d>", PPC_ALL_COMPLETIONS, PPCA_NO_FOCUS, nofocus, PPCA_LEN, len);
}

void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d %s=%ld>%s</%s>\n", kind,
            attr, val, PPCA_LEN, (unsigned long)strlen(message),
            message, kind);
}

void ppcGenNumericRecord(char *kind, int val,char *message) {
    ppcIndent();
    ppcGenRecordWithNumeric(kind, PPCA_VALUE, val, message);
}

void ppcGenRecord(char *kind, char *message) {
    assert(strlen(message) < MAX_PPC_RECORD_SIZE-1);
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%ld>%s</%s>\n", kind, PPCA_LEN, (unsigned long)strlen(message),
            message, kind);
}

// use this for debugging purposes only!!!
void ppcGenTmpBuff(void) {
    char tmpBuff[TMP_BUFF_SIZE];

    ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
    //&ppcGenRecord(PPC_INFORMATION,tmpBuff);
    fflush(communicationChannel);
}

void ppcGenDisplaySelectionRecord(char *message, int messageType, int continuation) {
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%ld %s=%d %s=%d>%s</%s>\n", PPC_DISPLAY_RESOLUTION,
            PPCA_LEN, (unsigned long)strlen(message),
            PPCA_MTYPE, messageType,
            PPCA_CONTINUE, (continuation==CONTINUATION_ENABLED) ? 1 : 0,
            message, PPC_DISPLAY_RESOLUTION);
}

void ppcGenReplaceRecord(char *file, int offset, char *oldName, int oldLen, char *newName) {
    int i;
    ppcGenGotoOffsetPosition(file, offset);
    ppcGenRecordBegin(PPC_REFACTORING_REPLACEMENT);
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d>", PPC_STRING_VALUE, PPCA_LEN, oldLen);
    for(i=0; i<oldLen; i++) putc(oldName[i], communicationChannel);
    fprintf(communicationChannel, "</%s> ", PPC_STRING_VALUE);
    ppcGenRecord(PPC_STRING_VALUE, newName);
    ppcGenRecordEnd(PPC_REFACTORING_REPLACEMENT);
}

void ppcGenPreCheckRecord(EditorMarker *pos, int oldLen) {
    int     i;
    char    *bufferedText;
    bufferedText = pos->buffer->allocation.text + pos->offset;
    ppcGenGotoMarkerRecord(pos);
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d>", PPC_REFACTORING_PRECHECK, PPCA_LEN, oldLen);
    for(i=0; i<oldLen; i++) putc(bufferedText[i], communicationChannel);
    fprintf(communicationChannel, "</%s>\n", PPC_REFACTORING_PRECHECK);
}

void ppcGenReferencePreCheckRecord(Reference *r, char *text) {
    int     len;
    len = strlen(text);
    ppcGenGotoPositionRecord(&r->p);
    ppcIndent();
    fprintf(communicationChannel, "<%s %s=%d>", PPC_REFACTORING_PRECHECK, PPCA_LEN, len);
    fprintf(communicationChannel, "%s", text);
    fprintf(communicationChannel, "</%s>\n", PPC_REFACTORING_PRECHECK);
}

void ppcGenDefinitionNotFoundWarning(void) {
    ppcGenRecord(PPC_WARNING, DEFINITION_NOT_FOUND_MESSAGE);
}

void ppcGenDefinitionNotFoundWarningAtBottom(void) {
    ppcGenRecordWithNumeric(PPC_BOTTOM_WARNING, PPCA_BEEP, 1, DEFINITION_NOT_FOUND_MESSAGE);
}

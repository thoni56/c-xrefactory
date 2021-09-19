#ifndef PPC_INCLUDED
#define PPC_INCLUDED

#include "editor.h"


#define DEFINITION_NOT_FOUND_MESSAGE "Definition not found"


extern void ppcSynchronize(void);
extern void ppcIndent(void);
extern void ppcGotoPosition(Position *p);
extern void ppcGotoOffsetPosition(char *fname, int offset);
extern void ppcBegin(char *kind);
extern void ppcBeginWithAttribute(char *kind, char *attr, char *val);
extern void ppcBeginWithNumericAttribute(char *kind, char *attr, int val);
extern void ppcBeginWithNumericValue(char *kind, int val);
extern void ppcBeginWithTwoNumericValues(char *kind, char *attr1, int val1, char *attr2, int val2);
extern void ppcBeginWithNumericValueAndAttribute(char *kind, int val, char *attr, char *attrVal);
extern void ppcBeginAllCompletions(int nofocus, int len);
extern void ppcEnd(char *kind);
extern void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message);
extern void ppcGenNumericRecord(char *kind, int val, char *message);
extern void ppcGenRecord(char *kind, char *message);
extern void ppcDisplaySelection(char *message, int messageType, int continuation);
extern void ppcGotoMarker(EditorMarker *pos);
extern void ppcGenDefinitionNotFoundWarning(void);
extern void ppcGenDefinitionNotFoundWarningAtBottom(void);
extern void ppcReplace(char *file, int offset, char *oldName, int oldNameLen, char *newName);
extern void ppcPreCheck(EditorMarker *pos, int oldLen);
extern void ppcReferencePreCheck(Reference *r, char *text);

extern void ppcGenTmpBuff(void);

#endif

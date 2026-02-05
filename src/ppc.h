#ifndef PPC_INCLUDED
#define PPC_INCLUDED

#include "reference.h"
#include "editormarker.h"


extern void ppcSynchronize(void);
extern void ppcIndent(void);
extern void ppcGotoPosition(Position position);
extern void ppcGotoOffsetPosition(char *fname, int offset);
extern void ppcDisplaySelection(char *message, int messageType);
extern void ppcGotoMarker(EditorMarker *pos);
extern void ppcReplace(char *file, int offset, char *oldName, int oldNameLen, char *newName);
extern void ppcPreCheck(EditorMarker *pos, int oldLen);
extern void ppcReferencePreCheck(Reference *r, char *text);

extern void ppcBegin(char *kind);
extern void ppcBeginWithStringAttribute(char *kind, char *attr, char *val);
extern void ppcBeginWithNumericValue(char *kind, int val);
extern void ppcBeginWithTwoNumericAttributes(char *kind, char *attr1, int val1, char *attr2, int val2);
extern void ppcBeginWithNumericValueAndAttribute(char *kind, int val, char *attr, char *attrVal);
extern void ppcBeginAllCompletions(int nofocus, int len);
extern void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message);
extern void ppcValueRecord(char *kind, int val, char *message);
extern void ppcEnd(char *kind);

extern void ppcBottomInformation(char *string);
extern void ppcWarning(char *message);
extern void ppcBottomWarning(char *message);
extern void ppcIndicateNoReference(void);

extern void ppcAskConfirmation(char *message);

extern void ppcGenRecord(char *kind, char *message);

#endif

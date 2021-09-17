#ifndef PPC_INCLUDED
#define PPC_INCLUDED

#include "editor.h"


extern void ppcGenSynchroRecord(void);
extern void ppcIndent(void);
extern void ppcGenGotoOffsetPosition(char *fname, int offset);
extern void ppcGenRecordBegin(char *kind);
extern void ppcGenRecordWithAttributeBegin(char *kind, char *attr, char *val);
extern void ppcGenRecordWithNumAttributeBegin(char *kind, char *attr, int val);
extern void ppcGenRecordEnd(char *kind);
extern void ppcGenNumericRecordBegin(char *kind, int val);
extern void ppcGenTwoNumericAndRecordBegin(char *kind, char *attr1, int val1, char *attr2, int val2);
extern void ppcGenWithNumericAndRecordBegin(char *kind, int val, char *attr, char *attrVal);
extern void ppcGenAllCompletionsRecordBegin(int nofocus, int len);
extern void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message);
extern void ppcGenNumericRecord(char *kind, int val, char *message);
extern void ppcGenRecord(char *kind, char *message);
extern void ppcGenTmpBuff(void);
extern void ppcGenDisplaySelectionRecord(char *message, int messageType, int continuation);
extern void ppcGenGotoMarkerRecord(EditorMarker *pos);
extern void ppcGenPosition(Position *p);
extern void ppcGenDefinitionNotFoundWarning(void);
extern void ppcGenDefinitionNotFoundWarningAtBottom(void);
extern void ppcGenReplaceRecord(char *file, int offset, char *oldName, int oldNameLen, char *newName);
extern void ppcGenPreCheckRecord(EditorMarker *pos, int oldLen);
extern void ppcGenReferencePreCheckRecord(Reference *r, char *text);
extern void ppcGenGotoPositionRecord(Position *p);
extern void ppcGenOffsetPosition(char *fn, int offset);
extern void ppcGenMarker(EditorMarker *m);


#endif

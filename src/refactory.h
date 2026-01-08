#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "editormarker.h"
#include "options.h"
#include "proto.h"


extern Options refactoringOptions;

extern void refactory(void);
extern void applyWholeRefactoringFromUndo(void);
extern void removeNonCommentCode(EditorMarker *marker, int length);
extern EditorMarker *createMarkerForExpressionStart(EditorMarker *marker, ExpressionStartKind startKind);
extern void ensureReferencesAreUpdated(char *project);

#endif

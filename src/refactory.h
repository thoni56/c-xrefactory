#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "proto.h"
#include "editor.h"


typedef enum trivialPreChecks {
    TPC_NONE,                   /* Must be 0 because of initOpt in globals.h */
    TPC_GET_LAST_IMPORT_LINE, // not really a check
} TrivialPreChecks;

typedef struct tpCheckMoveClassData {
    struct pushAllInBetweenData  mm;
    char		*spack;
    char		*tpack;
    int			transPackageMove;
    char		*sclass;
} TpCheckMoveClassData;


extern void refactoryAskForReallyContinueConfirmation(void);
extern void refactoryDisplayResolutionDialog(char *message,int messageType, int continuation);
extern void editorApplyUndos(EditorUndo *undos, EditorUndo *until, EditorUndo **undoundo, int gen);
extern void editorUndoUntil(EditorUndo *until, EditorUndo **undoUndo);
extern void mainRefactory();

#endif

#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "proto.h"
#include "editor.h"


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

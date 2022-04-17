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
} S_tpCheckMoveClassData;


extern void refactoryAskForReallyContinueConfirmation(void);
extern void refactoryDisplayResolutionDialog(char *message,int messageType, int continuation);
extern void editorApplyUndos(EditorUndo *undos, EditorUndo *until, EditorUndo **undoundo, int gen);
extern void editorUndoUntil(EditorUndo *until, EditorUndo **undoUndo);
extern void mainRefactory();
extern bool tpCheckSourceIsNotInnerClass(void);
extern bool tpCheckMoveClassAccessibilities(void);
extern bool tpCheckSuperMethodReferencesForDynToSt(void);
extern bool tpCheckOuterScopeUsagesForDynToSt(void);
extern bool tpCheckMethodReferencesWithApplOnSuperClassForPullUp(void);
extern bool tpCheckSuperMethodReferencesForPullUp(void);
extern bool tpCheckSuperMethodReferencesAfterPushDown(void);
extern void tpCheckFillMoveClassData(S_tpCheckMoveClassData *dd, char *spack, char *tpack);
extern bool tpCheckTargetToBeDirectSubOrSuperClass(int flag, char *subOrSuper);
extern bool tpPullUpFieldLastPreconditions(void);
extern bool tpPushDownFieldLastPreconditions(void);

#endif

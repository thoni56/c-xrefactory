#ifndef REFACTORY_H
#define REFACTORY_H

#include "proto.h"

extern void refactoryAskForReallyContinueConfirmation(void);
extern void refactoryDisplayResolutionDialog(char *message,int messageType, int continuation);
extern void editorApplyUndos(S_editorUndo *undos, S_editorUndo *until, S_editorUndo **undoundo, int gen);
extern void editorUndoUntil(S_editorUndo *until, S_editorUndo **undoUndo);
extern void mainRefactory(int argc, char **argv);
extern int tpCheckSourceIsNotInnerClass(void);
extern int tpCheckMoveClassAccessibilities(void);
extern int tpCheckSuperMethodReferencesForDynToSt(void);
extern int tpCheckOuterScopeUsagesForDynToSt(void);
extern int tpCheckMethodReferencesWithApplOnSuperClassForPullUp(void);
extern int tpCheckSuperMethodReferencesForPullUp(void);
extern int tpCheckSuperMethodReferencesAfterPushDown(void);
extern int tpCheckTargetToBeDirectSubOrSupClass(int flag, char *subOrSuper);
extern int tpPullUpFieldLastPreconditions(void);
extern int tpPushDownFieldLastPreconditions(void);

#endif

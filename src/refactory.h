#ifndef REFACTORY_H
#define REFACTORY_H

#include "proto.h"

extern void refactoryAskForReallyContinueConfirmation();
extern void refactoryDisplayResolutionDialog(char *message,int messageType, int continuation);
extern void editorApplyUndos(S_editorUndo *undos, S_editorUndo *until, S_editorUndo **undoundo, int gen);
extern void editorUndoUntil(S_editorUndo *until, S_editorUndo **undoUndo);
extern void mainRefactory(int argc, char **argv);
extern int tpCheckSourceIsNotInnerClass();
extern int tpCheckMoveClassAccessibilities();
extern int tpCheckSuperMethodReferencesForDynToSt();
extern int tpCheckOuterScopeUsagesForDynToSt();
extern int tpCheckMethodReferencesWithApplOnSuperClassForPullUp();
extern int tpCheckSuperMethodReferencesForPullUp();
extern int tpCheckSuperMethodReferencesAfterPushDown();
extern int tpCheckTargetToBeDirectSubOrSupClass(int flag, char *subOrSuper);
extern int tpPullUpFieldLastPreconditions();
extern int tpPushDownFieldLastPreconditions();

#endif

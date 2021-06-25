#include <cgreen/cgreen.h>

#include "filetable.mock"

Id s_javaAnonymousClassName = {"{Anonymous}", NULL, {-1,0,0}};
Id s_javaConstructorName = {"<init>", NULL, {-1,0,0}};
TypeModifier * s_preCreatedTypesTable[MAX_TYPE];
Symbol s_errorSymbol;
char ppmMemory[SIZE_ppmMemory];
char *s_javaLangObjectLinkName="java/lang/Object";
Symbol s_defaultVoidDefinition;

/* Mocks: */
#include "globals.mock"
#include "commons.mock"
#include "options.mock"
#include "classcaster.mock"
#include "classfilereader.mock"
#include "typemodifier.mock"
#include "cxref.mock"
#include "jsemact.mock"
#include "jsltypetab.mock"
#include "memory.mock"
#include "misc.mock"
#include "semact.mock"


Describe(JslSemAct);
BeforeEach(JslSemAct) {}
AfterEach(JslSemAct) {}

Ensure(JslSemAct, something) {
}

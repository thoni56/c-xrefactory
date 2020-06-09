#include <cgreen/cgreen.h>

#include "filetab.mock"

S_fileTab s_fileTab;
Id s_javaAnonymousClassName = {"{Anonymous}", NULL, {-1,0,0}};
Id s_javaConstructorName = {"<init>", NULL, {-1,0,0}};
TypeModifier * s_preCreatedTypesTable[MAX_TYPE];
Symbol s_errorSymbol;
S_options options;        // current options
char ppmMemory[SIZE_ppmMemory];
char *s_javaLangObjectLinkName="java/lang/Object";
Symbol s_defaultVoidDefinition;

/* Mocks: */
#include "classcaster.mock"
#include "classfilereader.mock"
#include "typemodifier.mock"
#include "commons.mock"
#include "cxref.mock"
#include "jsemact.mock"
#include "jsltypetab.mock"
#include "memory.mock"
#include "misc.mock"
#include "semact.mock"
#include "globals.mock"


Describe(JslSemAct);
BeforeEach(JslSemAct) {}
AfterEach(JslSemAct) {}

Ensure(JslSemAct, something) {
}

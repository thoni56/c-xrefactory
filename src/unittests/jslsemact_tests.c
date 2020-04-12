#include <cgreen/cgreen.h>

/* From globals.c */
#include "filetab.h"

S_fileTab s_fileTab;
S_id s_javaAnonymousClassName = {"{Anonymous}", NULL, {-1,0,0}};
S_id s_javaConstructorName = {"<init>", NULL, {-1,0,0}};
S_typeModifier * s_preCreatedTypesTable[MAX_TYPE];
Symbol s_errorSymbol;
S_options s_opt;        // current options
int ppmMemoryi=0;
char ppmMemory[SIZE_ppmMemory];
char *s_javaLangObjectLinkName="java/lang/Object";
Symbol s_defaultVoidDefinition;

/* Mocks: */
#include "cct.mock"
#include "classfilereader.mock"
#include "commons.mock"
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

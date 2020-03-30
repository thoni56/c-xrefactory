#include <cgreen/cgreen.h>

#include "classFileReader.mock"

/* From globals.c */
#include "filetab.h"

S_fileTab s_fileTab;
S_id s_javaAnonymousClassName = {"{Anonymous}", NULL, {-1,0,0}};
S_id s_javaConstructorName = {"<init>", NULL, {-1,0,0}};
S_typeModifier * s_preCrTypesTab[MAX_TYPE];
S_position s_noPos = {-1, 0, 0};
Symbol s_errorSymbol;
S_options s_opt;        // current options
int s_olOriginalFileNumber = -1;
int s_noneFileIndex = -1;
int ppmMemoryi=0;
char ppmMemory[SIZE_ppmMemory];
char *s_javaLangObjectLinkName="java/lang/Object";
Symbol s_defaultVoidDefinition;

/* Mocks: */
#include "jsltypetab.mock"
#include "commons.mock"
#include "jsemact.mock"
#include "semact.mock"
#include "cxref.mock"
#include "misc.mock"
#include "cct.mock"


Describe(JslSemAct);
BeforeEach(JslSemAct) {}
AfterEach(JslSemAct) {}

Ensure(JslSemAct, something) {
}

#include "init.h"

#include "commons.h"
#include "globals.h"
#include "parsers.h"
#include "protocol.h"
#include "misc.h"
#include "enumTxt.h"
#include "symbol.h"
#include "classfilereader.h"
#include "log.h"


static void initTokensFromTab(S_tokenNameIni *tokenTabIni) {
    char *name;
    int token, not_used, i, languages;
    Symbol *pp;

    for(i=0; tokenTabIni[i].name!=NULL; i++) {
        name = tokenTabIni[i].name;
        token = tokenTabIni[i].token;
        languages = tokenTabIni[i].languages;
        s_tokenName[token] = name;
        s_tokenLength[token] = strlen(name);
        if ((isalpha(*name) || *name=='_') && (languages & s_language)) {
            /* looks like a keyword */
            pp = newSymbolAsKeyword(name, name, s_noPos, token);
            fillSymbolBits(&pp->bits, ACCESS_DEFAULT, TypeKeyword, StorageNone);

            log_trace("adding keyword '%s' to symbol table", name);
            symbolTableAdd(s_symbolTable, pp, &not_used);
        }
    }
}

static char *autoDetectJavaVersion(void) {
    int i;
    char *res;
    // O.K. pass jars and look for rt.jar
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[i].fn[0]!=0; i++) {
        if (stringContainsSubstring(s_zipArchiveTable[i].fn, "rt.jar")) {
            // I got it, detect java version
            if (stringContainsSubstring(s_zipArchiveTable[i].fn, "1.4")) {
                res = JAVA_VERSION_1_4;
                goto fini;
            }
        }
    }
    res = JAVA_VERSION_1_3;
 fini:
    return res;
}

void initTokenNameTab(void) {
    char *jv;
    int not_used;
    Symbol *pp;
    static int messageWritten=0;

    if (! s_opt.strictAnsi) {
        initTokensFromTab(s_tokenNameIniTab2);
    }
    jv = s_opt.javaVersion;
    if (strcmp(jv, JAVA_VERSION_AUTO)==0) jv = autoDetectJavaVersion();
    if (s_opt.taskRegime!=RegimeEditServer
        && s_opt.taskRegime!=RegimeGenerate
        && messageWritten==0) {
        if (s_opt.xref2) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"java version == %s", jv);
            ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
        } else {
            fprintf(dumpOut,"java version == %s\n", jv);
        }
        messageWritten=1;
    }
    if (strcmp(jv, JAVA_VERSION_1_4)==0) {
        initTokensFromTab(s_tokenNameIniTab3);
    }
    /* regular tokentab at last, because we wish to have correct names */
    initTokensFromTab(s_tokenNameIniTab);
    /* and add the 'defined' keyword for #if */
    pp = newSymbol("defined", "defined", s_noPos);
    fillSymbolBits(&pp->bits, ACCESS_DEFAULT, TypeDefinedOp, StorageNone);
    symbolTableAdd(s_symbolTable, pp, &not_used);
}

void initJavaTypePCTIConvertIniTab(void) {
    int                         i;
    S_javaTypePCTIConvertIni    *s;
    for (i=0; s_javaTypePCTIConvertIniTab[i].symType != -1; i++) {
        s = &s_javaTypePCTIConvertIniTab[i];
        assert(s->symType >= 0 && s->symType < MAX_TYPE);
        s_javaTypePCTIConvert[s->symType] = s->PCTIndex;
    }
}

void initTypeCharCodeTab(void) {
    int                 i;
    S_typeCharCodeIni   *s;
    for (i=0; s_baseTypeCharCodesIniTab[i].symType != -1; i++) {
        s = &s_baseTypeCharCodesIniTab[i];
        assert(s->symType >= 0 && s->symType < MAX_TYPE);
        s_javaBaseTypeCharCodes[s->symType] = s->code;
        assert(s->code >= 0 && s->code < MAX_CHARS);
        s_javaCharCodeBaseTypes[s->code] = s->symType;
    }
}

void initTypesNamesTab(void) {
    int                 i;
    S_int2StringTab      *s;
    for (i=0; s_typeNamesInitTab[i].i != -1; i++) {
        s = &s_typeNamesInitTab[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        typeEnumName[s->i] = s->s;
    }
}

void initExtractStoragesNameTab(void) {
    int                 i;
    S_int2StringTab      *s;
    for(i=0; i<MAX_STORAGE; i++) s_extractStorageName[i]="";
    for (i=0; s_extractStoragesNamesInitTab[i].i != -1; i++) {
        s = &s_extractStoragesNamesInitTab[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        s_extractStorageName[s->i] = s->s;
    }
}


void initArchaicTypes(void) {
    /* ******* some defaults and built-ins initialisations ********* */

    initTypeModifier(&s_defaultIntModifier, TypeInt);
    fillSymbolWithType(&s_defaultIntDefinition, NULL, NULL, s_noPos, &s_defaultIntModifier);

    initTypeModifier(&s_defaultPackedTypeModifier, TypePackedType);

    initTypeModifier(&s_defaultVoidModifier,TypeVoid);
    fillSymbolWithType(&s_defaultVoidDefinition, NULL, NULL, s_noPos, &s_defaultVoidModifier);

    initTypeModifier(&s_errorModifier, TypeError);
    fillSymbolWithType(&s_errorSymbol, "__ERROR__", "__ERROR__", s_noPos, &s_errorModifier);
    fillSymbolBits(&s_errorSymbol.bits, ACCESS_DEFAULT, TypeError, StorageNone);
}

void initPreCreatedTypes(void) {
    int i,t;

    for(i=0; i<MAX_TYPE; i++) {
        s_preCreatedTypesTable[i] = NULL;
        s_preCrPtr1TypesTab[i] = NULL;
    }
    for(i=0; ; i++) {
        t = s_preCrTypesIniTab[i];
        if (t<0) break;
        /* pre-create X */
        assert(t>=0 && t<MAX_TYPE);
        s_preCreatedTypesTable[t] = newTypeModifier(t, NULL, NULL);
        /* pre-create *X */
        s_preCrPtr1TypesTab[t] = newTypeModifier(TypePointer, NULL, s_preCreatedTypesTable[t]);
        /* pre-create **X */
        s_preCrPtr2TypesTab[t] = newTypeModifier(TypePointer, NULL, s_preCrPtr1TypesTab[t]);
    }
}

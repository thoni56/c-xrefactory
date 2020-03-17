#include "init.h"

#include "commons.h"
#include "globals.h"
#include "parsers.h"
#include "protocol.h"
#include "misc.h"
#include "enumTxt.h"
#include "symbol.h"
#include "strFill.h"


static void initTokensFromTab(S_tokenNameIni *tokenTabIni) {
    char *nn;
    int tok, not_used, i, tlan;
    Symbol *pp;

    for(i=0; tokenTabIni[i].name!=NULL; i++) {
        nn = tokenTabIni[i].name;
        tok = tokenTabIni[i].token;
        tlan = tokenTabIni[i].languages;
        s_tokenName[tok] = nn;
        s_tokenLength[tok] = strlen(nn);
        if ((isalpha(*nn) || *nn=='_') && (tlan & s_language)) {
            /* looks like a keyword */
            pp = newSymbolIsKeyword(nn, nn, s_noPos, tok);
            fillSymbolBits(&pp->bits, ACC_DEFAULT, TypeKeyword, StorageNone);

            /*fprintf(dumpOut,"adding keyword %s to tab %d\n",nn,s_symTab);*/
            symTabAdd(s_symTab,pp,&not_used);
        }
    }
}

static char *autoDetectJavaVersion(void) {
    int i;
    char *res;
    // O.K. pass jars and look for rt.jar
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTab[i].fn[0]!=0; i++) {
        if (stringContainsSubstring(s_zipArchiveTab[i].fn, "rt.jar")) {
            // I got it, detect java version
            if (stringContainsSubstring(s_zipArchiveTab[i].fn, "1.4")) {
                res = JAVA_VERSION_1_4;
                goto fini;
            }
        }
    }
    res = JAVA_VERSION_1_3;
 fini:
    return(res);
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
    fillSymbolBits(&pp->bits, ACC_DEFAULT, TypeDefinedOp, StorageNone);
    symTabAdd(s_symTab, pp, &not_used);
}

#define CHANGE_MODIF_ENTRY(index,modifier) {                \
        int t,tmp;                                          \
        t = s_typeModificationsInit[index].type;            \
        tmp = s_typeModificationsInit[index].mod##modifier; \
        if (tmp>=0) type##modifier##Change[t] = tmp;        \
    }

void initTypeModifiersTabs(void) {
    int i;

    for(i=0; i<MAX_TYPE; i++) {
        typeShortChange[i] = i;
        typeLongChange[i] = i;
        typeSignedChange[i] = i;
        typeUnsignedChange[i] = i;
    }
    for(i=0; s_typeModificationsInit[i].type >= 0; i++) {
        CHANGE_MODIF_ENTRY(i,Short);
        CHANGE_MODIF_ENTRY(i,Long);
        CHANGE_MODIF_ENTRY(i,Signed);
        CHANGE_MODIF_ENTRY(i,Unsigned);
    }
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
    S_intStringTab      *s;
    for (i=0; s_typesNamesInitTab[i].i != -1; i++) {
        s = &s_typesNamesInitTab[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        typesName[s->i] = s->s;
    }
}

void initExtractStoragesNameTab(void) {
    int                 i;
    S_intStringTab      *s;
    for(i=0; i<MAX_STORAGE; i++) s_extractStorageName[i]="";
    for (i=0; s_extractStoragesNamesInitTab[i].i != -1; i++) {
        s = &s_extractStoragesNamesInitTab[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        s_extractStorageName[s->i] = s->s;
    }
}


void initArchaicTypes(void) {
    /* ******* some defaults and built-ins initialisations ********* */

    FILLF_typeModifiers(&s_defaultIntModifier, TypeInt, f, (NULL,NULL), NULL, NULL);
    fillSymbolWithType(&s_defaultIntDefinition, NULL, NULL, s_noPos, &s_defaultIntModifier);

    FILLF_typeModifiers(&s_defaultPackedTypeModifier, TypePackedType, f,
                        (NULL,NULL), NULL, NULL);

    FILLF_typeModifiers(&s_defaultVoidModifier,TypeVoid,f,( NULL,NULL) ,NULL,NULL);
    fillSymbolWithType(&s_defaultVoidDefinition, NULL, NULL, s_noPos,
               &s_defaultVoidModifier);

    FILLF_typeModifiers(&s_errorModifier, TypeError,f,( NULL,NULL) ,NULL,NULL);
    fillSymbolWithType(&s_errorSymbol,"__ERROR__", "__ERROR__", s_noPos, &s_errorModifier);
    fillSymbolBits(&s_errorSymbol.bits, ACC_DEFAULT, TypeError, StorageNone);
}

void initPreCreatedTypes(void) {
    int i,t;
    S_typeModifiers *tt;
    for(i=0; i<MAX_TYPE; i++) {
        s_preCrTypesTab[i] = NULL;
        s_preCrPtr1TypesTab[i] = NULL;
    }
    for(i=0; ; i++) {
        t = s_preCrTypesIniTab[i];
        if (t<0) break;
        /* pre-create X */
        XX_ALLOC(tt, S_typeModifiers);
        FILLF_typeModifiers(tt, t, t, NULL,NULL, NULL);
        assert(t>=0 && t<MAX_TYPE);
        s_preCrTypesTab[t] = tt;
        /* pre-create *X */
        XX_ALLOC(tt, S_typeModifiers);
        FILLF_typeModifiers(tt, TypePointer,t,NULL,NULL, s_preCrTypesTab[t]);
        s_preCrPtr1TypesTab[t] = tt;
        /* pre-create **X */
        XX_ALLOC(tt, S_typeModifiers);
        FILLF_typeModifiers(tt, TypePointer,t,NULL,NULL, s_preCrPtr1TypesTab[t]);
        s_preCrPtr2TypesTab[t] = tt;
    }
}

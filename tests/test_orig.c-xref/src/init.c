/*
	$Revision: 1.4 $
	$Date: 2002/07/31 17:19:58 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "unigram.h"
#include "protocol.h"
//

static void initTokensFromTab(S_tokenNameIni *tokenTabIni) {
	char *nn;
	int tok, ii, i, tlan;
	S_symbol *pp;
	for(i=0; tokenTabIni[i].name!=NULL; i++) {
		nn = tokenTabIni[i].name;
		tok = tokenTabIni[i].token;
		tlan = tokenTabIni[i].languages;
		s_tokenName[tok] = nn;
		s_tokenLength[tok] = strlen(nn);
		if ((isalpha(*nn) || *nn=='_') && (tlan & s_language)) {
			/* looks like a key word */
			XX_ALLOC(pp, S_symbol);
			FILL_symbolBits(&pp->b,0,0, 0,0,0,TypeKeyword,StorageNone,0);
			FILL_symbol(pp,nn,nn,s_noPos,pp->b,keyWordVal,tok,NULL);
			pp->u.keyWordVal = tok;
/*fprintf(dumpOut,"adding keyword %s to tab %d\n",nn,s_symTab);*/
			symTabAdd(s_symTab,pp,&ii);
		}
	}
}

static char *autoDetectJavaVersion() {	
	int i;
	char *res;
	// O.K. pass jars and look for rt.jar
	for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchivTab[i].fn[0]!=0; i++) {
		if (stringContainsSubstring(s_zipArchivTab[i].fn, "rt.jar")) {
			// I got it, detect java version
			if (stringContainsSubstring(s_zipArchivTab[i].fn, "1.4")) {
				res = JAVA_VERSION_1_4;
				goto fini;
			}
		}
	}
	res = JAVA_VERSION_1_3;
 fini:
	return(res);
}

void initTokenNameTab() {
	char 		*nn,*jv;
	int 		tok,ii,i,tlan;
	S_symbol 	*pp;
	static int 	messageWritten=0;
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
	XX_ALLOC(pp, S_symbol);
	FILL_symbolBits(&pp->b,0,0,0,0,0,TypeDefinedOp,StorageNone,0);
	FILL_symbol(pp,"defined","defined",s_noPos,pp->b,type,NULL,NULL);
	symTabAdd(s_symTab,pp,&ii);
}

#define CHANGE_MODIF_ENTRY(index,modifier) {\
	int t,tmp;\
	t = s_typeModificationsInit[index].type;\
	tmp = s_typeModificationsInit[index].mod##modifier;\
	if (tmp>=0) type##modifier##Change[t] = tmp;\
}

void initTypeModifiersTabs() {
	int i,t;
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

void initJavaTypePCTIConvertIniTab() {
	int 						i;
	S_javaTypePCTIConvertIni	*s;
	for (i=0; s_javaTypePCTIConvertIniTab[i].symType != -1; i++) {
		s = &s_javaTypePCTIConvertIniTab[i];
		assert(s->symType >= 0 && s->symType < MAX_TYPE);
		s_javaTypePCTIConvert[s->symType] = s->PCTIndex;
	}
}

void initTypeCharCodeTab() {
	int 				i;
	S_typeCharCodeIni	*s;
	for (i=0; s_baseTypeCharCodesIniTab[i].symType != -1; i++) {
		s = &s_baseTypeCharCodesIniTab[i];
		assert(s->symType >= 0 && s->symType < MAX_TYPE);
		s_javaBaseTypeCharCodes[s->symType] = s->code;
		assert(s->code >= 0 && s->code < MAX_CHARS);
		s_javaCharCodeBaseTypes[s->code] = s->symType;
	}
}

void initTypesNamesTab() {
	int 				i;
	S_intStringTab		*s;
	for (i=0; s_typesNamesInitTab[i].i != -1; i++) {
		s = &s_typesNamesInitTab[i];
		assert(s->i >= 0 && s->i < MAX_TYPE);
		typesName[s->i] = s->s;
	}
}

void initExtractStoragesNameTab() {
	int 				i;
	S_intStringTab		*s;
	for(i=0; i<MAX_STORAGE; i++) s_extractStorageName[i]="";
	for (i=0; s_extractStoragesNamesInitTab[i].i != -1; i++) {
		s = &s_extractStoragesNamesInitTab[i];
		assert(s->i >= 0 && s->i < MAX_TYPE);
		s_extractStorageName[s->i] = s->s;
	}
}


void initArchaicTypes() {
	/* ******* some defaults and built-ins initialisationa ********* */

	FILLF_typeModifiers(&s_defaultIntModifier,TypeInt,f,( NULL,NULL) ,NULL,NULL);
	FILL_symbolBits(&s_defaultIntDefinition.b,0,0,0,0,0,TypeDefault,StorageDefault,0);
	FILL_symbol(&s_defaultIntDefinition,NULL,NULL,s_noPos,
					s_defaultIntDefinition.b,type,&s_defaultIntModifier,NULL);
	s_defaultIntDefinition.u.type = &s_defaultIntModifier;
	FILLF_typeModifiers(&s_defaultPackedTypeModifier,TypePackedType,f,( 
					NULL,NULL) ,NULL,NULL);
	FILLF_typeModifiers(&s_defaultVoidModifier,TypeVoid,f,( NULL,NULL) ,NULL,NULL);
	FILL_symbolBits(&s_defaultVoidDefinition.b,0,0,0,0,0,TypeDefault,StorageDefault,0);
	FILL_symbol(&s_defaultVoidDefinition,NULL,NULL,s_noPos,
			s_defaultVoidDefinition.b,type,&s_defaultVoidModifier,NULL);
	s_defaultVoidDefinition.u.type = &s_defaultVoidModifier;
	FILLF_typeModifiers(&s_errorModifier, TypeError,f,( NULL,NULL) ,NULL,NULL);
	FILL_symbolBits(&s_errorSymbol.b,0,0, 0,0,0,TypeError, StorageNone,0);
	FILL_symbol(&s_errorSymbol,"__ERROR__",
			"__ERROR__",s_noPos,s_errorSymbol.b,type,&s_errorModifier,NULL);
	s_errorSymbol.u.type = &s_errorModifier;
}

void initPreCreatedTypes() {
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



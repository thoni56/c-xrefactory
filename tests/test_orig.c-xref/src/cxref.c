/*
	$Revision: 1.86 $
	$Date: 2002/09/10 18:14:42 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/

#include "protocol.h"
//

#define SORTED_LIST_LESS(tmp,key) (POSITION_LESS((tmp)->p, (key).p))

#if 0    // this would not work because lvalue usage is overwritten over usage
#define SORTED_LIST_NEQ(tmp,key) (\
	POSITION_NEQ((tmp)->p, (key).p) || (tmp)->usage!=(key).usage\
)
#else
#define SORTED_LIST_NEQ(tmp,key) (\
	POSITION_NEQ((tmp)->p, (key).p)\
)
#endif

#define POSITION_MINUS(res,p1,p2) {\
	FILL_position(&(res), (p1).file-(p2).file, (p1).line-(p2).line, (p1).coll-(p2).coll);\
}

#define POSITION_PLUS(res,p1,p2) {\
	FILL_position(&(res), (p1).file+(p2).file, (p1).line+(p2).line, (p1).coll+(p2).coll);\
}

#define CX_TEST_SPACE() {\
	assert(s_opt.taskRegime);\
	if (s_opt.taskRegime==RegimeXref||s_opt.taskRegime==RegimeHtmlGenerate) {\
		if (!(DM_FREE_SPACE(cxMemory,CX_SPACE_RESERVE))) {\
			longjmp(cxmemOverflow,MESS_REFERENCE_OVERFLOW);\
		}\
	}\
}

#define OLCX_USER_RESERVE 30

/* ************************************************************** */

typedef struct olcxTab S_olcxTab;
#define HASH_TAB_TYPE struct olcxTab
#define HASH_ELEM_TYPE S_userOlcx
#define HASH_FUN_PREFIX olcxTab
#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashlist.th"
#include "hashlist.tc"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#undef HASH_FUN
#undef HASH_ELEM_EQUAL

static S_olcxTab s_olcxTab;

static void oloPushByNameIfNothingPushed();

/* *********************************************************************** */

int olcxReferenceInternalLessFunction(S_reference *r1, S_reference *r2) {
	return(SORTED_LIST_LESS(r1, (*r2)));
}

int olSymbolRefItemLess(S_symbolRefItem *s1, S_symbolRefItem *s2) {
	int cmp;
	//& if (s1->vlevel < s2->vlevel) return(1);
	//& else if (s2->vlevel < s1->vlevel) return(0);
	cmp = strcmp(s1->name, s2->name);
	if (cmp < 0) return(1);
	else if (cmp > 0) return(0);
	if (s1->vFunClass < s2->vFunClass) return(1);
	else if (s1->vFunClass > s2->vFunClass) return(0);
	if (s1->vApplClass < s2->vApplClass) return(1);
	else if (s1->vApplClass > s2->vApplClass) return(0);
	if (s1->b.symType < s2->b.symType) return(1);
	else if (s1->b.symType > s2->b.symType) return(0);
	if (s1->b.storage < s2->b.storage) return(1);
	else if (s1->b.storage > s2->b.storage) return(0);
	if (s1->b.category < s2->b.category) return(1);
	else if (s1->b.category > s2->b.category) return(0);
	return(0);
}

static int olSymbolMenuLess(S_olSymbolsMenu *s1, S_olSymbolsMenu *s2) {
	int cmp;
	//& if (s1->vlevel < s2->vlevel) return(1);
	//& else if (s2->vlevel < s1->vlevel) return(0);
	return(olSymbolRefItemLess(&s1->s, &s2->s));
}

S_olSymbolsMenu *olCreateNewMenuItem( 
		S_symbolRefItem *sym, int vApplClass, int vFunCl, S_position *defpos, int defusage,
		int selected, int visible, 
		unsigned ooBits, int olusage, int vlevel 
	) {
	S_olSymbolsMenu *rr;
	int nlen;
	char *nn;
	nlen = strlen(sym->name);
	OLCX_ALLOCC(nn, nlen+1, char);
	strcpy(nn, sym->name);
	OLCX_ALLOC(rr, S_olSymbolsMenu);
	FILLF_olSymbolsMenu(rr, 
						// symbolRefItem
						nn, cxFileHashNumber(nn),
						vApplClass, vFunCl,
						sym->b.symType, sym->b.storage, sym->b.scope,
						sym->b.accessFlags, sym->b.category, 
						sym->b.htmlWasLn,
						NULL, NULL,
						// symbolMenu
						selected, visible, ooBits, olusage, vlevel, 0,0,
						defusage, defpos->file, defpos->line, defpos->coll,
						0, NULL,
						NULL);

	return(rr);
}

S_olSymbolsMenu *olAddBrowsedSymbol(
						S_symbolRefItem *sym, S_olSymbolsMenu **list, 
						int selected, int visible, unsigned ooBits, 
						int olusage, int vlevel, 
						S_position *defpos, int defusage) {
	S_olSymbolsMenu 	*rr, *ll, **place, ddd;
	int					nlen;
	char				*nn;
	FILL_olSymbolsMenu(&ddd, *sym, 0,0,0, olusage, vlevel,0,0, UsageNone,s_noPos,0, NULL, NULL);
	SORTED_LIST_PLACE3(place, S_olSymbolsMenu, (&ddd), list, olSymbolMenuLess);
	rr = *place;
	if (*place==NULL || olSymbolMenuLess(&ddd, *place)) {
		assert(sym);
		rr = olCreateNewMenuItem( sym, sym->vApplClass, sym->vFunClass, defpos, defusage, 
								  selected, visible, ooBits, 
								  olusage, vlevel);
		LIST_CONS(rr,(*place));
//fprintf(dumpOut,":adding browsed symbol %s\n", sym->name);
	}
	return(rr);
}

void renameCollationSymbols(S_olSymbolsMenu *sss) {
	S_olSymbolsMenu 	*ss;
	int 				filei,len,len1;
	char				*nn, *cs;
	assert(sss);
	for(ss=sss; ss!=NULL; ss=ss->next) {
		cs = strchr(ss->s.name, LINK_NAME_COLLATE_SYMBOL);
		if (cs!=NULL && ss->s.b.symType==TypeCppCollate) {
			len = strlen(ss->s.name);
			assert(len>=2);
			OLCX_ALLOCC(nn, len-1, char);
			len1 = cs-ss->s.name;
			strncpy(nn, ss->s.name, len1);
			strcpy(nn+len1, cs+2);
//&fprintf(dumpOut, "renaming %s to %s\n", ss->s.name, nn);
			OLCX_FREE(ss->s.name, len+1);
			ss->s.name = nn;
		}
	}
}


/* *********************************************************************** */


/*
void deleteFromRefList(void *p) {
	S_reference **pp, *ff;
	pp = (S_reference **) p;
	ff = *pp;
	*pp = (*pp)->next;
	CX_FREE(ff);
}
*/


void setIntToZero(void *p) {
	int *pp;
	pp = (int *) p;
	*pp = 0;
}

#ifdef LINEAR_ADD_REFERENCE

   this will not work, DO NOT compie this version with this macro setting

#else
S_reference **addToRefList(	S_reference **list,
							S_usageBits *pusage,				
							S_position *pos,
							int category
						) {
	S_reference *rr, **place;
	S_reference ppp;
	FILL_reference(&ppp, *pusage, *pos, NULL);
	SORTED_LIST_PLACE2(place,S_reference,ppp,list);
	if (*place==NULL || SORTED_LIST_NEQ((*place),ppp) 
			|| s_opt.cxrefs==OLO_EXTRACT) {
		CX_ALLOC(rr, S_reference);
		FILL_reference(rr, *pusage, *pos, NULL);
		LIST_CONS(rr,(*place));
	} else {
		assert(*place);
		(*place)->usg = *pusage;
	}
	return(place);
}
#endif

int isInRefList(S_reference *list,
				S_usageBits *pusage,				
				S_position *pos,
				int category
				) {
	S_reference *rr;
	S_reference ppp;
	FILL_reference(&ppp, *pusage, *pos, NULL);
	SORTED_LIST_FIND2(rr, S_reference, ppp, list);
	if (rr==NULL || SORTED_LIST_NEQ(rr,ppp)) return(0);
	return(1);
}


S_reference *duplicateReference(S_reference *r) {
	// this is used in extract x=x+2; to re-arrange order of references
	// i.e. usage must be first, lValue second.
	S_reference *rr;
	r->usg = s_noUsage;
	CX_ALLOC(rr, S_reference);
	*rr = *r;
	r->next = rr;
	return(rr);
}

static int newRefTabItem(S_refTab *reftab,
						S_symbolRefItem **memb,
						int usage,
						int storage,
						 int scope,
						int category,
						S_symbol *p,
						int vApplClass,
						int vFunClass,
						int *ii
						) {
	int rr;
	S_symbolRefItem ppp;
	FILL_symbolRefItemBits(&ppp.b,p->b.symType,storage,scope,
						   p->b.accessFlags,category,0);
	FILL_symbolRefItem(&ppp,p->linkName,
					   0,                  // cxFileHashNumber(p->linkName),
					   vApplClass,vFunClass,ppp.b,NULL,NULL);
	rr = refTabIsMember(reftab, &ppp, ii, memb);
	if (rr == 0) return(1);
//&	if (p->b.symType == TypeLabel) return(0);
//&	if (p->b.symType == TypeYaccSymbol) return(0);
//&	if (usage != UsageDefined) return(0);
/*following is comented, because it generated new item, for each file of which
  a header was included, solution should be smart link names */
//&	if (p->b.symType == TypeMacro) return(1);
//&	if (p->b.symType == TypeCppIfElse) return(1);
//&	if (p->b.symType == TypeDefault && p->b.storage == StorageStatic
//&		&& p->u.type->m == TypeFunction) return(0);
//&	if (category == CatLocal) return(1); // can I comment it ?
	return(0);
}

static int htmlReferencableSymbol(int scope, int category, S_symbol *p) {
//&	if (p->b.symType==TypeCppIfElse) return(0);	/* takes too much memory */
	if (scope==ScopeFile) return(1);
	if (category==CatGlobal) return(1);
	if (p->b.symType==TypeDefault && p->b.storage==StorageStatic) return(1);
	return(0);
}

static void getSymbolCxrefCategories( 
									S_symbol *p, 
									int *p_category, 
									int *p_scope, 
									int *p_storage 
	) {
	int category, scope, storage;
	category = CatLocal; scope = ScopeAuto; storage=StorageAuto;
	/* default */
	if (p->b.symType==TypeDefault) {
		storage = p->b.storage;
		if (	p->b.storage==StorageExtern
			|| 	p->b.storage==StorageDefault
			|| 	p->b.storage==StorageTypedef
			|| 	p->b.storage==StorageField
			|| 	p->b.storage==StorageMethod
			|| 	p->b.storage==StorageConstructor
			||	p->b.storage==StorageStatic
			) { 
				if (p->linkName[0]==' ' && p->linkName[1]==' ') {
					// a special symbol local linkname
					category = CatLocal;
				} else {
					category = CatGlobal;
				}
				scope = ScopeGlobal;
		}
	}
	/* enumeration constants */
	if (p->b.symType==TypeDefault && p->b.storage==StorageConstant) {
		category = CatGlobal;  scope = ScopeGlobal; storage=StorageExtern;
	}
	/* struct, union, enum */
	if ((p->b.symType==TypeStruct||p->b.symType==TypeUnion||p->b.symType==TypeEnum)){
		category = CatGlobal;  scope = ScopeGlobal; storage=StorageExtern;
	}
	/* macros */
	if (p->b.symType == TypeMacro) {
		category = CatGlobal;  scope = ScopeGlobal; storage=StorageExtern;
	}
	if (p->b.symType == TypeLabel) {
		category = CatLocal; scope = ScopeFile; storage=StorageStatic;
	}
	if (p->b.symType == TypeCppIfElse) {
		category = CatLocal; scope = ScopeFile; storage=StorageStatic;
	}
	if (p->b.symType == TypeCppInclude) {
		category = CatGlobal; scope = ScopeGlobal; storage=StorageExtern;
	}
	if (p->b.symType == TypeCppCollate) {
		category = CatGlobal; scope = ScopeGlobal; storage=StorageExtern;
	}
	if (p->b.symType == TypeYaccSymbol) {
		category = CatLocal; scope = ScopeFile; storage=StorageStatic;
	}
	if (s_opt.taskRegime == RegimeHtmlGenerate) {
		if (p->b.symType == TypeKeyword) {
			category = CatGlobal; scope = ScopeFile; storage=StorageStatic;
			/* global because I dont want them from processed include files */
		}
		if (p->b.symType == TypeComment) {
			category = CatGlobal; scope = ScopeFile; storage=StorageStatic;
		}
		if (p->b.symType == TypeCppAny) {
			category = CatGlobal; scope = ScopeFile; storage=StorageStatic;
		}
		if (p->b.symType == TypeCppIfElse) {
			/* it tooks too much memory on first pass */
			category = CatLocal; scope = ScopeFile; storage=StorageStatic;
		}
	}
	/* JAVA packages */
	if (p->b.symType == TypePackage) {
		category = CatGlobal;  scope = ScopeGlobal; storage=StorageExtern;
	}

	*p_category = category;
	*p_scope = scope;
	*p_storage = storage;
}


static void setClassTreeBaseType(S_classTreeData *ct, S_symbol *p) {
	S_symbol 		*rtcls;
	S_typeModifiers	*tt;
	assert(s_javaObjectSymbol && s_javaObjectSymbol->u.s);
	assert(ct);
//&fprintf(dumpOut,"!looking for result of %s\n",p->linkName);fflush(dumpOut);
	ct->baseClassIndex = s_javaObjectSymbol->u.s->classFile;
	if (p->b.symType == TypeStruct) {
		assert(p->u.s);
		ct->baseClassIndex = p->u.s->classFile;
	} else if (p->b.symType == TypeDefault && p->b.storage!=StorageConstructor) {
		tt = p->u.type;
		assert(tt);
		if (tt->m == TypeFunction) tt = tt->next;
		assert(tt);
		if (tt->m == TypeStruct) {
			rtcls = tt->u.t;
			assert(rtcls!=NULL && rtcls->b.symType==TypeStruct 
				   && rtcls->u.s!=NULL);
//&fprintf(dumpOut,"!resulting class is %s\n",rtcls->linkName);fflush(dumpOut);
			ct->baseClassIndex = rtcls->u.s->classFile;
		}
	}
}

S_reference *addSpecialFieldReference(char *name,int storage, int fnum, 
									  S_position *p, int usage){
	S_symbol 		ss;
	S_symbolBits	bb;
	S_reference 	*res;
	FILL_symbolBits(&bb,0,0, ACC_DEFAULT,0,0,TypeDefault, storage,0);
	FILL_symbol(&ss, name, name, *p, bb,type,NULL, NULL);
	res = addCxReference(&ss, p, usage, fnum, fnum);
	return(res);
}

static int isEnclosingClass(int enclosedClass, int enclosingClass) {
	int i;
	int cc,slow;
	for(i = slow = enclosedClass, cc=0; 
		i!=s_noneFileIndex && i!=-1; 
		i=s_fileTab.tab[i]->directEnclosingInstance) {
		assert(s_fileTab.tab[i]);
		if (i == enclosingClass) return(1);
		// this loop looks very suspect, I prefer to put here a loop check
		if (cc==0) {
			cc = !cc; 
		} else {
			assert(slow != i);
			slow = s_fileTab.tab[slow]->directEnclosingInstance;
		}
	}
	return(0);
}

int isStrictlyEnclosingClass(int enclosedClass, int enclosingClass) {
	if (enclosedClass == enclosingClass) return(0);
	return(isEnclosingClass(enclosedClass, enclosingClass));
}

// TODO, all this stuff should be done differently!
void changeFieldRefUsages(S_symbolRefItem  *ri, void  *rrcd) {
	S_referencesChangeData 		*rcd;
	S_symbolRefItem				ddd;
	S_reference					*rr;
	rcd = (S_referencesChangeData*) rrcd;
	FILL_symbolRefItemBits(&ddd.b,TypeDefault,StorageField,
						   ScopeFile,ACC_DEFAULT, rcd->category ,0);
	FILL_symbolRefItem(&ddd,rcd->linkName,
					   cxFileHashNumber(rcd->linkName),
					   s_noneFileIndex, s_noneFileIndex, ddd.b,NULL,NULL);
	if (itIsSameCxSymbol(ri, &ddd)) {
//&sprintf(tmpBuff, "checking %s <-> %s, %d,%d", ri->name, rcd->linkName, rcd->cxMemBegin,rcd->cxMemEnd);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
		for(rr = ri->refs; rr!=NULL; rr=rr->next) {
//&sprintf(tmpBuff, "checking %d,%d %d,%d,%d", rr->p.file, rcd->fnum, rr, rcd->cxMemBegin,rcd->cxMemEnd);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
			if (rr->p.file == rcd->fnum &&  /* I think it is used only for Java */
				DM_IS_BETWEEN(cxMemory,rr,rcd->cxMemBegin,rcd->cxMemEnd)) {
//&sprintf(tmpBuff, "checking %d,%d %d,%d,%d", rr->p.file, rcd->fnum, rr, rcd->cxMemBegin,rcd->cxMemEnd);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
				switch(rr->usg.base) {
				case UsageMaybeThis: 
					assert(rcd->cclass->u.s);
					if (isEnclosingClass(rcd->cclass->u.s->classFile, ri->vFunClass)) {
						rr->usg.base = UsageMaybeThisInClassOrMethod;
					}
					break;
				case UsageMaybeQualifiedThis: 
					assert(rcd->cclass->u.s);
					if (isEnclosingClass(rcd->cclass->u.s->classFile, ri->vFunClass)) {
						rr->usg.base = UsageMaybeQualifThisInClassOrMethod;
					}
					break;
				case UsageNotFQType: rr->usg.base = UsageNotFQTypeInClassOrMethod;
//&sprintf(tmpBuff, "reseting %d:%d:%d at %d", rr->p.file, rr->p.line, rr->p.coll, rr);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
					break;
				case UsageNotFQField: rr->usg.base = UsageNotFQFieldInClassOrMethod;
					break;
				case UsageNonExpandableNotFQTName: rr->usg.base = UsageNonExpandableNotFQTNameInClassOrMethod;
					break;
				case UsageLastUseless: rr->usg.base = UsageLastUselessInClassOrMethod;
					break;
				case UsageOtherUseless: 
					//& rr->usg.base = UsageOtherUselessInMethod;
					break;
				case UsageLastUselessInClassOrMethod:
				case UsageNotFQFieldInClassOrMethod:
				case UsageNotFQTypeInClassOrMethod:
				case UsageMaybeQualifThisInClassOrMethod:
				case UsageMaybeThisInClassOrMethod:
					// do not care if it is yet requalified
					break;				
				default:
					// some problems with UsageNone ??
#if ZERO
					sprintf(tmpBuff,"unexpected usage %s restricted to class/method",
							usagesName[rr->usg.base]);
					error(ERR_INTERNAL, tmpBuff);
#endif
					break;
				}
			}
		}
	}
}

void changeMethodReferencesUsages(char *linkName, int category, int fnum, 
								  S_symbol *cclass){
	S_referencesChangeData rr;
	FILL_referencesChangeData(&rr, linkName, fnum, cclass, category, 
							  s_cps.cxMemiAtMethodBeginning,
							  s_cps.cxMemiAtMethodEnd);
	refTabMap2(&s_cxrefTab, changeFieldRefUsages, &rr);
}

void changeClassReferencesUsages(char *linkName, int category, int fnum, 
								 S_symbol *cclass){
	S_referencesChangeData rr;
	FILL_referencesChangeData(&rr, linkName, fnum, cclass, category, 
							  s_cps.cxMemiAtClassBeginning,
							  s_cps.cxMemiAtClassEnd);
	refTabMap2(&s_cxrefTab, changeFieldRefUsages, &rr);
}

S_reference * getDefinitionRef(S_reference *rr) {
	S_reference *r;
	S_reference *res;
	res = NULL;
	for(r=rr; r!=NULL; r=r->next) {
		if (r->usg.base==UsageDefined || r->usg.base==UsageOLBestFitDefined) {
			res = r;
		}
		if (r->usg.base==UsageDeclared && res==NULL) res = r;
	}
	return(res);
}

// used only with OLO_GET_SYMBOL_TYPE;
static void setOlSymbolTypeForPrint(S_symbol *p) {
	int 			size, len;
	S_typeModifiers *tt;
	size = COMPLETION_STRING_SIZE;
	s_olSymbolType[0]=0;
	s_olSymbolClassType[0]=0;
	if (p->b.symType == TypeDefault) {
		tt = p->u.type;
		if (tt!=NULL && tt->m==TypeFunction) tt = tt->next;
		typeSPrint(s_olSymbolType, &size, tt, "", ' ', 0, 1, LONG_NAME, NULL);
		if (tt->m == TypeStruct && tt->u.t!=NULL) {
			strcpy(s_olSymbolClassType, tt->u.t->linkName);
			InternalCheck(strlen(s_olSymbolClassType)+1 < COMPLETION_STRING_SIZE);
		}
		// remove pending spaces
		len = strlen(s_olSymbolType);
		while (len > 0 && s_olSymbolType[len-1] == ' ') len--;
		s_olSymbolType[len]=0;
	} 
}

static void setOlAvailableRefactorings(S_symbol *p, S_olSymbolsMenu *mmi, int usage) {
	char *opt;
	if (strcmp(p->linkName, LINK_NAME_UNIMPORTED_QUALIFIED_ITEM)==0) {
		s_availableRefactorings[PPC_AVR_ADD_TO_IMPORT].available = 1;		
		return;
	}
	switch (p->b.symType) {
	case TypePackage:
		s_availableRefactorings[PPC_AVR_RENAME_PACKAGE].available = 1;
		CX_ALLOCC(opt, strlen(mmi->s.name)+1, char);
		strcpy(opt, mmi->s.name);
		javaDotifyFileName(opt);
		s_availableRefactorings[PPC_AVR_RENAME_PACKAGE].option = opt;
		break;
	case TypeStruct:
		if (LANGUAGE(LAN_JAVA)) {
			if (IS_DEFINITION_USAGE(usage)) {
				s_availableRefactorings[PPC_AVR_RENAME_CLASS].available = 1;
				s_availableRefactorings[PPC_AVR_MOVE_CLASS].available = 1;
				s_availableRefactorings[PPC_AVR_MOVE_CLASS_TO_NEW_FILE].available = 1;
				//& s_availableRefactorings[PPC_AVR_MOVE_ALL_CLASSES_TO_NEW_FILE].available = 1;
				s_availableRefactorings[PPC_AVR_EXPAND_NAMES].available = 1;
				s_availableRefactorings[PPC_AVR_REDUCE_NAMES].available = 1;
			}
		} else {
			s_availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = 1;
		}
		break;
	case TypeMacroArg:
		s_availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = 1;
		break;
	case TypeLabel:
		s_availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = 1;
		break;
	case TypeMacro:
		s_availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = 1;
		s_availableRefactorings[PPC_AVR_ADD_PARAMETER].available = 1;
		s_availableRefactorings[PPC_AVR_ADD_PARAMETER].option = "macro";
		s_availableRefactorings[PPC_AVR_DEL_PARAMETER].available = 1;
		s_availableRefactorings[PPC_AVR_DEL_PARAMETER].option = "macro";
		s_availableRefactorings[PPC_AVR_MOVE_PARAMETER].available = 1;
		s_availableRefactorings[PPC_AVR_MOVE_PARAMETER].option = "macro";
		break;
	default:
		if (p->b.storage != StorageConstructor) {
			s_availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = 1;
		}
		if (p->u.type->m == TypeFunction || p->u.type->m == TypeMacro) {
			s_availableRefactorings[PPC_AVR_ADD_PARAMETER].available = 1;
			s_availableRefactorings[PPC_AVR_DEL_PARAMETER].available = 1;
			s_availableRefactorings[PPC_AVR_MOVE_PARAMETER].available = 1;
		}
		if (p->b.storage == StorageField) {
			if (IS_DEFINITION_USAGE(usage)) {
				if (p->b.accessFlags & ACC_STATIC) {
					s_availableRefactorings[PPC_AVR_MOVE_STATIC_FIELD].available = 1;
				} else {
					//& TODO! restrict this better
					s_availableRefactorings[PPC_AVR_PULL_UP_FIELD].available = 1;
					s_availableRefactorings[PPC_AVR_PUSH_DOWN_FIELD].available = 1;
				}
				s_availableRefactorings[PPC_AVR_MOVE_FIELD].available = 1;
				s_availableRefactorings[PPC_AVR_ENCAPSULATE_FIELD].available = 1;
				s_availableRefactorings[PPC_AVR_SELF_ENCAPSULATE_FIELD].available = 1;
			}
		}
		if (p->b.storage == StorageMethod || p->b.storage == StorageConstructor) {
			if (IS_DEFINITION_USAGE(usage)) {
				//&s_availableRefactorings[PPC_AVR_EXPAND_NAMES].available = 1;
				//&s_availableRefactorings[PPC_AVR_REDUCE_NAMES].available = 1;
				if (p->b.accessFlags & ACC_STATIC) {
					s_availableRefactorings[PPC_AVR_MOVE_STATIC_METHOD].available = 1;
					if (p->b.storage == StorageMethod) {
						s_availableRefactorings[PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC].available = 1;
					}
				} else {
					if (p->b.storage == StorageMethod) {
						// TODO! some restrictions
						s_availableRefactorings[PPC_AVR_PULL_UP_METHOD].available = 1;
						s_availableRefactorings[PPC_AVR_PUSH_DOWN_METHOD].available = 1;
						s_availableRefactorings[PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC].available = 1;
					}
				}
			}
		}
		break;
	}
}

static void olGetAvailableRefactorings() {
	int					i, count;
	char				*additional;
	count = 0;
	for(i=0; i<MAX_AVAILABLE_REFACTORINGS; i++) {
		count += (s_availableRefactorings[i].available!=0);
	}
	if (count==0) s_availableRefactorings[PPC_AVR_SET_MOVE_TARGET].available = 1;
	if (s_opt.editor == ED_EMACS) {
		s_availableRefactorings[PPC_AVR_UNDO].available = 1;
#if ZERO
		if (LANGUAGE(LAN_JAVA)) {
			s_availableRefactorings[PPC_AVR_ADD_ALL_POSSIBLE_IMPORTS].available = 1;
		}
#endif
	}
	if (s_opt.olCursorPos != s_opt.olMarkPos) {
		// region selected, TODO!!! some more prechecks for extract method
		if (LANGUAGE(LAN_JAVA)) {
			s_availableRefactorings[PPC_AVR_EXTRACT_METHOD].available = 1;
		} else {
			s_availableRefactorings[PPC_AVR_EXTRACT_FUNCTION].available = 1;
		}
		if (! LANGUAGE(LAN_JAVA)) {
			s_availableRefactorings[PPC_AVR_EXTRACT_MACRO].available = 1;
		}
	}
	if (! s_opt.xref2) {
		fprintf(ccOut,"* refactoring list not available in C-xrefactory-I");
		return;
	}
	ppcGenRecordBegin(PPC_AVAILABLE_REFACTORINGS);
	for(i=0; i<MAX_AVAILABLE_REFACTORINGS; i++) {
		if (s_availableRefactorings[i].available) {
			ppcGenNumericRecord(PPC_INT_VALUE, i, s_availableRefactorings[i].option, "\n");
		}
	}
	ppcGenRecordEnd(PPC_AVAILABLE_REFACTORINGS);
}


static int olcxOnlyParseNoPushing(opt) {
	return(
		opt==OLO_GLOBAL_UNUSED
		|| opt==OLO_LOCAL_UNUSED
		);
}


/* ********************************************************************* */
/* ********************************************************************* */
/* default vappClass == vFunClass == s_noneFileIndex !!!!!!!             */
/*                                                                       */
S_reference * addCxReferenceNew(S_symbol *p, S_position *pos, S_usageBits *usageb,
							 int vFunCl, int vApplCl) {
	int 				ii,mm,category,scope,storage,defusage;
	char 				*ss;
	S_reference 		rr, **place;
	S_position 			*defpos;
	S_symbolRefItem 	*pp,*memb, ppp;
	S_olSymbolsMenu 	*mmi;
	S_refTab 			*reftab;
	int					usage;
	usage = usageb->base;
	// do not record references on prescanning
	// this is because of cxMem overflow during prescanning (for ex. with -html)
	if (s_javaPreScanOnly) return(NULL);
	if (p->linkName == NULL) return(NULL);
	if (* p->linkName == 0) return(NULL);
	if (p == &s_errorSymbol || p->b.symType==TypeError) return(NULL);
	if (pos->file == s_noneFileIndex) return(NULL);
	assert(pos->file<MAX_FILES);
	assert(s_fileTab.tab[pos->file]);
	if (p->b.symType==TypeDefault && p->b.storage==StorageConstant) {
		if (s_opt.no_ref_enumerator) return(NULL);
	}
	/* typedefs */
	if (p->b.symType==TypeDefault && p->b.storage==StorageTypedef) {
		if (s_opt.no_ref_typedef) return(NULL);
	}
	/* struct, union, enum */
	if ((p->b.symType==TypeStruct||p->b.symType==TypeUnion||p->b.symType==TypeEnum)){
		if (s_opt.no_ref_typedef) return(NULL);
	}
	/* macros */
	if (p->b.symType == TypeMacro) {
		if (s_opt.no_ref_macro) return(NULL);
	}
	if (p->b.symType==TypeDefault) {
		if (p->b.record && s_opt.no_ref_records) return(NULL);
	}

	getSymbolCxrefCategories( p, &category, &scope, &storage);
	if (scope == ScopeAuto && s_opt.no_ref_locals) return(NULL);

//&fprintf(dumpOut,":adding reference on %s(%d,%d) at %d,%d,%d  (%s) (%s) (%s) \n",p->linkName,vFunCl,vApplCl, pos->file,pos->line,pos->coll,refCategoriesName[category],usagesName[usage],storagesName[p->b.storage]);
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime == RegimeEditServer) {
		if (s_opt.cxrefs == OLO_EXTRACT) {
			if (s_input_file_number != cFile.lb.cb.fileNumber) return(NULL);
		} else {
			if (category==CatGlobal && p->b.symType!=TypeCppInclude && s_opt.cxrefs!=OLO_TAG_SEARCH) {
				// do not load references if not the currently edited file
				if (s_olOriginalFileNumber!=pos->file
					&& s_opt.noIncludeRefs) return(NULL);
				// do not load references if current file is an 
				// included header, they will be reloaded from ref file
//&fprintf(dumpOut,"%s comm %d\n", s_fileTab.tab[pos->file]->name, s_fileTab.tab[pos->file]->b.commandLineEntered);
			}
		}
	}
	if (s_opt.taskRegime == RegimeGenerate) return(NULL);
	if (s_opt.taskRegime == RegimeXref) {
		if (category == CatLocal) return(NULL); /* dont cxref local symbols */
		if (s_fileTab.tab[pos->file]->b.cxLoading==0) return(NULL);
	}
	if (s_opt.taskRegime == RegimeHtmlGenerate) {
//&		scope = ScopeFile;	/* do not forget any reference */
//&		if (! htmlReferencableSymbol(scope, category, p)) return(NULL);
//&		if (s_fileTab.tab[pos->file]->b.cxLoading==0) return(NULL);
		if (s_fileTab.tab[pos->file]->b.cxLoading==0&&category==CatGlobal) return(NULL);
		if (s_fileTab.tab[pos->file]->b.cxLoaded
			&&p->b.symType==TypeCppIfElse) return(NULL);
	}
	reftab = &s_cxrefTab;
	FILL_symbolRefItemBits(&ppp.b,p->b.symType,storage,scope,
						   p->b.accessFlags,category,0);
	FILL_symbolRefItem(&ppp,p->linkName,
					   0,                  // cxFileHashNumber(p->linkName),
					   vApplCl,vFunCl,ppp.b,NULL,NULL);
	if (s_opt.taskRegime==RegimeEditServer && s_opt.cxrefs==OLO_TAG_SEARCH && s_opt.tagSearchSpecif==TSS_FULL_SEARCH) {
		FILL_usageBits(&rr.usg, usage, 0, 0);
		FILL_reference(&rr, rr.usg, *pos, NULL);
		searchSymbolCheckReference(&ppp, &rr);
		return(NULL);
	}
	mm = refTabIsMember(reftab, &ppp, &ii, &memb);
	//& was if (newRefTabItem(reftab,&memb,usage,storage,scope,category,p,vApplCl,vFunCl,&ii));
	if (mm==0) {
//&fprintf(dumpOut,"allocating %s\n",p->linkName);fflush(dumpOut);
		CX_ALLOC(pp, S_symbolRefItem);
		CX_ALLOCC(ss, strlen(p->linkName)+1, char);
		strcpy(ss, p->linkName);
		FILL_symbolRefItemBits(&pp->b,p->b.symType,storage,scope,
							   p->b.accessFlags,category,0);
		FILL_symbolRefItem(pp,ss,cxFileHashNumber(ss),
						   vApplCl,vFunCl,pp->b,NULL,NULL);
		refTabSet(reftab, pp, ii);
		memb = pp;
	} else {
		// at least reset some maybe new informations
		// sometimes classes were added from directory listing, 
		// without knowing if it is an interface or not
		memb->b.accessFlags |= p->b.accessFlags;
	}
/*	category = reftab->category; */
	place = addToRefList(&memb->refs,usageb,pos,category);
//&fprintf(dumpOut,"checking %s(%d),%d,%d <-> %s(%d),%d,%d == %d(%d), usage == %d, %s\n", s_fileTab.tab[s_cxRefPos.file]->name, s_cxRefPos.file, s_cxRefPos.line, s_cxRefPos.coll, s_fileTab.tab[pos->file]->name, pos->file, pos->line, pos->coll, memcmp(&s_cxRefPos, pos, sizeof(S_position)), POSITION_EQ(s_cxRefPos, *pos), usage, p->linkName);

	if (s_opt.taskRegime == RegimeEditServer 
		&& POSITION_EQ(s_cxRefPos, *pos)
		&& usage<UsageMaxOLUsages) {
		if (p->linkName[0] == ' ') {  // special symbols for internal use!
			if (strcmp(p->linkName, LINK_NAME_UNIMPORTED_QUALIFIED_ITEM)==0) {
				if (s_opt.cxrefs == OLO_GET_AVAILABLE_REFACTORINGS) {
					setOlAvailableRefactorings(p, NULL, usage);
				}
			}
		} else {
			/* an on - line cxref action ?*/
//&fprintf(dumpOut,"!got it %s !!!!!!!\n", memb->name);
			s_olstringServed = 1;		/* olstring will be served */
			s_olstringUsage = usage;
			assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
			olSetCallerPosition(pos);
			defpos = &s_noPos;
			defusage = s_noUsage.base;
			if (p->b.symType==TypeMacro && ! s_opt.exactPositionResolve) {
				// a hack for macros
				defpos = &p->pos; 
				defusage = UsageDefined;
			}
//&if (defpos->file!=s_noneFileIndex) fprintf(dumpOut,":getting definition position of %s at line %d\n", p->name, defpos->line);
			if (! olcxOnlyParseNoPushing(s_opt.cxrefs)) {
				mmi = olAddBrowsedSymbol(memb,&s_olcxCurrentUser->browserStack.top->hkSelectedSym,
										 1,1,0,usage,0, defpos, defusage);
				// hack added for EncapsulateField
				// to determine whether there is yet definiton of getter/setter
				if (IS_DEFINITION_USAGE(usage)) {
					mmi->defpos = *pos;
					mmi->defUsage = usage;
				}
				if (s_opt.cxrefs == OLO_CLASS_TREE 
					&& (LANGUAGE(LAN_JAVA) || LANGUAGE(LAN_CCC))) {
					setClassTreeBaseType(&s_olcxCurrentUser->ct, p);
				}
				if (s_opt.cxrefs == OLO_GET_SYMBOL_TYPE) {
					setOlSymbolTypeForPrint(p);
				}
				if (s_opt.cxrefs == OLO_GET_AVAILABLE_REFACTORINGS) {
					setOlAvailableRefactorings(p, mmi, usage);
				}
			}
		}
	}

	CX_TEST_SPACE();
	assert(place);
/*&fprintf(dumpOut,":returning %x == %s %s:%d\n",*place,usagesName[(*place)->usg.base],s_fileTab.tab[(*place)->p.file]->name,(*place)->p.line);&*/
	return(*place);
}

S_reference * addCxReference(S_symbol *p, S_position *pos, int usage, int vFunCl, int vApplCl) {
	S_usageBits	ub;
	FILL_usageBits(&ub, usage, MIN_REQUIRED_ACCESS, 0);
	return(addCxReferenceNew(p, pos, &ub, vFunCl, vApplCl));
}

void addTrivialCxReference(char *name,int symType,int storage,S_position *pos,int usage) {
	S_symbol 		ss;
	S_symbolBits	bb;
	FILL_symbolBits(&bb,0,0, 0,0,0,symType,storage,0);
	FILL_symbol(&ss, name, name, *pos, bb,type,NULL, NULL);
	addCxReference(&ss, pos, usage,s_noneFileIndex, s_noneFileIndex);
}

void addClassTreeHierarchyReference(int fnum, S_position *p, int usage) {
	addSpecialFieldReference(LINK_NAME_CLASS_TREE_ITEM,StorageMethod, 
							 fnum, p, usage);
}

void addCfClassTreeHierarchyRef(int fnum, int usage) {
	S_position 		p;
	FILL_position(&p, fnum, 0, 0);
	addClassTreeHierarchyReference(fnum, &p, usage);
}

/* ***************************************************************** */

#if ZERO
static void markRelatedClasses(int cl, int originCl) {
	S_chReference *p;
//&fprintf(dumpOut,"relation mark %s\n",s_fileTab.tab[cl]->name);
	assert(s_fileTab.tab[cl]);
	for(p=s_fileTab.tab[cl]->sups; p!=NULL; p=p->next) {
		assert(s_fileTab.tab[p->clas]);
		s_fileTab.tab[p->clas]->b.classIsRelatedTo = originCl;
		if (s_fileTab.tab[p->clas]->sups!=NULL) {
			markRelatedClasses(p->clas, originCl);
		}
	}
}
static int searchRelatedClass(int cl, int originCl) {
	S_chReference *p;
//&fprintf(dumpOut,"relation mark %s\n",s_fileTab.tab[cl]->name);
	assert(s_fileTab.tab[cl]);
	for(p=s_fileTab.tab[cl]->sups; p!=NULL; p=p->next) {
		assert(s_fileTab.tab[p->clas]);
		if (s_fileTab.tab[p->clas]->b.classIsRelatedTo==originCl) return(1);
		if (s_fileTab.tab[p->clas]->sups!=NULL) {
			if (searchRelatedClass(p->clas, originCl)) return(1);
		}
	}
	return(0);
}
int isRelatedClass(int cl1, int cl2) {
	S_chReference *p;
/*&fprintf(dumpOut,"searching relation betwee %s\t%s\n",s_fileTab.tab[cl1]->name,s_fileTab.tab[cl2]->name);&*/
	if (cl1 == cl2) return(1);
	markRelatedClasses(cl1, cl1);
	if (searchRelatedClass(cl2, cl1)) return(1);
	return(0);
}
#endif

int isSmallerOrEqClassR(int inf, int sup, int level) {
	S_chReference *p;
	int			smallerLevel;
	assert(level>0);
//&fprintf(dumpOut,"testing hierarchy of %s\n<\t%s\n",s_fileTab.tab[inf]->name,s_fileTab.tab[sup]->name);
	if (inf == sup) return(level);
	assert(s_fileTab.tab[inf]);
	for(p=s_fileTab.tab[inf]->sups; p!=NULL; p=p->next) {
		if (p->clas == sup) {
//&fprintf(dumpOut,"return 1\n");
			return(level+1);
		}
	}
	for(p=s_fileTab.tab[inf]->sups; p!=NULL; p=p->next) {
		smallerLevel = isSmallerOrEqClassR(p->clas, sup, level+1);
		if (smallerLevel) {
//&fprintf(dumpOut,"return 1\n");
			return(smallerLevel);
		}
	}
//&fprintf(dumpOut,"return 0\n");
	return(0);
}

int isSmallerOrEqClass(int inf, int sup) {
	return(isSmallerOrEqClassR(inf, sup, 1)!=0);
}

#define OLCX_FREE_REFERENCE(r) {\
	RLM_FREE(olcxMemory, (r), sizeof(S_reference));\
}


void olcxFreeReferences(S_reference *r) {
	S_reference *tmp;
	while (r!=NULL) {
		tmp = r->next;
		OLCX_FREE_REFERENCE(r);
		r = tmp;
	}
}

static void olcxFreeCompletion(S_olCompletion *r) {
	int nlen;
	RLM_FREE(olcxMemory, r->name, strlen(r->name)+1);
	if (r->fullName!=NULL) RLM_FREE(olcxMemory, r->fullName, strlen(r->fullName)+1);
	if (r->vclass!=NULL) RLM_FREE(olcxMemory, r->vclass, strlen(r->vclass)+1);
	if (r->cat == CatGlobal) {
		assert(r->sym.name);
		nlen = strlen(r->sym.name);
		RLM_FREE(olcxMemory, r->sym.name, nlen+1);
	}
	RLM_FREE(olcxMemory, r, sizeof(S_olCompletion));
}


static void olcxFreeCompletions( S_olCompletion *r ) {
	S_olCompletion *tmp;
	int nlen;
	while (r!=NULL) {
		tmp = r->next;
		olcxFreeCompletion(r);
		r = tmp;
	}
}

S_olSymbolsMenu *olcxFreeSymbolMenuItem(S_olSymbolsMenu *ll) {
	int nlen;
	S_olSymbolsMenu *tt;
	nlen = strlen(ll->s.name);
	OLCX_FREE(ll->s.name, nlen+1);
	olcxFreeReferences(ll->s.refs);
	tt = ll->next;
	OLCX_FREE(ll, sizeof(*ll));
	ll = tt;
	return(ll);
}


void olcxFreeResolutionMenu( S_olSymbolsMenu *sym ) {
	int 			nlen;
	S_olSymbolsMenu *ll,*tt;
	S_reference		*r,*rr;
	ll = sym; 
	while (ll!=NULL) {
		ll = olcxFreeSymbolMenuItem(ll);
	}
}

static void deleteOlcxRefs(S_olcxReferences **rrefs, S_olcxReferencesStack *stack) {
	S_olcxReferences 	*refs;
	refs = *rrefs;
/*fprintf(ccOut,": pop 2\n"); fflush(ccOut);*/
	olcxFreeReferences(refs->r);
	olcxFreeCompletions(refs->cpls);
	olcxFreeResolutionMenu(refs->hkSelectedSym);
	olcxFreeResolutionMenu(refs->menuSym);
/*fprintf(ccOut,": pop 3\n"); fflush(ccOut);*/
	// if deleting second entry point, update it
	if (refs==stack->top) {
		stack->top = refs->previous;
	}
	// this is useless, but one never knows
	if (refs==stack->root) {
		stack->root = refs->previous;
	}
	*rrefs = refs->previous;
	RLM_FREE(olcxMemory, refs, sizeof(S_olcxReferences));
}

#define CHECK_AND_SET_OLDEST(stack) {\
	if ((stack)->root!=NULL) {\
		/* do never free the very first item, start by second */\
		for(refs= &((stack)->root->previous); *refs!=NULL; refs= &(*refs)->previous) {\
			if (oldestt > (*refs)->atime) {\
				oldestt = (*refs)->atime;\
				oldest = refs;\
	            oldestStack = (stack);\
			}\
		}\
	}\
}

// TODO!!! should free completions in priority!
void freeOldestOlcx() {
	int 					i;
	S_userOlcx				*user;
	S_olcxReferences		**refs,**oldest;
	time_t 					oldestt;
	S_olcxReferencesStack 	*oldestStack;
	oldestt = s_fileProcessStartTime; oldest=NULL; oldestStack=NULL;
	if (s_ropt.refactoringRegime != RegimeRefactory) {
		for(i=0; i<OLCX_TAB_SIZE; i++) {
			user = s_olcxTab.tab[i];
			if (user!=NULL) {
				CHECK_AND_SET_OLDEST(&user->browserStack);
				CHECK_AND_SET_OLDEST(&user->completionsStack);
				CHECK_AND_SET_OLDEST(&user->retrieverStack);
			}
		}
	}
	if (oldestt == s_fileProcessStartTime || oldest == NULL) {
		fatalError(ERR_ST, "olcxMemory memory overflow, please try again.", XREF_EXIT_ERR);
	} else {
		assert(oldest!=NULL && oldestStack!=NULL);
		deleteOlcxRefs(oldest, oldestStack);
	}
}

int olcxFreeOldCompletionItems(S_olcxReferencesStack *stack) {
	S_olcxReferences **ss;
	int i;
	ss = &stack->top;
	if (*ss == NULL) return(0);
	for(i=1; i<MAX_COMPLETIONS_HISTORY_DEEP; i++) {
		ss = &(*ss)->previous;
		if (*ss == NULL) return(0);
	}
	deleteOlcxRefs(ss, stack);
	return(1);
}

void olcxInit() {
	int i;
	void * uu[OLCX_USER_RESERVE];
	RLM_INIT(olcxMemory);
	RLM_SOFT_ALLOCC(olcxMemory, s_olcxTab.tab, OLCX_TAB_SIZE, S_userOlcx *);
	//CHECK_FREE(s_olcxTab.tab);		// do not report non-freeing of olcxtable
	olcxTabNAInit(&s_olcxTab, OLCX_TAB_SIZE);
	/* reserve place for some users */
	for(i=0; i<OLCX_USER_RESERVE; i++) OLCX_ALLOC(uu[i], S_userOlcx);
	for(i=0; i<OLCX_USER_RESERVE; i++) RLM_FREE(olcxMemory, uu[i], sizeof(S_userOlcx));
}

S_userOlcx *olcxSetCurrentUser(char *user) {
	S_userOlcx 	dd,*memb;
	int 		ii,sz;
	char 		*nn;
	FILLF_userOlcx(&dd, user, NULL, NULL,NULL,NULL,NULL,NULL,s_noneFileIndex, NULL, NULL);
	if (! olcxTabIsMember(&s_olcxTab, &dd, &ii, &memb)) {
		// I have changed it to FT, so it never invokes freeing of OLCX
		FT_ALLOC(memb, S_userOlcx);
		sz = strlen(user)+1;
		if (sz < sizeof(void*)) sz = sizeof(void*);
		FT_ALLOCC(nn, sz, char); // why this is in ftMem ?, some pb with free
		strcpy(nn, user);
		FILLF_userOlcx(memb, nn, NULL, NULL, NULL, NULL,  NULL, NULL, s_noneFileIndex, NULL, NULL);
		olcxTabAdd(&s_olcxTab, memb, &ii);
	}
	s_olcxCurrentUser = memb;
	return(memb);
}

static void setRefSuffix(S_olcxReferences *refs) {
	strncpy(refs->refsuffix, s_opt.olcxRefSuffix, MAX_OLCX_SUFF_SIZE-1);
	refs->refsuffix[MAX_OLCX_SUFF_SIZE-1]=0;
	refs->act = refs->r;
}

static void olcxFreePopedStackItems(S_olcxReferencesStack *stack) {
	assert(stack);
	// delete all after top
	while (stack->root != stack->top) {
//&fprintf(dumpOut,":freeing %s\n", stack->root->hkSelectedSym->s.name);
		deleteOlcxRefs(&stack->root, stack);
	}
}

void olcxPushEmptyStackItem(S_olcxReferencesStack *stack) {
	S_olcxReferences *res, *rr;
	olcxFreePopedStackItems(stack);
	OLCX_ALLOC(res, S_olcxReferences);	/* first alloc, to avoid surprise */
	FILL_olcxReferences(res, NULL, NULL, s_opt.cxrefs, s_language,
						s_fileProcessStartTime, s_noPos, NULL, NULL, NULL,
						DEFAULT_MENU_FILTER_LEVEL, DEFAULT_REFS_FILTER_LEVEL,
						stack->top);
	setRefSuffix(res);
	stack->top = stack->root = res;
}

static int olcxVirtualyUsageAdequate(int vApplCl, int vFunCl, 
								int olUsage, int olApplCl, int olFunCl) {
	int res;
	res = 0;
//&fprintf(dumpOut,"\n:checking %s\n%s\n%s\n<->\n%s\n%s\n",usagesName[olUsage],s_fileTab.tab[olFunCl]->name,s_fileTab.tab[olApplCl]->name,s_fileTab.tab[vFunCl]->name,s_fileTab.tab[vApplCl]->name); fflush(dumpOut);
	if (IS_DEFINITION_OR_DECL_USAGE(olUsage)) {
		if (vFunCl == olFunCl) res = 1;
		if (isSmallerOrEqClass(olFunCl, vApplCl)) res = 1;
	} else {
//&		if (vApplCl==vFunCl) { // only classes with definitions are considered
			if (vApplCl == olFunCl) res = 1;
//&			if (vFunCl == olFunCl) res = 1;
			if (isSmallerOrEqClass(vApplCl, olApplCl)) res = 1;
//&		}
	}
//&fprintf(dumpOut,"result is %d\n",res);fflush(dumpOut);
	return(res);
}

S_reference *olcxAddReferenceNoUsageCheck(S_reference **rlist, S_reference *ref, int bestMatchFlag) {
	S_reference **place, *rr;
	rr = NULL;
	SORTED_LIST_PLACE2(place,S_reference, *ref, rlist);
	if (*place==NULL || SORTED_LIST_NEQ(*place,*ref)) {
		OLCX_ALLOC(rr, S_reference);
		*rr = *ref;
		if (LANGUAGE(LAN_JAVA)) {
			if (ref->usg.base==UsageDefined &&	bestMatchFlag) {
				rr->usg.base = UsageOLBestFitDefined;
			}
		}
		LIST_CONS(rr,(*place));
//&fprintf(dumpOut,"olcx adding %s %s:%d:%d\n",usagesName[ref->usg.base], s_fileTab.tab[ref->p.file]->name,ref->p.line,ref->p.coll); fflush(dumpOut);
	}	
	return(rr);
}


S_reference *olcxAddReference(S_reference **rlist,S_reference *ref,int bestMatchFlag) {
	S_reference **place,*rr;
//&sprintf(tmpBuff,"checking ref %s %s:%d:%d at %d\n",usagesName[ref->usg.base], simpleFileName(s_fileTab.tab[ref->p.file]->name),ref->p.line,ref->p.coll, ref); ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
	if (! OL_VIEWABLE_REFS(ref)) return(NULL); // no regular on-line refs
	rr = olcxAddReferenceNoUsageCheck(rlist, ref, bestMatchFlag);
	return(rr);
}

static S_reference *olcxCopyReference(S_reference *ref) {
	S_reference *rr;
	OLCX_ALLOC(rr, S_reference);
	*rr = *ref;
	rr->next = NULL;
	return(rr);
}

void olcxAppendReference(S_reference *ref, S_olcxReferences *refs) {
	S_reference *rr;
	rr = olcxCopyReference(ref);
	LIST_APPEND(S_reference, refs->r, rr);
//&fprintf(dumpOut,"olcx appending %s %s:%d:%d\n",usagesName[ref->usg.base], s_fileTab.tab[ref->p.file]->name,ref->p.line,ref->p.coll); fflush(dumpOut);
}

/* fnum is file number of which references are added, can be ANY_FILE
*/
void olcxAddReferences(S_reference *list, S_reference **dlist,
					   int fnum, int bestMatchFlag) {
	S_reference *revlist,*tmp;
	/* from now, you should add it to macros as REVERSE_LIST_MAP() */
	revlist = NULL;
	while (list!=NULL) {
		tmp = list->next; list->next = revlist;
		revlist = list;   list = tmp;
	}
	list = revlist;
	revlist = NULL;
	while (list!=NULL) {
		if (fnum==ANY_FILE || fnum==list->p.file) {
			olcxAddReference(dlist, list, bestMatchFlag);
		}
		tmp = list->next; list->next = revlist;
		revlist = list;   list = tmp;
	}
	list = revlist;
}

void olcxAddReferenceToOlSymbolsMenu(S_olSymbolsMenu  *cms, S_reference *rr, 
						  int bestFitTlag) {
	int bestfit;
	S_reference *added;
	added = olcxAddReference(&cms->s.refs, rr, bestFitTlag);
	if (rr->usg.base == UsageClassTreeDefinition) cms->defpos = rr->p;
	if (added!=NULL) {
		if (IS_DEFINITION_OR_DECL_USAGE(rr->usg.base)) {
			if (rr->usg.base==UsageDefined && POSITION_EQ(rr->p, cms->defpos)) {
				added->usg.base = UsageOLBestFitDefined;
			}
			if (rr->usg.base < cms->defUsage) {
				cms->defUsage = rr->usg.base;
				cms->defpos = rr->p;
			}
			cms->defRefn ++;
		} else {
			cms->refn ++;
		}
	}
}

static void olcxAddReferencesToOlSymbolsMenu(S_olSymbolsMenu  *cms, 
											 S_reference *rlist,
											 int bestFitFlag
	) {
	S_reference *rr;
	for(rr=rlist; rr!=NULL; rr=rr->next) {
		olcxAddReferenceToOlSymbolsMenu(cms, rr, bestFitFlag);
	}	
}

void generateOnlineCxref(	S_position *p, 
							char *commandString,
							int usage,
							char *suffix,
							char *suffix2
	) {
	if (s_opt.xref2) {
		ppcGenGotoPositionRecord(p);
	} else {
		fprintf(ccOut,"%s%s#*+*#%d %d :%c%s%s ;;\n", commandString,
				getRealFileNameStatic(s_fileTab.tab[p->file]->name), 
				p->line, p->coll, refCharCode(usage),
				suffix, suffix2);
	}
}

#define IS_COMPLETION_COMMAND(comm) (\
	(comm==OLO_COMPLETION \
	 || comm==OLO_SEARCH \
	 || comm==OLO_CGOTO \
	 /*&|| comm==OLO_TAG_SEARCH &*/\
	)\
)

#define OLCX_MOVE_INIT(olcxuser,refs,checkFlag) {\
	assert(olcxuser);\
	if (	s_opt.cxrefs==OLO_COMPLETION || s_opt.cxrefs==OLO_CSELECT\
		||	s_opt.cxrefs==OLO_CGOTO || s_opt.cxrefs==OLO_CBROWSE\
		/*& || s_opt.cxrefs==OLO_SEARCH &*/\
		||	s_opt.cxrefs==OLO_TAG_SEARCH) {\
	    /*&refs = olcxuser->browserStack.top;&*/\
		/*&while (refs!=NULL && !IS_COMPLETION_COMMAND(refs->command)) refs=refs->previous;&*/\
        refs = olcxuser->completionsStack.top;\
	} else {\
    	refs = olcxuser->browserStack.top;\
		/*&while (refs!=NULL && IS_COMPLETION_COMMAND(refs->command)) refs=refs->previous;&*/\
	}\
	if (checkFlag==CHECK_NULL && refs == NULL) {\
		if (s_opt.xref2) {\
			ppcGenRecordWithNumeric(PPC_BOTTOM_WARNING, PPCA_BEEP, 1, "Empty stack","\n");\
		} else {\
			fprintf(ccOut, "=");\
		}\
		return; /* tricky return from function calling this macro !!!!! */\
	}\
}

static void olcxRenameInit() {
	S_olcxReferences *refs;
	S_reference *rr,*rrd;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
//	setRefSuffix(refs);
	refs->act = refs->r;
	generateOnlineCxref(&refs->act->p, COLCX_GOTO_REFERENCE, 
						refs->act->usg.base, refs->refsuffix, "");
}


int olcxListLessFunction(S_reference *r1, S_reference *r2) {
	int fc;
	char *s1,*s2;
	s1 = simpleFileName(s_fileTab.tab[r1->p.file]->name);
	s2 = simpleFileName(s_fileTab.tab[r2->p.file]->name);
	fc=strcmp(s1, s2);
	if (fc<0) return(1);
	if (fc>0) return(0);
	if (r1->p.file < r2->p.file) return(1);
	if (r1->p.file > r2->p.file) return(0);
	if (r1->p.line < r2->p.line) return(1);
	if (r1->p.line > r2->p.line) return(0);
	if (r1->p.coll < r2->p.coll) return(1);
	if (r1->p.coll > r2->p.coll) return(0);
	return(0);
}

#define UsageImportantInOrder(r1,r2) (\
  r1->usg.base==UsageDefined || r1->usg.base==UsageDeclared\
  || r1->usg.base==UsageOLBestFitDefined\
  || r2->usg.base==UsageDefined || r2->usg.base==UsageDeclared\
  || r2->usg.base==UsageOLBestFitDefined\
)

int olcxPushLessFunction(S_reference *r1, S_reference *r2) {
	int fc;
	char *s1,*s2;
	if (UsageImportantInOrder(r1,r2)) {
		// in definition, declaration usage is important
		if (r1->usg.base < r2->usg.base) return(1);
		if (r1->usg.base > r2->usg.base) return(0);
	}
	return(olcxListLessFunction(r1, r2));
}


#if ZERO
static int olcxFastPushLessFunction(S_reference *r1, S_reference *r2) {
	char *s1,*s2;
	if (UsageImportantInOrder(r1,r2)) {
		// in definition, declaration usage is important
		if (r1->usg.base < r2->usg.base) return(1);
		if (r1->usg.base > r2->usg.base) return(0);
	}
	if (r1->p.file < r2->p.file) return(1);
	if (r1->p.file > r2->p.file) return(0);
	if (r1->p.line < r2->p.line) return(1);
	if (r1->p.line > r2->p.line) return(0);
	if (r1->p.coll < r2->p.coll) return(1);
	if (r1->p.coll > r2->p.coll) return(0);
	return(0);
}
#endif

static void olcxNaturalReorder(S_olcxReferences *refs) {
	LIST_MERGE_SORT(S_reference, refs->r, olcxPushLessFunction);
}

static void olcxGenNoReferenceSignal() {
	if (s_opt.xref2) {
		ppcGenRecord(PPC_BOTTOM_INFORMATION, "No reference", "\n");
	} else {
		fprintf(ccOut, "_");
	}
}

static void olcxOrderRefsAndGotoFirst() {
	S_olcxReferences *refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
//&	setRefSuffix(refs);
	LIST_MERGE_SORT(S_reference, refs->r, olcxListLessFunction);
	refs->act = refs->r;
	if (refs->r != NULL) {
		generateOnlineCxref(&refs->act->p, COLCX_GOTO_REFERENCE, 
							refs->act->usg.base, refs->refsuffix, "");
	} else {
		olcxGenNoReferenceSignal();
	}
}

// references has to be ordered according internal file numbers order !!!!
static int olcxSetCurrentRefsOnCaller( S_olcxReferences *refs ) {
	S_reference *rr;
	for(rr=refs->r; rr!=NULL; rr=rr->next){
//&fprintf(dumpOut,"checking %d %d %d to %d %d %d\n",rr->p.file, rr->p.line,rr->p.coll, refs->cpos.file,  refs->cpos.line,  refs->cpos.coll);
		if (! POSITION_LESS(rr->p, refs->cpos)) break; 
	}
	// it should never be NULL, but one never knows
	if (rr == NULL) {
		refs->act = refs->r;
		return(0);
	} else {
		refs->act = rr;
		return(1);
	}
}

char *getJavaDocUrl_st(S_symbolRefItem *rr) {
	static char res[MAX_HTML_REF_LEN];
	char *tt;
	int len = MAX_HTML_REF_LEN;
	res[0] = 0;
	if (rr->b.symType == TypeDefault) {
		if (rr->vFunClass==s_noneFileIndex) {
			tt = strchr(rr->name, '.');
			if (tt==NULL) {
				sprintf(res,"packages.html#xrefproblem");
			} else {
				len = tt - rr->name;
				strncpy(res, rr->name, len);
				sprintf(res+len, ".html#");
				len = strlen(res);
				linkNamePrettyPrint(res+len, tt+1, 
										MAX_HTML_REF_LEN-len, LONG_NAME);
			} 
		} else {
			javaGetClassNameFromFileNum(rr->vFunClass, res, KEEP_SLASHES);
			for(tt=res; *tt; tt++) if (*tt=='$') *tt = '.';
			len = strlen(res);
			sprintf(res+len, ".html");
			len += strlen(res+len);
			if (strcmp(rr->name, LINK_NAME_CLASS_TREE_ITEM)!=0) {
				sprintf(res+len, "#");
				len += strlen(res+len);
				linkNamePrettyPrint(res+len, rr->name, 
									MAX_HTML_REF_LEN-len, LONG_NAME);
			}
		}
	} else if (rr->b.symType == TypeStruct) {
		sprintf(res,"%s.html",rr->name);
	} else if (rr->b.symType == TypePackage) {
		sprintf(res,"%s/package-tree.html",rr->name);
	}
	InternalCheck(strlen(res)<MAX_HTML_REF_LEN-1);
	return(res);
}

int htmlJdkDocAvailableForUrl(char *ss){
	char		ttt[MAX_FILE_NAME_SIZE];
	char		*cp;
	int 		ind, nlen;
	cp = s_opt.htmlJdkDocAvailable;
	while (*cp!=0) {
		for(ind=0; 
			cp[ind]!=0 && cp[ind]!=CLASS_PATH_SEPARATOR && cp[ind]!=':' ;
			ind++) {
				ttt[ind]=cp[ind];
				if (cp[ind]=='.') ttt[ind] = '/';
		}
		ttt[ind]=0;
//&fprintf(dumpOut,"testing %s <-> %s (%d)\n", ttt, ss, ind);
		if (strncmp(ttt, ss, ind)==0) return(1);
		cp += ind;
		if (*cp == CLASS_PATH_SEPARATOR || *cp == ':') cp++;
	}
	return(0);
}

static int olcxGenHtmlFileWithIndirectLink(char *ofname, char *url) {
	FILE *of;
	of = fopen(ofname, "w");
	if (of == NULL) {
		fprintf(ccOut,"* ** Can't open temporary file %s\n", ofname);	
		return(0);
	}
	fprintf(of, "<html><head>");
	if (s_opt.urlAutoRedirect || strchr(url,' ')==NULL) {
		fprintf(of, "<META http-equiv=\"refresh\" ");
		fprintf(of, "content=\"0;URL=%s\">\n", url);
	}
	fprintf(of, "</head><body>\n");
	if (! (s_opt.urlAutoRedirect || strchr(url,' ')==NULL)) {
		fprintf(of, "Please follow the link: ");
	}
	fprintf(of, "<A HREF=\"%s\">", url);
	fprintf(of, "%s", url);
	fprintf(of, "</A>\n");
	fprintf(of, "</body></html>");
	fclose(of);
	return(1);
}

static char *getExpandedLocalJavaDocFile_st(char *expandedPath, char *prefix, char *tmpfname) {
	static char 	fullurl[MAX_FILE_NAME_SIZE];
	char 			fullfname[MAX_FILE_NAME_SIZE];
	char			*s;
	struct stat 	st;
	int				cplen;
	JavaMapOnPaths(expandedPath, {
		cplen = strlen(currentPath);
		if (cplen>0 && currentPath[cplen-1]==SLASH) {
			if (prefix == NULL) {
				sprintf(fullurl, "%s%s", currentPath, tmpfname);
			} else {
				sprintf(fullurl, "%sapi%c%s", currentPath, SLASH, tmpfname);
			}
		} else {
			if (prefix == NULL) {
				sprintf(fullurl, "%s%c%s", currentPath, SLASH, tmpfname);
			} else {
				sprintf(fullurl, "%s%capi%c%s", currentPath, SLASH, SLASH, tmpfname);
			}
		}
		strcpy(fullfname, fullurl);
		if ((s=strchr(fullfname, '#'))!=NULL) *s = 0;
		if (stat(fullfname, &st)==0) return(fullurl);
	});
	return(NULL);
}

char *getLocalJavaDocFile_st(char *fileUrl) {
	char tmpfname[MAX_FILE_NAME_SIZE];
	static char wcJavaDocPath[MAX_OPTION_LEN];
	char *ss, *res;
	if (s_opt.javaDocPath==NULL) return(NULL);
	strcpy(tmpfname, fileUrl);	
	for(ss=tmpfname; *ss; ss++) if (*ss == '/') *ss = SLASH;
	expandWildCharactersInPaths(s_opt.javaDocPath, wcJavaDocPath, MAX_OPTION_LEN);
	res = getExpandedLocalJavaDocFile_st(wcJavaDocPath, NULL, tmpfname);
	// O.K. try once more time with 'api' prefixed
	if (res == NULL) {
		res = getExpandedLocalJavaDocFile_st(wcJavaDocPath, "api", tmpfname);
	}
	return(res);
}

static void unBackslashifyUrl(char *url) {
	char *ss;
#if defined (__WIN32__) || defined (__OS2__)		/*SBD*/
	for(ss=url; *ss; ss++) {
		if (*ss=='\\') *ss='/';
	}
#endif												/*SBD*/
}

char *getFullUrlOfJavaDoc_st(char *fileUrl) {
	static char fullUrl[MAX_CX_SYMBOL_SIZE];
	char *ss;
	ss = getLocalJavaDocFile_st(fileUrl);
	if (ss!=NULL) {
		sprintf(fullUrl,"file:///%s", ss);
	} else {
		sprintf(fullUrl, "%s/%s", s_opt.htmlJdkDocUrl, fileUrl);
	}
	// replace backslashes under windows by slashes
	// maybe this should be on option?
	unBackslashifyUrl(fullUrl);
	return(fullUrl);
}

static int olcxBrowseSymbolInJavaDoc( S_symbolRefItem *rr ) {
	char *url, *tt, *tmd, *lfn, *furl;
	char tmpfname[MAX_FILE_NAME_SIZE];
	char theUrl[MAX_FILE_NAME_SIZE];
	int rrr;
	url = getJavaDocUrl_st(rr);
	lfn = getLocalJavaDocFile_st(url);
	if (lfn==NULL && (s_opt.htmlJdkDocUrl==NULL || ! htmlJdkDocAvailableForUrl(url))) return(0);
	if (! s_opt.urlGenTemporaryFile) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_BROWSE_URL, getFullUrlOfJavaDoc_st(url), "\n");
		} else {
			fprintf(ccOut,"~%s\n", getFullUrlOfJavaDoc_st(url));	
		}
	} else {
#ifdef __WIN32__		/*SBD*/
		tmd = getenv("TEMP");
		assert(tmd);
		sprintf(tmpfname, "%s/xrefjdoc.html", tmd);
		unBackslashifyUrl(tmpfname);
#else 					/*SBD*/
		tmd = getenv("LOGNAME");
		assert(tmd);
		sprintf(tmpfname, "/tmp/%sxref.html", tmd);
#endif					/*SBD*/
		rrr = olcxGenHtmlFileWithIndirectLink(tmpfname, getFullUrlOfJavaDoc_st(url));
		if (rrr) {
			sprintf(theUrl, "file:///%s", tmpfname);
			InternalCheck(strlen(theUrl)<MAX_FILE_NAME_SIZE-1);
			if (s_opt.xref2) {
				ppcGenRecord(PPC_BROWSE_URL, theUrl, "\n");
			} else {
				fprintf(ccOut,"~%s\n", theUrl);	
			}
		}
	}
	return(1);
}

static int checkTheJavaDocBrowsing( S_olcxReferences *refs ) {
	S_olSymbolsMenu *mm;
	int res;
	res = 0;
	// this was in commentary, why?
	// [16/2/2003 putting it back, because it tries to browse something
	// when definition not found in C language
	if (LANGUAGE(LAN_JAVA)) {
		for(mm=refs->menuSym; mm!=NULL; mm=mm->next) {
			if (mm->visible && mm->selected) {
				res = olcxBrowseSymbolInJavaDoc(&mm->s);
				if (res) goto fini;
			}
		}
    }
 fini:
	return(res);
}


static void orderRefsAndGotoDefinition(S_olcxReferences *refs, int afterMenuFlag) {
	S_olSymbolsMenu *mm;
	int res;
	olcxNaturalReorder(refs);
	if (refs->r == NULL) {
		refs->act = refs->r;
		if (afterMenuFlag==PUSH_AFTER_MENU) res=0;
		else res = checkTheJavaDocBrowsing(refs);
		if (res==0) {
			olcxGenNoReferenceSignal();
		}
	} else if (refs->r->usg.base<=UsageDeclared) {
		refs->act = refs->r;
		generateOnlineCxref(&refs->act->p, COLCX_GOTO_REFERENCE, 
							refs->act->usg.base, refs->refsuffix, "");
	} else {
		if (afterMenuFlag==PUSH_AFTER_MENU) res=0;
		else res = checkTheJavaDocBrowsing(refs);
		if (res==0) {
			if (s_opt.xref2) {
				ppcGenDefinitionNotFoundWarning();
			} else {
				fprintf(ccOut,"*** Definition reference not found **");
			}
		}
	}
}

static void olcxOrderRefsAndGotoDefinition(int afterMenuFlag) {
	S_olcxReferences *refs;
	S_olSymbolsMenu *mm;
	int res;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
//&	setRefSuffix(refs);
	orderRefsAndGotoDefinition(refs, afterMenuFlag);
}

static void skipNLines(FILE **cofile, int n) {
	int c;
	for(; n>0; n--) {
		while ((c=fgetc(*cofile)) != '\n' && c!=EOF) ;
		if (c==EOF) {
			*cofile = NULL;
			return;
		}
	}
}

#define GetBufChar(cch, bbb) {\
	if ((bbb)->cc >= (bbb)->fin) {\
		if ((bbb)->isAtEOF || getCharBuf(bbb) == 0) {\
			cch = EOF;\
			(bbb)->isAtEOF = 1;\
		} else {\
			cch = * ((unsigned char*)(bbb)->cc); (bbb)->cc ++;\
		}\
	} else {\
		cch = * ((unsigned char*)(bbb)->cc); (bbb)->cc++;\
	}\
/*fprintf(dumpOut,"getting char *%x < %x == '0x%x'\n",ccc,ffin,cch);fflush(dumpOut);*/\
}

#define GetFileChar(ch,cp,bbb) {\
	if (ch=='\n') {(cp)->line++; (cp)->coll=0;}\
	else (cp)->coll++;\
	GetBufChar(ch, bbb);\
}

int refCharCode(int usage) {
	switch (usage) {
	case UsageOLBestFitDefined:	return('!');
	case UsageDefined:	return('*');
	case UsageDeclared:	return('+');
	case UsageLvalUsed:	return(',');
	case UsageAddrUsed:	return('.');
		/* some specials for refactorings now */
	case UsageNotFQFieldInClassOrMethod:	return('.');
		// Usage Constructor definition is for move class to not expand
		// and move constructor definition references
	case UsageConstructorDefinition:		return('-');
	default:			return(' ');
	}
	assert(0);
}

static char s_crefListLine[MAX_REF_LIST_LINE_LEN+5];
static int s_crefListLinei = 0;

static void passSourcePutChar(int c, FILE *ff) {
	if (s_opt.xref2) {
		if (s_crefListLinei < MAX_REF_LIST_LINE_LEN) {
			s_crefListLine[s_crefListLinei++] = c;
			s_crefListLine[s_crefListLinei] = 0;
		} else {
			strcpy(s_crefListLine + s_crefListLinei, "...");
		}
	} else {
		if (s_opt.taskRegime == RegimeHtmlGenerate) htmlPutChar(ff,c);
		else fputc(c,ff);
	}
}

static void printKawaRefRecord(FILE *off, int type, char *rec) {
	int len;
	len = strlen(rec);
	fprintf(off, " %d%c%s", len, type, rec);
}

#define LISTABLE_USAGE(rr, usages, usageFilter) (\
  usages==USAGE_ANY || usages==rr->usg.base \
  || (usages==USAGE_FILTER && rr->usg.base<usageFilter) \
)

static void linePosProcess(	FILE *off,
							char *ofname,
							int usages,
							int usageFilter, // only if usages==USAGE_FILTER
							char *fname, 
							S_reference **rrr, 
							S_position *callerp,
							S_position *cp,
							int *cch,
							S_charBuf *cxfBuf
						) {
	int 			ch, pendingRefFlag, line, linerefn;
	S_reference 	*rr,*r;
	char			*fn;
	rr = *rrr;
	ch = *cch;
	fn = simpleFileName(getRealFileNameStatic(fname));
//&	fn = getRealFileNameStatic(fname);
	r = NULL; pendingRefFlag = 0;
	line = 0; linerefn = 0; 
	s_crefListLinei = 0;
	s_crefListLine[s_crefListLinei] = 0;
	do {
		if (LISTABLE_USAGE(rr, usages, usageFilter)) {
			if (r==NULL || r->usg.base > rr->usg.base)  r = rr;
			if (s_opt.taskRegime!=RegimeHtmlGenerate) {
				if (pendingRefFlag) {
					if (! s_opt.xref2) fprintf(off,"\n");
				}
				if (s_opt.xref2) {
					if (! pendingRefFlag) {
						sprintf(s_crefListLine+s_crefListLinei, "%s:%d:", fn, rr->p.line);
						s_crefListLinei += strlen(s_crefListLine+s_crefListLinei);
					}
				} else {
					if (POSITION_NEQ(*callerp, rr->p)) fprintf(off, " ");
					else fprintf(off, ">");
					fprintf(off,"%c%s:%d:",refCharCode(rr->usg.base),fn,
							rr->p.line);
				}
				line = rr->p.line;
				linerefn ++;
				pendingRefFlag = 1;
			}
		}
		rr=rr->next;
	} while (rr!=NULL && ((rr->p.file == cp->file && rr->p.line == cp->line)
		|| (rr->usg.base>UsageMaxOLUsages)));
	if (r!=NULL) {
		if (! cxfBuf->isAtEOF) {
			while (ch!='\n' && (! cxfBuf->isAtEOF)) {
				passSourcePutChar(ch,off);
				GetFileChar(ch, cp, cxfBuf);
			}
		}
		if (! s_opt.xref2) passSourcePutChar('\n',off);
	}
	if (s_opt.xref2 && s_crefListLinei!=0) {
		ppcIndentOffset();
		fprintf(off, "<%s %s=%d %s=%d>%s</%s>\n", 
				PPC_SRC_LINE, PPCA_REFN, linerefn,
				PPCA_LEN, strlen(s_crefListLine), 
				s_crefListLine, PPC_SRC_LINE);
	}
	*rrr = rr;
	*cch = ch;
}

static S_reference *passNonPrintableRefsForFile(S_reference *r,
												int fnum,
												int usages, int usageFilter
	) {
	S_reference *rr;
	for(rr=r; rr!=NULL && rr->p.file == fnum; rr=rr->next) {
		if (LISTABLE_USAGE(rr, usages, usageFilter)) return(rr);
	}
	return(rr);
}

static void passRefsThroughSourceFile(S_reference **rrr, S_position *callerp,
									FILE *off, 
									  char *ofname, int usages, int usageFilter) {
	S_reference 		*rr,*oldrr;
	int					n,ch,fnum;
	FILE				*cofile;
	S_editorBuffer		*ebuf;
	char				*cofileName;
	S_position			cp;
	S_charBuf 			cxfBuf;
	rr = *rrr;
	if (rr==NULL) goto fin;
	fnum = rr->p.file;
	assert(s_fileTab.tab[fnum]);
	cofileName = s_fileTab.tab[fnum]->name;
	rr = passNonPrintableRefsForFile(rr, fnum, usages, usageFilter);
	if (rr==NULL || rr->p.file != fnum) goto fin;
	if (s_opt.referenceListWithoutSource) {
		ebuf = NULL;
	} else {
		ebuf = editorFindFile(cofileName);
		//&cofile = fopen(cofileName, "r");
		//&FILL_charBuf(&cxfBuf, cxfBuf.a, cxfBuf.a, cofile, 0, -1, 0, 0, 0, 0,INPUT_DIRECT,s_defaultZStream);
		if (ebuf==NULL) {
			if (s_opt.xref2) {
				sprintf(tmpBuff, "file %s not accessible", cofileName);
				error(ERR_ST, tmpBuff);
			} else {
				fprintf(off,"  !!! file %s is not accessible: ", cofileName);
			}
			//&error(ERR_CANT_OPEN, cofileName);
		}
	}
	ch = ' ';
	if (ebuf==NULL) {
		cxfBuf.isAtEOF = 1;
	} else {
		FILL_charBuf(&cxfBuf, ebuf->a.text, ebuf->a.text+ebuf->a.bufferSize, NULL, ebuf->a.bufferSize, s_noneFileIndex, 0, ebuf->a.text, 0, 0,INPUT_DIRECT, s_defaultZStream);
		GetFileChar(ch, &cp, &cxfBuf);
	}
	FILL_position(&cp, rr->p.file, 1, 0);
	oldrr=NULL;
	while (rr!=NULL && rr->p.file==cp.file && rr->p.line>=cp.line) {
		assert(oldrr!=rr); oldrr=rr;	// because it is a dangerous loop
		while ((! cxfBuf.isAtEOF) && cp.line<rr->p.line) {
			while (ch!='\n' && ch!=EOF) GetBufChar(ch, &cxfBuf);
			GetFileChar(ch, &cp, &cxfBuf);
		}
		linePosProcess(off, ofname, usages, usageFilter, cofileName, 
					   &rr, callerp, &cp, &ch, &cxfBuf);
	}
	//&if (cofile != NULL) fclose(cofile);
 fin:
	*rrr = rr;
}

/* ******************************************************************** */

void olcxDumpSelectionMenu(S_olSymbolsMenu *menu) {
	S_olSymbolsMenu *ss;
	for(ss=menu; ss!=NULL; ss=ss->next) {
		fprintf(dumpOut,">> %d/%d %s %s %s %d\n", ss->defRefn, ss->refn, ss->s.name, simpleFileName(s_fileTab.tab[ss->s.vFunClass]->name), simpleFileName(s_fileTab.tab[ss->s.vApplClass]->name), ss->outOnLine);
	}
	fprintf(dumpOut,"\n\n");
}

int ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2) {
	if ((oo1&OOC_PROFILE_MASK) < (oo2&OOC_PROFILE_MASK)) {
		return(0);
	}
	if ((oo1&OOC_VIRTUAL_MASK) < (oo2&OOC_VIRTUAL_MASK)) {
		return(0);
	}
	return(1);
}

void olcxPrintClassTree(S_olSymbolsMenu *sss) {
	if (s_opt.xref2) {
		ppcGenRecordBegin(PPC_DISPLAY_CLASS_TREE);
	} else {
		fprintf(ccOut, "<");
	}
	readOneAppropReferenceFile(NULL, s_cxScanFunTabForClassHierarchy);
	htmlGenGlobRefLists(sss, ccOut, "__NO_HTML_FILE_NAME!__");
	if (s_opt.xref2) ppcGenRecordEnd(PPC_DISPLAY_CLASS_TREE);
}

void olcxPrintSelectionMenu(S_olSymbolsMenu *sss) {
	if (s_opt.xref2) {
		ppcGenRecordBegin(PPC_SYMBOL_RESOLUTION);
	} else {
		fprintf(ccOut, ">");
	}
	if (sss!=NULL) {
		readOneAppropReferenceFile(NULL, s_cxScanFunTabForClassHierarchy);
		htmlGenGlobRefLists(sss, ccOut, "__NO_HTML_FILE_NAME!__");
	}
	if (s_opt.xref2) {
		ppcGenRecordEnd(PPC_SYMBOL_RESOLUTION);
	} else {
		if (s_opt.cxrefs==OLO_RENAME || s_opt.cxrefs==OLO_ARG_MANIP || s_opt.cxrefs==OLO_ENCAPSULATE) {
			if (LANGUAGE(LAN_JAVA)) {
				fprintf(ccOut, "-![Warning] It is highly recommended to process the whole hierarchy of related classes at once. Unselection of any class of applications above (and its exclusion from refactoring process) may cause changes in your program behavior. Press <return> to continue.\n");
			} else {
				fprintf(ccOut, "-![Warning] It is highly recommended to process all symbols at once. Unselection of any symbols and its exclusion from refactoring process may cause changes in your program behavior. Press <return> to continue.\n");
			}
		}
		if (s_opt.cxrefs==OLO_VIRTUAL2STATIC_PUSH) {
			fprintf(ccOut, "-![Warning] If you see this message it is highly probable that turning this virtual method into static will not be behaviour preserving! This refactoring is behaviour preserving only if the method does not use mechanism of virtual invocations. On this screen you should select the application classes which are refering to the method which will become static. If you can't unambiguously determine those references do not continue in this refactoring!\n");
		}
		if (s_opt.cxrefs==OLO_SAFETY_CHECK2) {
			if (LANGUAGE(LAN_JAVA)) {
				fprintf(ccOut, "-![Warning] There are differences between original class hierarchy and the new one, those name clashes may cause that the refactoring will not be behavior preserving!\n");
			} else {
				fprintf(ccOut, "-![Error] There is differences between original and new symbols referenced at this position. The difference is due to name clashes and may cause changes in the behaviour of the program. Please, undo last refactoring!");
			}
		}
		if (s_opt.cxrefs==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK) {
			fprintf(ccOut, "-![Warning] A method (getter or setter) created during the encapsulation has the same name as an existing method, so it will be inserted into this (existing) inheritance hierarchy. This may cause that the refactoring will not be behaviour preserving. Please, select applications unambiguously reporting to the newly created method. If you can't do this, you should undo the refactoring and rename the field first!\n");
		}
	}
}

static int getCurrentRefPosition(S_olcxReferences *refs) {
	S_reference 	*rr;
	int 			actn;
	int				rlevel;
	actn = 0;
	rr = NULL;
	if (refs!=NULL) {
		rlevel = s_refListFilters[refs->refsFilterLevel];
		for(rr=refs->r; rr!=NULL && rr!=refs->act; rr=rr->next) {
			if (rr->usg.base < rlevel) actn ++ ;
		}
	}
	if (rr==NULL) actn = 0;
	return(actn);
}

static void symbolHighlighNameSprint(char *output, S_olSymbolsMenu *ss) {
	char *bb, *cc;
	int len, llen;
	sprintfSymbolLinkName(output, ss);
	cc = strchr(output, '(');
	if (cc != NULL) *cc = 0;
	len = strlen(output);
	output[len]=0;
	bb = lastOccurenceInString(output,'.');
	if (bb!=NULL) {
		bb++;
		llen = strlen(bb);
		memmove(output, bb, llen+1);
		output[llen]=0;
	}
}

static void olcxPrintRefList( char *commandString, S_olcxReferences *refs ) {
	S_reference *rr;
	int 		actn, nn, len;
	char		ttt[MAX_CX_SYMBOL_SIZE];
	char		*cc;
	if (s_opt.xref2) {
		actn = getCurrentRefPosition(refs);
		if (refs!=NULL && refs->menuSym != NULL) {
			ttt[0]='\"';
			symbolHighlighNameSprint(ttt+1, refs->menuSym);
			len = strlen(ttt);
			ttt[len]='\"';
			ttt[len+1]=0;
			ppcGenWithNumericAndRecordBegin(PPC_REFERENCE_LIST, actn, 
											PPCA_SYMBOL, ttt);
		} else {
			ppcGenNumericRecordBegin(PPC_REFERENCE_LIST, actn);
		}
	} else {
		fprintf(ccOut,"%s",commandString);/* communication char */
	}
	if (refs!=NULL) {
		rr=refs->r;
		while (rr != NULL) {
			passRefsThroughSourceFile(&rr, &refs->act->p,
									  ccOut, "*xref-list*", USAGE_FILTER, 
									  s_refListFilters[refs->refsFilterLevel]);
		}
	}
	if (s_opt.xref2) {
		ppcGenRecordEnd(PPC_REFERENCE_LIST);
		//& if (refs!=NULL && refs->act!=NULL) ppcGenGotoPositionRecord(&refs->act->p);
	}
	fflush(ccOut);
}

static void olcxReferenceList(char *commandString) {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, CHECK_NULL);
//&	setRefSuffix(refs);
	//&LIST_MERGE_SORT(S_reference, refs->r, olcxListLessFunction);
	olcxPrintRefList( commandString, refs);
}

static void olcxListTopReferences(char *commandString) {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
	olcxPrintRefList( commandString, refs);
}

static void olcxGenGotoActReference(S_olcxReferences *refs) {
	if (refs->act != NULL) {
		generateOnlineCxref(&refs->act->p, COLCX_GOTO_REFERENCE, 
							refs->act->usg.base, refs->refsuffix, "");
	} else {
		olcxGenNoReferenceSignal();
	}
}

static void olcxPushOnly() {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser, refs, CHECK_NULL);
	//&LIST_MERGE_SORT(S_reference, refs->r, olcxListLessFunction);
	olcxGenGotoActReference(refs);
}

static void olcxPushAndCallMacro() {
	S_olcxReferences 	*refs;
	S_reference			*rr;
	char				symbol[MAX_CX_SYMBOL_SIZE];
	OLCX_MOVE_INIT(s_olcxCurrentUser, refs, CHECK_NULL);
	LIST_MERGE_SORT(S_reference, refs->r, olcxListLessFunction);
	LIST_REVERSE(S_reference, refs->r);
	assert(s_opt.xref2);
	symbolHighlighNameSprint(symbol, refs->hkSelectedSym);
	// precheck first
	for(rr=refs->r; rr!=NULL; rr=rr->next) {
		ppcGenReferencePreCheckRecord(rr, symbol);
	}
	for(rr=refs->r; rr!=NULL; rr=rr->next) {
		ppcGenReferencePreCheckRecord(rr, symbol);
		ppcGenRecord(PPC_CALL_MACRO, "", "\n");
	}
	LIST_REVERSE(S_reference, refs->r);
}

static void olcxReferenceGotoRef(int refn) {
	S_olcxReferences 	*refs;
	S_reference			*rr;
	int 				i,rfilter;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	rfilter = s_refListFilters[refs->refsFilterLevel];
	for(rr=refs->r,i=1; rr!=NULL && (i<refn||rr->usg.base>=rfilter); rr=rr->next){
		if (rr->usg.base < rfilter) i++;
	}
	refs->act = rr;
	olcxGenGotoActReference(refs);
}

S_olCompletion *olCompletionNthLineRef(S_olCompletion *cpls, int refn) {
	S_olCompletion *rr, *rrr;
	int i;
	for(rr=rrr=cpls, i=1; i<=refn && rrr!=NULL; rrr=rrr->next) {
		i += rrr->lineCount;
		rr = rrr;
	}
	return(rr);
}

static void olcxPopUser() {
	s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;
}

static void olcxPopUserAndFreePoped() {
	olcxPopUser();
	olcxFreePopedStackItems(&s_olcxCurrentUser->browserStack);
}

static S_olcxReferences *olcxPushUserOnPhysicalTopOfStack() {
	S_olcxReferences *oldtop;
	oldtop = s_olcxCurrentUser->browserStack.top;
	s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.root;
	olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
	return(oldtop);
}

static void olcxPopAndFreeAndPopsUntil(S_olcxReferences *oldtop) {
	olcxPopUserAndFreePoped();
	// recover old top, but what if it was freed, hmm
	while (s_olcxCurrentUser->browserStack.top!=NULL && s_olcxCurrentUser->browserStack.top!=oldtop) {
		olcxPopUser();
	}
}

static void olcxFindDefinitionAndGenGoto(S_symbolRefItem *sym) {
	S_olcxReferences 	*refs, *oldtop;
	S_olSymbolsMenu		mmm;
	// preserve poped items from browser first
	oldtop = olcxPushUserOnPhysicalTopOfStack();
	refs = s_olcxCurrentUser->browserStack.top;
	FILL_olSymbolsMenu(&mmm, *sym, 1,1,0,UsageUsed,0,0,0,UsageNone,s_noPos,0, NULL, NULL);
	//&oldrefs = *refs; 
	refs->menuSym = &mmm;
	readOneAppropReferenceFile(sym->name, s_cxFullScanFunTab);
	orderRefsAndGotoDefinition(refs, DEFAULT_VALUE);
	//&olcxFreeReferences(refs->r);
	//&*refs = oldrefs;
	refs->menuSym = NULL;
	// recover stack
	olcxPopAndFreeAndPopsUntil(oldtop);
}

static void olcxReferenceGotoCompletion(int refn) {
	S_olcxReferences 	*refs;
	S_olCompletion		*rr,*rrr;
	S_symbolRefItemBits	sb;
	int 				i,jdoc;
	assert(refn > 0);
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	rr = olCompletionNthLineRef(refs->cpls, refn);
	if (rr != NULL) {
		if (rr->cat == CatLocal /*& || refs->command == OLO_TAG_SEARCH &*/) {
			if (rr->ref.usg.base != UsageClassFileDefinition
				&& rr->ref.usg.base != UsageClassTreeDefinition
				&& POSITION_NEQ(rr->ref.p, s_noPos)) {
					generateOnlineCxref(&rr->ref.p, COLCX_GOTO_REFERENCE, 
										UsageDefined, refs->refsuffix, "");
			} else {
				jdoc = olcxBrowseSymbolInJavaDoc(&rr->sym);
				if (jdoc==0) olcxGenNoReferenceSignal();
			}
		} else {
			olcxFindDefinitionAndGenGoto(&rr->sym);
		}
	} else {
		olcxGenNoReferenceSignal();
	}
}

static void olcxReferenceGotoTagSearchItem(int refn) {
	S_olCompletion		*rr,*rrr;
	S_symbolRefItemBits	sb;
	int 				i,jdoc;
	assert(refn > 0);
	assert(s_olcxCurrentUser);
	assert(s_olcxCurrentUser->retrieverStack.top);
	rr = olCompletionNthLineRef(s_olcxCurrentUser->retrieverStack.top->cpls, refn);
	if (rr != NULL) {
		if (rr->ref.usg.base != UsageClassFileDefinition
			&& rr->ref.usg.base != UsageClassTreeDefinition
			&& POSITION_NEQ(rr->ref.p, s_noPos)) {
			generateOnlineCxref(&rr->ref.p, COLCX_GOTO_REFERENCE, 
								UsageDefined, "0:#", "");
		} else {
			jdoc = olcxBrowseSymbolInJavaDoc(&rr->sym);
			if (jdoc==0) olcxGenNoReferenceSignal();
		}
	} else {
		olcxGenNoReferenceSignal();
	}
}

static void olcxReferenceBrowseCompletion(int refn) {
	S_olcxReferences 	*refs;
	S_olCompletion		*rr;
	char 				*url,*tt;
	int 				aa;
	assert(refn > 0);
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	rr = olCompletionNthLineRef(refs->cpls, refn);
	if (rr != NULL) {
		if (rr->cat == CatLocal) {
			fprintf(ccOut,"* ** no JavaDoc is available for local symbols **");
		} else {
			aa = olcxBrowseSymbolInJavaDoc(&rr->sym);
			if (aa==0) {
				fprintf(ccOut, "*** JavaDoc for ");
				url = getJavaDocUrl_st(&rr->sym);
				for(tt=url ; *tt && *tt!='#'; tt++) fputc(*tt, ccOut);
				fprintf(ccOut, " not available, (check -javadocpath) **");
			}
		}
	} else {
		fprintf(ccOut, "* ** out of range **");
	}
}

static void olcxSetActReferenceToFirstVisible(S_olcxReferences *refs, S_reference *r) {
	int					rlevel;
	rlevel = s_refListFilters[refs->refsFilterLevel];
	while (r!=NULL && r->usg.base>=rlevel) r = r->next;
	if (r != NULL) {
		refs->act = r;
	} else {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_BOTTOM_INFORMATION, "Moving to the first reference", "\n");
		}
		r = refs->r;
		while (r!=NULL && r->usg.base>=rlevel) r = r->next;
		refs->act = r;
	}
}

static void olcxReferencePlus() {
	S_olcxReferences 	*refs;
	S_reference			*r;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	if (refs->act == NULL) refs->act = refs->r;
	else {
		r = refs->act->next;
		olcxSetActReferenceToFirstVisible(refs, r);
	}
	olcxGenGotoActReference(refs);
}

static void olcxReferenceMinus() {
	S_olcxReferences 	*refs;
	S_reference 		*r,*l,*act;
	int					rlevel;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	rlevel = s_refListFilters[refs->refsFilterLevel];
	if (refs->act == NULL) refs->act = refs->r;
	else {
		act = refs->act;
		l = NULL;
		for(r=refs->r; r!=act && r!=NULL; r=r->next) {
			if (r->usg.base < rlevel) l = r;
		}
		if (l==NULL) {
			if (s_opt.xref2) {
				ppcGenRecord(PPC_BOTTOM_INFORMATION, "Moving to the last reference", "\n");
			}
			for( ; r!=NULL; r=r->next) {
				if (r->usg.base < rlevel) l = r;
			}
		}
		refs->act = l;
	}
	olcxGenGotoActReference(refs);
}

static void olcxReferenceGotoDef() {
	S_olcxReferences 	*refs;
	S_reference 		*dr;
	int					rlevel;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	dr = getDefinitionRef(refs->r);
	if (dr != NULL) refs->act = dr;
	else refs->act = refs->r;
//&fprintf(dumpOut,"goto ref %d %d\n", refs->act->p.line, refs->act->p.coll);
	olcxGenGotoActReference(refs);
}

static void olcxReferenceGotoCurrent() {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	olcxGenGotoActReference(refs);
}

static void olcxReferenceGetCurrentRefn() {
	S_olcxReferences 	*refs;
	int					n;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	n = getCurrentRefPosition(refs);
	assert(s_opt.xref2);
	ppcGenNumericRecord(PPC_UPDATE_CURRENT_REFERENCE, n, "", "\n");
}

static void olcxReferenceGotoCaller() {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	if (refs->cpos.file != s_noneFileIndex) {
		generateOnlineCxref(&refs->cpos, COLCX_GOTO_REFERENCE, 
							UsageUsed, refs->refsuffix, "");

	} else {
		olcxGenNoReferenceSignal();
	}
}

#define MAX_SYMBOL_MESSAGE_LEN 50

static void olcxPrintSymbolName( S_olcxReferences *refs ) {
	char ttt[MAX_CX_SYMBOL_SIZE+MAX_SYMBOL_MESSAGE_LEN];
	S_olSymbolsMenu *ss;
	if (refs==NULL) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_BOTTOM_INFORMATION, "stack is now empty", "\n");
		} else {
			fprintf(ccOut, "*stack is now empty");
		}
//&		fprintf(ccOut, "*");
	} else if (refs->hkSelectedSym==NULL) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_BOTTOM_INFORMATION, "Current top symbol: <empty>", "\n");
		} else {
			fprintf(ccOut, "*Current top symbol: <empty>");
		}
	} else {
		ss = refs->hkSelectedSym;
		if (s_opt.xref2) {
			sprintf(ttt, "Current top symbol: ");
			assert(strlen(ttt) < MAX_SYMBOL_MESSAGE_LEN);
			sprintfSymbolLinkName(ttt+strlen(ttt), ss);
			ppcGenRecord(PPC_BOTTOM_INFORMATION, ttt, "\n");
		} else {
			fprintf(ccOut, "*Current top symbol: ");
			printSymbolLinkName(ccOut, ss);
		}
	}
}


static void olcxShowTopSymbol() {
	S_olcxReferences 	*refs;
	S_olSymbolsMenu		*ss;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
//&	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, CHECK_NULL);
	olcxPrintSymbolName( refs);
}

static int referenceLess(S_reference *r1, S_reference *r2) {
	return(POSITION_LESS(r1->p, r2->p));
}

static S_olSymbolsMenu *findSymbolCorrespondingToReference(
	S_olSymbolsMenu *menu, 
	S_reference *ref
	) {
	S_olSymbolsMenu *ss;
	S_reference *rr;
	for(ss=menu; ss!=NULL; ss=ss->next) {
		SORTED_LIST_FIND3(rr, S_reference, ref, ss->s.refs, referenceLess);
		if (rr!=NULL && POSITION_EQ(rr->p, ref->p)) {
			return(ss);
		}
	}
	return(NULL);
}

static void olcxShowTopApplClass() {
	S_olcxReferences 	*refs;
	S_olSymbolsMenu		*ss;
	S_olSymbolsMenu		*mms;
//&	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, CHECK_NULL);
	assert(refs->act!=NULL);
	mms = findSymbolCorrespondingToReference(refs->menuSym, refs->act);
	if (mms==NULL) {
		olcxGenNoReferenceSignal();
	} else {
		fprintf(ccOut, "*");
		printClassFqtNameFromClassNum(ccOut, mms->s.vApplClass);
	}
}

static void olcxShowTopType() {
	S_olcxReferences 	*refs;
	S_olSymbolsMenu		*ss;
	S_olSymbolsMenu		*mms;
//&	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, CHECK_NULL);
	assert(refs->act!=NULL);
	mms = findSymbolCorrespondingToReference(refs->menuSym, refs->act);
	if (mms==NULL) {
		olcxGenNoReferenceSignal();
	} else {
		fprintf(ccOut, "*%s",typesName[mms->s.b.symType]);
	}
}

static void olcxShowClassTree() {
	olcxPrintClassTree(s_olcxCurrentUser->ct.tree);
}

S_olSymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi,int storage){
	S_olSymbolsMenu 	*res;
	S_symbolRefItemBits bb;
	S_symbolRefItem 	ss;
	FILL_symbolRefItemBits(&bb, TypeDefault, storage, ScopeGlobal,
						   ACC_DEFAULT, CatGlobal, 0);
	FILL_symbolRefItem(&ss, fieldName, cxFileHashNumber(fieldName),
					   cfi, cfi, bb, NULL, NULL);
	res = olCreateNewMenuItem(&ss, ss.vApplClass, ss.vFunClass, &s_noPos, UsageNone,
							  1, 1, OOC_VIRT_SAME_APPL_FUN_CLASS, 
							  UsageUsed, 0);
	return(res);
}

static void olcxTopSymbolResolution() {
	S_olcxReferences 	*refs;
	S_olSymbolsMenu		*ss;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
	ss = NULL;
	if (refs!=NULL) {
		LIST_MERGE_SORT(S_olSymbolsMenu,
						refs->menuSym,
						htmlRefItemsOrderLess);
		ss = refs->menuSym;
	}
	olcxPrintSelectionMenu(ss);
}

#define CHECK_ATTRIBUTES(p1,p2) {\
	if (p1 == p2) return(1);\
	if (p1->b.category != p2->b.category) return(0);\
	if (p1->b.symType!=TypeCppCollate && p2->b.symType!=TypeCppCollate && p1->b.symType!=p2->b.symType) return(0);\
	if (p1->b.storage!=p2->b.storage) return(0);\
}

int itIsSameCxSymbol(S_symbolRefItem *p1, S_symbolRefItem *p2) {
	CHECK_ATTRIBUTES(p1,p2);
	if (strcmp(p1->name,p2->name)) return(0);
	return(1);
}

int itIsSameCxSymbolIncludingFunClass(S_symbolRefItem *p1, S_symbolRefItem *p2) {
	if (p1->vFunClass != p2->vFunClass) return(0);
	return(itIsSameCxSymbol(p1, p2));
}
int itIsSameCxSymbolIncludingApplClass(S_symbolRefItem *p1, S_symbolRefItem *p2) {
	if (p1->vApplClass != p2->vApplClass) return(0);
	return(itIsSameCxSymbol(p1, p2));
}
int olcxItIsSameCxSymbol(S_symbolRefItem *p1, S_symbolRefItem *p2) {
	int c, n1len, n2len;
	char *ss, *n1start, *n2start;
//&	CHECK_ATTRIBUTES(p1,p2);  // to show panic macro and panic symbol and to make safetycheck working
	GET_NUDE_NAME(p1->name, n1start, n1len);
	GET_NUDE_NAME(p2->name, n2start, n2len);
	if (n1len != n2len) return(0);
	if (strncmp(n1start, n2start, n1len)) return(0);
	return(1);
}

void olStackDeleteSymbol( S_olcxReferences *refs) {
	S_olcxReferences **rr;
	for(	rr= &s_olcxCurrentUser->browserStack.root;
			*rr!=NULL&&*rr!=refs;
			rr= &(*rr)->previous
		) ;
	assert(*rr != NULL);
	deleteOlcxRefs(rr, &s_olcxCurrentUser->browserStack);
}

static void olcxGenInspectClassDefinitionRef(int classnum, char *refsuffix) {
	S_symbolRefItem 	mmm;
	char 				ccc[MAX_CX_SYMBOL_SIZE];
	javaGetClassNameFromFileNum(classnum, ccc, KEEP_SLASHES);
	FILL_symbolRefItemBits(&mmm.b, TypeStruct, StorageExtern, ScopeGlobal,
						   ACC_DEFAULT, CatGlobal, 0);
	FILL_symbolRefItem(&mmm, ccc, cxFileHashNumber(ccc), 
					   s_noneFileIndex, s_noneFileIndex, mmm.b,
					   NULL, NULL);
//&sprintf(tmpBuff, "looking for %s (%s)", mmm.name, s_fileTab.tab[mmm.vApplClass]->name);ppcGenTmpBuff();
	olcxFindDefinitionAndGenGoto(&mmm);
}

static void olcxMenuInspectDef(S_olSymbolsMenu *menu, char *refsuffix,
							   int inspect) {
	S_olSymbolsMenu 	*ss;
	int 				line;
	for(ss=menu; ss!=NULL; ss=ss->next) {
//&sprintf(tmpBuff,"checking line %d", ss->outOnLine);	ppcGenRecord(PPC_BOTTOM_INFORMATION, tmpBuff, "\n");
		line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
		if (line == s_opt.olcxMenuSelectLineNum) goto breakl;
	}
breakl:
	if (ss == NULL) {
		olcxGenNoReferenceSignal();
	} else {
		if (inspect == INSPECT_DEF) {
			if (ss->defpos.file>=0 && ss->defpos.file!=s_noneFileIndex) {
				generateOnlineCxref(&ss->defpos, COLCX_GOTO_REFERENCE, 
									UsageDefined, refsuffix, "");
			} else if (! olcxBrowseSymbolInJavaDoc(&ss->s)) {
				olcxGenNoReferenceSignal();
			}
		} else {
			// inspect class
			olcxGenInspectClassDefinitionRef(ss->s.vApplClass, refsuffix);
		}
	}
}

static void olcxSymbolMenuInspectClass() {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	olcxMenuInspectDef(refs->menuSym, refs->refsuffix, INSPECT_CLASS);
}

static void olcxSymbolMenuInspectDef() {
	S_olcxReferences 	*refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	olcxMenuInspectDef(refs->menuSym, refs->refsuffix, INSPECT_DEF);
}

static void olcxClassTreeInspectDef() {
	assert(s_olcxCurrentUser);
	olcxMenuInspectDef(s_olcxCurrentUser->ct.tree, "0:#", INSPECT_CLASS);
}

void olProcessSelectedReferences(
		S_olcxReferences  	*rstack,
		void (*referencesMapFun)(S_olcxReferences *rstack, S_olSymbolsMenu *ss)
	) {
	S_olSymbolsMenu		*ss, *sss;
	int					fnum, nn;
	ss = rstack->menuSym;
	if (ss == NULL) return;
	//& renameCollationSymbols(ss);
	LIST_MERGE_SORT(S_reference, rstack->r, olcxReferenceInternalLessFunction);	
	for(ss=rstack->menuSym; ss!=NULL; ss=ss->next) {
//&LIST_LEN(nn, S_reference, ss->s.refs);sprintf(tmpBuff,"xxx1 %d refs for %s", nn, s_fileTab.tab[ss->s.vApplClass]->name);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
		referencesMapFun(rstack, ss);
	}
	olcxSetCurrentRefsOnCaller( rstack);
	LIST_MERGE_SORT(S_reference, rstack->r, olcxListLessFunction);
}

void olcxRecomputeSelRefs( S_olcxReferences *refs ) {
	int num;
	// [28/12] putting it into comment, just to see, if it works
	// otherwise it clears stack on pop on empty stack
	//& olcxFreePopedStackItems(&s_olcxCurrentUser->browserStack);
	olcxFreeReferences(refs->r); refs->r = NULL;
	olProcessSelectedReferences(refs, genOnLineReferences);
}

static void olcxMenuToggleSelect() {
	S_userOlcx 			*user;
	S_olcxReferences 	*refs;
	S_olSymbolsMenu 	*ss;
	int 				line,num;
	char 				ln[MAX_HTML_REF_LEN];
	char				*cname;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	for(ss=refs->menuSym; ss!=NULL; ss=ss->next) {
		line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
		if (line == s_opt.olcxMenuSelectLineNum) {
			ss->selected = ss->selected ^ 1;
			olcxRecomputeSelRefs( refs);
			break;
		}
	}
	if (s_opt.xref2) {
		if (ss!=NULL) {
			olcxPrintRefList( ";", refs);
		}
	} else {
		if (ss==NULL) {
			olcxGenNoReferenceSignal();
		} else {
			linkNamePrettyPrint(ln,ss->s.name,MAX_HTML_REF_LEN,SHORT_NAME);
			cname = javaGetNudePreTypeName_st(getRealFileNameStatic(
				s_fileTab.tab[ss->s.vApplClass]->name), s_opt.nestedClassDisplaying);
			sprintf(tmpBuff, "%s %s refs of \"%s\"", 
					(ss->selected?"inserting":"removing"), cname, ln);
			fprintf(ccOut,"*%s", tmpBuff);
//&fprintf(ccOut,"\"%s\" refs of \"%s\"", s_fileTab.tab[ss->s.vApplClass]->name, ss->s.name);
		}
	}
}

static void olcxMenuSelectOnly() {
	S_userOlcx 			*user;
	S_olcxReferences 	*refs;
	S_olSymbolsMenu 	*ss, *sel;
	S_reference			*dref;
	int 				line,num,jd;
	char				ttt[MAX_CX_SYMBOL_SIZE];
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	sel = NULL;
	for(ss=refs->menuSym; ss!=NULL; ss=ss->next) {
		ss->selected = 0;
		line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
		if (line == s_opt.olcxMenuSelectLineNum) {
			ss->selected = 1;
			sel = ss;
		}
	}
	if (sel==NULL) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_BOTTOM_WARNING, "No Symbol", "\n");
		} else {
			fprintf(ccOut,"*No symbol");
		}
		return;
	}
	olcxRecomputeSelRefs( refs);
	if (s_opt.xref2) {
		dref = getDefinitionRef(refs->r);
		if (dref != NULL) refs->act = dref;
		olcxPrintRefList(";", refs);
		if (dref == NULL) {
			if (sel!=NULL && sel->s.vApplClass!=sel->s.vFunClass) {
				sprintfSymbolLinkName(ttt, sel);
				sprintf(tmpBuff,"Class %s does not define %s", javaGetShortClassNameFromFileNum_st(sel->s.vApplClass), ttt);
				ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 1, tmpBuff, "\n");
			} else {
				jd = olcxBrowseSymbolInJavaDoc(&sel->s);  //& checkTheJavaDocBrowsing(refs);
				if (jd==0) {
					ppcGenDefinitionNotFoundWarningAtBottom();
				} else {
					ppcGenRecord(PPC_BOTTOM_INFORMATION, "Definition not found, loading javadoc.", "\n");
				}
			}
		} else {
			ppcGenGotoPositionRecord(&refs->act->p);
		}
	} else {
		fprintf(ccOut, "*");
	}
}


static void unselectUsedSymbol(S_olSymbolsMenu *list, S_olSymbolsMenu *sym) {
	S_olSymbolsMenu *s;
	for(s=list; s!=NULL; s=s->next) {
		if (s->s.vFunClass == sym->s.vFunClass) {
			s->selected = 0;
		}
	}
}

static void selectUnusedSymbols(S_olSymbolsMenu *mm, void *vflp, void *p2) {
	S_olSymbolsMenu 	*ss, *s;
	int 				used, atleastOneSelected, filter, *flp;
	flp = (int *)vflp;
	filter = *flp;
	for(ss=mm; ss!=NULL; ss=ss->next) {
		ss->visible = 1; ss->selected = 0;
	}
	if (mm->s.b.storage != StorageField && mm->s.b.storage != StorageMethod) {
		for(ss=mm; ss!=NULL; ss=ss->next) {
			if (ss->defRefn!=0 && ss->refn==0) ss->selected = 1;
		}
		goto fini;
	}
	atleastOneSelected = 1;
	while (atleastOneSelected) {
		atleastOneSelected = 0;
		// O.K. find definition
		for(ss=mm; ss!=NULL; ss=ss->next) {
			if (ss->selected==0 && ss->defRefn!=0) {
				assert(ss->s.vFunClass == ss->s.vApplClass);
				// O.K. is it potentially used?
				used = 0;
				for(s=mm; s!=NULL; s=s->next) {
					if (ss->s.vFunClass == s->s.vFunClass) {
						if (s->refn != 0) {
							used = 1;
							goto checked;
						}
					}
				}
				// for method, check if can be used in a superclass
				if (ss->s.b.storage == StorageMethod) {
					for(s=mm; s!=NULL; s=s->next) {
						if (s->s.vFunClass == s->s.vApplClass       // it is a definition
							&& ss->s.vApplClass != s->s.vApplClass  // not this one
							&& s->selected == 0                     // used
							&& isSmallerOrEqClass(ss->s.vApplClass, s->s.vApplClass)) {
							used = 1;
							goto checked;
						}
					}
				}
			checked:
				if (! used) {
					atleastOneSelected = 1;
					ss->selected = 1;
				}
			}
		}
	}
 fini:
	for(ss=mm; ss!=NULL; ss=ss->next) {
		if (ss->selected) goto fini2;
	}
	//nothing selected, make the symbol unvisible
	for(ss=mm; ss!=NULL; ss=ss->next) {
		ss->visible = 0;
	}
 fini2:
	if (filter>0) {
		// make all unselected unvisible
		for(ss=mm; ss!=NULL; ss=ss->next) {
			if (ss->selected == 0) ss->visible = 0;
		}
	}
	return;
}


static void olcxMenuSelectAll(int val) {
	S_userOlcx *user;
	S_olcxReferences *refs;
	S_olSymbolsMenu *ss;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	if (refs->command == OLO_GLOBAL_UNUSED) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_WARNING, "The browser does not display project unused symbols anymore","\n");
		}
	}
	for(ss=refs->menuSym; ss!=NULL; ss=ss->next) {
		if (ss->visible) ss->selected = val;
	}
	olcxRecomputeSelRefs( refs);
	if (s_opt.xref2) {
		olcxPrintRefList( ";", refs);
	} else {
		fprintf(ccOut, "*Done");
	}
}

static void setDefaultSelectedVisibleItems(S_olSymbolsMenu *menu,
										   unsigned ooVisible,
										   unsigned ooSelected
	) {
	S_olSymbolsMenu *ss;
	int select, visible;
	unsigned ooBits;
	for(ss=menu; ss!=NULL; ss=ss->next) {
		ooBits = ss->ooBits;
		visible = ooBitsGreaterOrEqual(ooBits, ooVisible);
		select = 0;
		if (visible) {
			select=ooBitsGreaterOrEqual(ooBits, ooSelected);
			if (ss->s.b.symType==TypeCppCollate) select=0;
		}
		ss->selected = select;
		ss->visible = visible;
	}
}

static int isSpecialConstructorOnlySelectionCase(int command) {
	if (command == OLO_PUSH) return(1);
	if (command == OLO_PUSH_ONLY) return(1);
	if (command == OLO_PUSH_AND_CALL_MACRO) return(1);
	if (command == OLO_ARG_MANIP) return(1);
	if (command == OLO_SAFETY_CHECK2) {
		// here check if origin was an argument manipulation
		S_olcxReferences *refs, *origrefs, *newrefs, *diffrefs;
		int pbflag=0;
		origrefs = newrefs = diffrefs = NULL;
		SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
		assert(origrefs && newrefs && diffrefs);
		if (pbflag) return(0);
		if (origrefs->command == OLO_ARG_MANIP) return(1);
	}
	return(0);
}

static void handleConstructorSpecialsInSelectingSymbolInMenu(
	S_olSymbolsMenu *menu, int command
	) {
	S_olSymbolsMenu *s1, *s2, *ss, *dd;
	S_reference *dr1, *dr2;
	int ccc, sss, lll, vn;
	if (! LANGUAGE(LAN_JAVA)) return;
	if (! isSpecialConstructorOnlySelectionCase(command)) return;
	s1 = NULL; s2 = NULL;
	vn = 0;
	for(ss=menu; ss!=NULL; ss=ss->next) {
		if (ss->visible) {
			if (vn==0) s1 = ss;
			else if (vn==1) s2 = ss;
			vn ++;
		}
	}
	if (vn == 2) {
		// exactly two visible items
		assert(s1 && s2);
		sss = s1->selected + s2->selected;
		ccc = (s1->s.b.storage==StorageConstructor) + (s2->s.b.storage==StorageConstructor);
		lll = (s1->s.b.symType==TypeStruct) + (s2->s.b.symType==TypeStruct);
//&fprintf(dumpOut," sss,ccc,lll == %d %d %d\n",sss,ccc,lll);
		if (sss==2 && ccc==1 && lll==1) {
			//dr1 = getDefinitionRef(s1->s.refs);
			//dr2 = getDefinitionRef(s2->s.refs);
			// deselect class references, but only if constructor definition exists
			// it was not a good idea with def refs, because of browsing of
			// javadoc for standard constructors
			if (s1->s.b.symType==TypeStruct /*& && dr2!=NULL &*/) {
				s1->selected = 0;
				if (command == OLO_ARG_MANIP) s1->visible = 0;
			} else if (s2->s.b.symType==TypeStruct /*& && dr1!=NULL &*/) {
				s2->selected = 0;
				if (command == OLO_ARG_MANIP) s2->visible = 0;
			}
		}
	}	

}


#define RENAME_MENU_SELECTION(command) (\
	command == OLO_RENAME \
	|| command == OLO_ENCAPSULATE \
	|| command == OLO_VIRTUAL2STATIC_PUSH \
	|| command == OLO_ARG_MANIP \
	|| command == OLO_PUSH_FOR_LOCALM \
	|| command == OLO_SAFETY_CHECK1 \
	|| command == OLO_SAFETY_CHECK2 \
	|| s_opt.manualResolve == RESOLVE_DIALOG_NEVER \
)

void dummyloop1(){}
void dummyloop2(){}

static void computeSubClassOfRelatedItemsOOBit(S_olSymbolsMenu *menu, int command) {
	S_olSymbolsMenu *s1, *s2;
	unsigned oov;
	int st, change;
	if (RENAME_MENU_SELECTION(command)) {
		// even worse than O(n^2), hmm.
		change = 1;
		while (change) {
			change = 0;
			for(s1=menu; s1!=NULL; s1=s1->next) {
				if ((s1->ooBits&OOC_VIRTUAL_MASK) < OOC_VIRT_SUBCLASS_OF_RELATED) goto nextrs1;
				for(s2=menu; s2!=NULL; s2=s2->next) {
					// do it only for virtuals
					st = JAVA_STATICALLY_LINKED(s2->s.b.storage, s2->s.b.accessFlags);
					if (st) goto nextrs2;
					oov = (s2->ooBits & OOC_VIRTUAL_MASK);
					if(oov >= OOC_VIRT_SUBCLASS_OF_RELATED) goto nextrs2;
					if (isSmallerOrEqClass(s2->s.vApplClass, s1->s.vApplClass)) {
						s2->ooBits = ((s2->ooBits & ~OOC_VIRTUAL_MASK)
									  | OOC_VIRT_SUBCLASS_OF_RELATED);
						change = 1;
					}
					if (isSmallerOrEqClass(s1->s.vApplClass, s2->s.vApplClass)) {
						s2->ooBits = ((s2->ooBits & ~OOC_VIRTUAL_MASK)
									  | OOC_VIRT_SUBCLASS_OF_RELATED);
						change = 1;
					}
				nextrs2:;
				}
			nextrs1:;
			}
		}
	} else {
		// O(n^2), someone should do this better !!!
		for(s1=menu; s1!=NULL; s1=s1->next) {
			if ((s1->ooBits&OOC_VIRTUAL_MASK) < OOC_VIRT_RELATED) goto nexts1;
			for(s2=menu; s2!=NULL; s2=s2->next) {
				// do it only for virtuals
				st = JAVA_STATICALLY_LINKED(s2->s.b.storage, s2->s.b.accessFlags);
				if (st) goto nexts2;
				oov = (s2->ooBits & OOC_VIRTUAL_MASK);
				if(oov >= OOC_VIRT_SUBCLASS_OF_RELATED) goto nexts2;
				if (isSmallerOrEqClass(s2->s.vApplClass, s1->s.vApplClass)) {
					s2->ooBits = ((s2->ooBits & ~OOC_VIRTUAL_MASK)
								  | OOC_VIRT_SUBCLASS_OF_RELATED);
				}
			nexts2:;
			}
		nexts1:;
		}
	}
}

static void setSelectedVisibleItems(S_olSymbolsMenu *menu, int command, int filterLevel) {
	S_olSymbolsMenu *s1, *s2;
	int ccc, sss, lll;
	unsigned ooselected, oovisible;
	if (command == OLO_GLOBAL_UNUSED) {
		splitMenuPerSymbolsAndMap(menu, selectUnusedSymbols, &filterLevel, NULL);
		goto sfini;
	}
	// do not compute subclasses of related for class tree, it is too slow
	// and useless in this context
	if (command != OLO_CLASS_TREE) {
		computeSubClassOfRelatedItemsOOBit(menu, command);
	}
	if (command == OLO_MAYBE_THIS
		|| command == OLO_NOT_FQT_REFS
		|| command == OLO_NOT_FQT_REFS_IN_CLASS
		|| command == OLO_USELESS_LONG_NAME
		|| command == OLO_USELESS_LONG_NAME_IN_CLASS
		|| command == OLO_PUSH_ALL_IN_METHOD
		|| command == OLO_PUSH_NAME
		|| command == OLO_PUSH_SPECIAL_NAME
		) {
		// handle those very special cases first
		// set it to select and show all symbols
		oovisible = 0;
		ooselected = 0;
	} else if (RENAME_MENU_SELECTION(command)) {
		oovisible = s_opt.ooChecksBits;
		ooselected = RENAME_SELECTION_OO_BITS;
	} else {
		if (s_olstringServed && s_olstringUsage == UsageMethodInvokedViaSuper) {
			//&oovisible = s_opt.ooChecksBits;
			oovisible = s_menuFilterOoBits[filterLevel];
			ooselected = METHOD_VIA_SUPER_SELECTION_OO_BITS;
		} else {
			//&oovisible = s_opt.ooChecksBits;
			oovisible = s_menuFilterOoBits[filterLevel];
			ooselected = DEFAULT_SELECTION_OO_BITS;
		}
	}
	setDefaultSelectedVisibleItems(menu, oovisible, ooselected);
	// for local motion a class browsing would make strange effect
	// if class is deselected on constructors
	handleConstructorSpecialsInSelectingSymbolInMenu(menu, command);
 sfini:
	return;
}

static void olcxMenuSelectPlusolcxMenuSelectFilterSet(int flevel) {
	S_userOlcx 			*user;
	S_olcxReferences 	*refs;
	S_olSymbolsMenu 	*ss;
	unsigned 			ooBits;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
	if (refs!=NULL && flevel < MAX_MENU_FILTER_LEVEL && flevel >= 0) {
		if (refs->menuFilterLevel != flevel) {
			refs->menuFilterLevel = flevel;
			setSelectedVisibleItems(refs->menuSym, refs->command, refs->menuFilterLevel);
			olcxRecomputeSelRefs(refs);
		}
	}
	if (refs!=NULL) {
		olcxPrintSelectionMenu(refs->menuSym);
		if (s_opt.xref2) {
			// auto update of references
			// useless, should be done by user interface (because of setting
			// of reffilter level)
			//& olcxPrintRefList(";", refs);
		}
	}
	if (refs==NULL) {
		if (s_opt.xref2) {
			olcxPrintSelectionMenu(NULL);
			olcxPrintRefList(";", NULL);
		} else {
			fprintf(ccOut, "=");
		}
	}
}

static void olcxReferenceFilterSet(int flevel) {
	S_userOlcx 			*user;
	S_olcxReferences 	*refs;
	S_olSymbolsMenu 	*ss;
	unsigned 			usagelevel;
	OLCX_MOVE_INIT(s_olcxCurrentUser, refs, DEFAULT_VALUE);
	if (refs!=NULL && flevel < MAX_REF_LIST_FILTER_LEVEL && flevel >= 0) {
		refs->refsFilterLevel = flevel;
//&		olcxPrintRefList( ";", refs);
//&	} else {
//&		olcxGenNoReferenceSignal();
	}
	if (s_opt.xref2) {
		// move to the visible reference
		if (refs!=NULL) olcxSetActReferenceToFirstVisible(refs, refs->act);
		olcxPrintRefList(";", refs);
	} else {
		fprintf(ccOut, "*");
	}
}

static S_olcxReferences *getNextTopStackItem(S_olcxReferencesStack *stack) {
	S_olcxReferences *rr, *nextrr;
	nextrr = NULL;
	rr = stack->root; 
	while (rr!=NULL && rr!=stack->top) {
		nextrr = rr;
		rr = rr->previous;
	}
	assert(rr==stack->top);
	return(nextrr);
}

static void olcxReferenceRePush() {
	S_userOlcx *user;
	char *tops;
	S_olcxReferences *refs,*rr,*nextrr;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, DEFAULT_VALUE);
	nextrr = getNextTopStackItem(&s_olcxCurrentUser->browserStack);
	if (nextrr != NULL) {
		s_olcxCurrentUser->browserStack.top = nextrr;
		olcxGenGotoActReference(s_olcxCurrentUser->browserStack.top);
		// TODO, replace this by follwoing since 1.6.1
		//& ppcGenGotoPositionRecord(&s_olcxCurrentUser->browserStack.top->cpos);
		olcxPrintSymbolName(s_olcxCurrentUser->browserStack.top);
	} else {
		if (s_opt.xref2) {
			ppcGenRecordWithNumeric(PPC_BOTTOM_WARNING, PPCA_BEEP, 1, "You are on the top of browser stack.", "\n");
		} else {
			fprintf(ccOut, "*** Complete stack, no pop-ed references");
		}
	}
}

static void olcxReferencePop() {
	S_userOlcx *user;
	char *tops;
	S_olcxReferences *refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser, refs, CHECK_NULL);
	if (refs->cpos.file != s_noneFileIndex) {
		generateOnlineCxref(&refs->cpos, COLCX_GOTO_REFERENCE, UsageUsed, 
							refs->refsuffix, "");
	} else {
		olcxGenNoReferenceSignal();
	}
//&	olStackDeleteSymbol( refs);  // this was before non deleting pop
	s_olcxCurrentUser->browserStack.top = refs->previous;
	olcxPrintSymbolName(s_olcxCurrentUser->browserStack.top);
}

void olcxPopOnly() {
	S_userOlcx *user;
	S_olcxReferences *refs;
	int previousIsList;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	if (!s_opt.xref2) fprintf(ccOut, "*");
//&	olStackDeleteSymbol( refs);
	s_olcxCurrentUser->browserStack.top = refs->previous;
}

S_reference * olcxCopyRefList(S_reference *ll) {
	S_reference *rr, *res, *a, **aa;
	res = NULL; aa= &res;
	for(rr=ll; rr!=NULL; rr=rr->next) {
		OLCX_ALLOC(a, S_reference);
		*a = *rr;
		a->next = NULL;
		*aa = a;
		aa = &(a->next);
	}
	return(res);
}

static S_olSymbolsMenu *olcxCopyMenuSym(S_olSymbolsMenu *mm) {
	S_olSymbolsMenu *rr, *res, *a, **aa;
	char *nn;
	res = NULL; aa= &res;
	for(rr=mm; rr!=NULL; rr=rr->next) {
		OLCX_ALLOC(a, S_olSymbolsMenu);
		*a = *rr;
		a->next = NULL;
		OLCX_ALLOCC(nn, strlen(rr->s.name)+1, char);
		strcpy(a->s.name, rr->s.name);
		a->s.refs = olcxCopyRefList(rr->s.refs);
		a->s.next = NULL;
#ifdef CORE_DUMP
		InternalCheck(rr->markers==NULL);
#endif
		a->markers = NULL;     // do not copy markers
		*aa = a;
		aa = &(a->next);
	}
	return(res);
}

static void safetyCheckAddDiffRef(S_reference *r, S_olcxReferences *diffrefs,
								  int mode) {
	int prefixchar;
	prefixchar = ' ';
	if (diffrefs->r == NULL) {
		fprintf(ccOut, "%s", COLCX_LIST);
		prefixchar = '>';
	}
	if (mode == DIFF_MISSING_REF) {
		fprintf(ccOut, "%c %s:%d missing reference\n", prefixchar,
				simpleFileNameFromFileNum(r->p.file), r->p.line);
	} else if (mode == DIFF_UNEXPECTED_REF) {
		fprintf(ccOut, "%c %s:%d unexpected new reference\n", prefixchar,
				simpleFileNameFromFileNum(r->p.file), r->p.line);
	} else {
		assert(0);
	}
	olcxAppendReference(r, diffrefs);
}

void olcxReferencesDiff(	S_reference **anr1, 
							S_reference **aor2,
							S_reference **diff
	) {
	S_reference *r, *nr1, *or2, **dd;
	int mode;
	LIST_MERGE_SORT(S_reference, *anr1, olcxReferenceInternalLessFunction);
	LIST_MERGE_SORT(S_reference, *aor2, olcxReferenceInternalLessFunction);
	nr1 = *anr1; or2 = *aor2; dd = diff;
	*dd = NULL;
	while (nr1!=NULL && or2!=NULL) {
		if (POSITION_EQ(nr1->p, or2->p)) {
			nr1 = nr1->next; or2=or2->next;
		} else {
			if (SORTED_LIST_LESS(nr1, *or2)) {
				*dd = olcxCopyReference(nr1);
				dd = &(*dd)->next;
				nr1=nr1->next;
			} else {
				*dd = olcxCopyReference(or2);
				dd = &(*dd)->next;
				or2=or2->next;
			}
		}
	}
	if (nr1!=NULL || or2!=NULL) {
		if (nr1!=NULL) {
			r = nr1;
			mode = DIFF_UNEXPECTED_REF;
		} else {
			r = or2;
			mode = DIFF_MISSING_REF;
		}
		for(; r!=NULL; r=r->next) {
			*dd = olcxCopyReference(r);
			dd = &(*dd)->next;
		}
	}
}

static void safetyCheckDiff(S_reference **anr1, 
							S_reference **aor2,
							S_olcxReferences *diffrefs
	) {
	S_reference *r, *nr1, *or2;
	int mode;
	LIST_MERGE_SORT(S_reference, *anr1, olcxReferenceInternalLessFunction);
	LIST_MERGE_SORT(S_reference, *aor2, olcxReferenceInternalLessFunction);
	nr1 = *anr1; or2 = *aor2;
	while (nr1!=NULL && or2!=NULL) {
		if (nr1->p.file==or2->p.file && nr1->p.line==or2->p.line) {
			nr1 = nr1->next; or2=or2->next;
		} else {
			if (SORTED_LIST_LESS(nr1, *or2)) {
				safetyCheckAddDiffRef(nr1, diffrefs, DIFF_UNEXPECTED_REF);
				nr1=nr1->next;
			} else {
				safetyCheckAddDiffRef(or2, diffrefs, DIFF_MISSING_REF);
				or2=or2->next;
			}
		}
	}
	if (nr1!=NULL || or2!=NULL) {
		if (nr1!=NULL) {
			r = nr1;
			mode = DIFF_UNEXPECTED_REF;
		} else {
			r = or2;
			mode = DIFF_MISSING_REF;
		}
		for(; r!=NULL; r=r->next) {
			safetyCheckAddDiffRef(r, diffrefs, mode);
		}
	}
	diffrefs->act = diffrefs->r;
	if (diffrefs->r!=NULL) {
		assert(diffrefs->menuSym);
		olcxAddReferencesToOlSymbolsMenu(diffrefs->menuSym, diffrefs->r, 0);
	}
}

int getFileNumberFromName(char *name) {
	char        *nn;
	S_fileItem  dd;
	int			ii;
	nn = normalizeFileName(name, s_cwd);
	FILLF_fileItem(&dd,nn, 0, 0,0,0, 0,0,0,0,0,0,0,0,0,s_noneFileIndex,
				   NULL,NULL,s_noneFileIndex,NULL);
	if (idTabIsMember(&s_fileTab, &dd, &ii)) {
		return(ii);
	} else {
		return(s_noneFileIndex);
	}
}

static S_reference *olcxCreateFileShiftedRefListForCheck(S_reference *rr) {
	S_reference *res, *r, *tt, **resa;
	int ofn, nfn, fmline, lmline;
//&fprintf(dumpOut,"!shifting enter\n");
	if (s_opt.checkFileMovedFrom==NULL) return(NULL);
	if (s_opt.checkFileMovedTo==NULL) return(NULL);
//&fprintf(dumpOut,"!shifting %s --> %s\n", s_opt.checkFileMovedFrom, s_opt.checkFileMovedTo);
	ofn = getFileNumberFromName(s_opt.checkFileMovedFrom);
	nfn = getFileNumberFromName(s_opt.checkFileMovedTo);
//&fprintf(dumpOut,"!shifting %d --> %d\n", ofn, nfn);
	if (ofn==s_noneFileIndex) return(NULL);
	if (nfn==s_noneFileIndex) return(NULL);
	fmline = s_opt.checkFirstMovedLine;
	lmline = s_opt.checkFirstMovedLine + s_opt.checkLinesMoved;
	res = NULL; resa = &res;
	for(r=rr; r!=NULL; r=r->next) {
		OLCX_ALLOC(tt, S_reference);
		*tt = *r;
		if (tt->p.file==ofn && tt->p.line>=fmline && tt->p.line<lmline) {
			tt->p.file = nfn;
//&fprintf(dumpOut,"!shifting %d:%d to %d (%d %d %d)\n", tt->p.file, tt->p.line, s_opt.checkNewLineNumber + (tt->p.line - fmline), s_opt.checkFirstMovedLine, s_opt.checkLinesMoved, s_opt.checkNewLineNumber);
			tt->p.line = s_opt.checkNewLineNumber + (tt->p.line - fmline);
		}
		*resa=tt; tt->next=NULL; resa= &(tt->next);
	}
	LIST_MERGE_SORT(S_reference, res, olcxReferenceInternalLessFunction);
	return(res);
}

static void olcxSafetyCheck2() {
	S_olcxReferences *refs, *origrefs, *newrefs, *diffrefs;
	S_reference *rr,*rrd, *shifted;
	int pbflag=0;
	origrefs = newrefs = diffrefs = NULL;
	SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
	assert(origrefs && newrefs && diffrefs);
	if (pbflag) return;
	shifted = olcxCreateFileShiftedRefListForCheck(origrefs->r);
	if (shifted != NULL) {
		safetyCheckDiff(&newrefs->r, &shifted, diffrefs);
		olcxFreeReferences(shifted);
	} else {
		safetyCheckDiff(&newrefs->r, &origrefs->r, diffrefs);
	}
	if (diffrefs->r == NULL) {
		// no need to free here, as popings are not freed
		s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;
		s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;
		s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;
 		fprintf(ccOut, "*Done. No conflicts detected.");
	} else {
		assert(diffrefs->menuSym);
		s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;
 		fprintf(ccOut, " ** Some misinterpreted references detected. Please, undo last refactoring.");
	}
	fflush(ccOut);
}

static int olRemoveCallerReference(S_olcxReferences *refs) {
  S_reference *rr, **rrr;
  LIST_MERGE_SORT(S_reference, refs->r, olcxReferenceInternalLessFunction);
  for(rrr= &refs->r, rr=refs->r; rr!=NULL; rrr= &rr->next, rr=rr->next){
//&fprintf(dumpOut,"checking %d %d %d to %d %d %d\n",rr->p.file, rr->p.line,rr->p.coll, refs->cpos.file,  refs->cpos.line,  refs->cpos.coll);
	  if (! POSITION_LESS(rr->p, refs->cpos)) break; 
  }
  if (rr == NULL) return(0);
  if (! IS_DEFINITION_OR_DECL_USAGE(rr->usg.base)) return(0);
//&fprintf(dumpOut,"!removing reference on %d\n", rr->p.line);
  *rrr = rr->next;
  OLCX_FREE_REFERENCE(rr);
  return(1);
}

static void olEncapsulationSafetyCheck() {
	S_olcxReferences *refs;
	assert(s_olcxCurrentUser);
	refs = s_olcxCurrentUser->browserStack.top;
	if (refs==NULL || refs->previous==NULL){
		error(ERR_INTERNAL,"something goes wrong at encapsulate safety check");
		return;
	}
	// remove definition reference, so they do not interfere
	assert(s_olcxCurrentUser->browserStack.top!=NULL && s_olcxCurrentUser->browserStack.top->previous!=NULL
		   && s_olcxCurrentUser->browserStack.top->previous->previous!=NULL
		   && s_olcxCurrentUser->browserStack.top->previous->previous->previous!=NULL);
	olRemoveCallerReference(s_olcxCurrentUser->browserStack.top);
	olRemoveCallerReference(s_olcxCurrentUser->browserStack.top->previous);
	olRemoveCallerReference(s_olcxCurrentUser->browserStack.top->previous->previous->previous);
	// join references from getter and setter and make regular safety check
	olcxAddReferences(refs->r, &s_olcxCurrentUser->browserStack.top->previous->r, 
					  ANY_FILE, 0);
	s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;
	olcxSafetyCheck2();
}

static void olcxResetSuffix() {
	S_userOlcx *user;
	S_olcxReferences *refs;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs,CHECK_NULL);
	strncpy(refs->refsuffix, s_opt.olcxRefSuffix, MAX_OLCX_SUFF_SIZE-1);
	refs->refsuffix[MAX_OLCX_SUFF_SIZE-1] = 0;
	fprintf(ccOut,"* refs %s", refs->refsuffix);
}

static void olCompletionSelect() {
	S_userOlcx 			*user;
	S_olcxReferences 	*refs;
	S_olCompletion		*rr;
	OLCX_MOVE_INIT(s_olcxCurrentUser,refs, CHECK_NULL);
	rr = olCompletionNthLineRef(refs->cpls, s_opt.olcxGotoVal);
	if (rr==NULL) {
		error(ERR_ST, "selection out of range.");
		return;
	}
	if (s_opt.xref2) {
		assert(s_olcxCurrentUser->completionsStack.root!=NULL);
		ppcGenGotoPositionRecord(&s_olcxCurrentUser->completionsStack.root->cpos);
		if (rr->csymType==TypeNonImportedClass) {
			ppcGenRecord(PPC_FQT_COMPLETION, rr->name, "\n");
		} else {
			ppcGenRecord(PPC_SINGLE_COMPLETION, rr->name, "\n");
		}
	} else {
		generateOnlineCxref(&refs->cpos, COLCX_GOTO_REFERENCE, UsageUsed,
							refs->refsuffix, rr->name);
	}
//&	olStackDeleteSymbol( refs);
}

static void olcxReferenceSelectTagSearchItem(int refn) {
	S_olCompletion		*rr,*rrr;
	S_symbolRefItemBits	sb;
	int 				i,jdoc;
	S_olcxReferences 	*refs;
	char				ttt[MAX_FUN_NAME_SIZE];
	assert(refn > 0);
	assert(s_olcxCurrentUser);
	assert(s_olcxCurrentUser->retrieverStack.top);
	refs = s_olcxCurrentUser->retrieverStack.top;
	rr = olCompletionNthLineRef(refs->cpls, refn);
	if (rr == NULL) {
		error(ERR_ST, "selection out of range.");
		return;
	}
	assert(s_olcxCurrentUser->retrieverStack.root!=NULL);
	ppcGenGotoPositionRecord(&s_olcxCurrentUser->retrieverStack.root->cpos);
	sprintf(ttt, " %s", rr->name);
	ppcGenRecord(PPC_SINGLE_COMPLETION, ttt, "\n");
}

static void olCompletionBack() {
	S_olcxReferences 	*top;
	assert(s_olcxCurrentUser);
	top = s_olcxCurrentUser->completionsStack.top;
	if (top != NULL && top->previous != NULL) {
		s_olcxCurrentUser->completionsStack.top = s_olcxCurrentUser->completionsStack.top->previous;
		ppcGenGotoPositionRecord(&s_olcxCurrentUser->completionsStack.top->cpos);
		printCompletionsList(0);
	}
}

static void olCompletionForward() {
	S_olcxReferences 	*top;
	assert(s_olcxCurrentUser);
	top = getNextTopStackItem(&s_olcxCurrentUser->completionsStack);
	if (top != NULL) {
		s_olcxCurrentUser->completionsStack.top = top;
		ppcGenGotoPositionRecord(&s_olcxCurrentUser->completionsStack.top->cpos);
		printCompletionsList(0);
	}
}

static void olcxNoSymbolFoundErrorMessage() {
	if (s_opt.cxrefs == OLO_PUSH_NAME || s_opt.cxrefs == OLO_PUSH_SPECIAL_NAME) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_ERROR,"No symbol found.", "\n");
		} else {
			fprintf(ccOut,"*** No symbol found.");
		}
	} else {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_ERROR,"No symbol found, please position the cursor on a program symbol.", "\n");
		} else {
			fprintf(ccOut,"*** No symbol found, please position the cursor on a program symbol.");
		}
	}
}


static int olcxCheckSymbolExists() {
	if (s_olcxCurrentUser!=NULL 
		&& s_olcxCurrentUser->browserStack.top!=NULL
		&& s_olcxCurrentUser->browserStack.top->menuSym==NULL
		) {
		return(0);
	}
	return(1);
}

static S_olSymbolsMenu *firstVisibleSymbol( S_olSymbolsMenu *first ) {
	S_olSymbolsMenu *ss, *fvisible;
	fvisible = NULL;
	for(ss=first; ss!=NULL; ss=ss->next) {
		if (ss->visible) {
			fvisible = ss;
			break;
		}
	}
	return(fvisible);
}

static int staticallyLinkedSymbolMenu(S_olSymbolsMenu *menu) {
	S_olSymbolsMenu *fv;
	fv = firstVisibleSymbol(menu);
	if (JAVA_STATICALLY_LINKED(fv->s.b.storage,fv->s.b.accessFlags)) { 
		return(1);
	} else {
		return(0);
	}
}

int olcxShowSelectionMenu() {
	S_olSymbolsMenu	*ss, *first, *fvisible;
	char *fname;
	// decide whether to show manual resolution menu
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	if (s_opt.cxrefs == OLO_PUSH_FOR_LOCALM) {
		// never ask for resolution for local motion symbols
		return(0);
	}
	if (s_opt.cxrefs == OLO_SAFETY_CHECK2) {
		// safety check showing of menu is resolved by safetyCheck2ShouldWarn
		return(0);
	}
	// first if just zero or one symbol, no resolution
	first = s_olcxCurrentUser->browserStack.top->menuSym;
	if (first == NULL) {
//&fprintf(dumpOut,"no resolve, no symbol\n"); fflush(dumpOut);
		return(0); // no symbol
	}
	fvisible = firstVisibleSymbol( first);
	if (fvisible==NULL) {
//&fprintf(dumpOut,"no resolve, no visible\n"); fflush(dumpOut);
		return(0); // no visible
	}
	first = NULL;
	if (s_opt.cxrefs==OLO_PUSH 
		|| s_opt.cxrefs==OLO_PUSH_ONLY
		|| s_opt.cxrefs==OLO_PUSH_AND_CALL_MACRO
		|| s_opt.cxrefs==OLO_RENAME 
		|| s_opt.cxrefs==OLO_ARG_MANIP
		|| s_opt.cxrefs==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
		|| JAVA_STATICALLY_LINKED(fvisible->s.b.storage, 
								  fvisible->s.b.accessFlags)) {
		// manually only if different 
		for(ss=s_olcxCurrentUser->browserStack.top->menuSym; ss!=NULL; ss=ss->next) {
			if (ss->selected) {
				if (first == NULL) {
					first = ss;
				} else if ( (! itIsSameCxSymbol(&first->s, &ss->s))
							|| first->s.vFunClass!=ss->s.vFunClass) {
					return(1);
				}
			}
		}
	} else {
		for(ss=s_olcxCurrentUser->browserStack.top->menuSym; ss!=NULL; ss=ss->next) {
			if (ss->visible) {
				if (first!=NULL) {
					return(1);
				}
				first = ss;
			}
		}
	}
//&fprintf(dumpOut,"no resolve, default value\n"); fflush(dumpOut);
	return(0);
}

static int countSelectedItems(S_olSymbolsMenu *menu) {
	S_olSymbolsMenu *ss;
	int res;
	res = 0;
	for(ss=menu; ss!=NULL; ss=ss->next) {
		if (ss->selected) res++;
	}
	return(res);
}

static int linkNamesHaveSameProfile(char *n1, char *n2) {
	char *p1, *p2;
	int diff;
	p1 = strchr(n1, '(');
	p2 = strchr(n2, '(');
	if (p1==NULL && p2==NULL) return(1);
	if (p1==NULL || p2==NULL) return(0);
	diff = strcmp(p1, p2);
	return(diff == 0);
}

static int refactoringLinkNameCorrespondance(char *n1, char *n2, int command) {
	// this should say if there is refactoring link names correspondance 
	// between n1 and n2
//&fprintf(dumpOut,"checking lnc between %s %s on %s\n", n1, n2, olcxOptionsName[command]);
	//& if (command == OLO_RENAME) return(linkNamesHaveSameProfile(n1, n2));
	//& if (command == OLO_ENCAPSULATE) return(linkNamesHaveSameProfile(n1, n2));
	//& if (command == OLO_ARG_MANIP) return(linkNamesHaveSameProfile(n1, n2));
	//& if (command == OLO_VIRTUAL2STATIC_PUSH) return(linkNamesHaveSameProfile(n1, n2));
	// TODO!! other cases (argument manipulations) not yet implemented
	// they are all as OLO_RENAME for now !!!
	return(1); 
}

static S_olSymbolsMenu *safetyCheck2FindCorrMenuItem(S_olSymbolsMenu *item,
													 S_olSymbolsMenu *menu,
													 int command) {
	S_olSymbolsMenu *ss;
	for(ss=menu; ss!=NULL; ss=ss->next) {
		if (ss->selected
			&& ss->s.vApplClass == item->s.vApplClass
			&& ss->s.b.symType == item->s.b.symType
			&& ss->s.b.storage == item->s.b.storage
			&& refactoringLinkNameCorrespondance(ss->s.name, item->s.name, 
												 command)) {
			// here it is
			return(ss);
		}
	}
	return(NULL);
}

static int scCompareVirtualHierarchies(S_olSymbolsMenu *origm, 
									   S_olSymbolsMenu *newm,
									   int command) {
	S_olSymbolsMenu *ss, *oss;
	int count1, count2;
	int res;
	res = 0;
	count1 = countSelectedItems(origm);
	count2 = countSelectedItems(newm);
//&fprintf(dumpOut,":COMPARING\n");olcxPrintSelectionMenu(origm);fprintf(dumpOut,":TO\n");olcxPrintSelectionMenu(newm);fprintf(dumpOut,":END\n");
	if (count1 != count2) res = 1;
	for(ss=newm; ss!=NULL; ss=ss->next) {
		if (ss->selected) {
			oss = safetyCheck2FindCorrMenuItem(ss, origm, command);
			if (oss==NULL || oss->selected == 0) {
				res = 1;
				ss->selected = 0;
			}
		}
	}
	return(res);
}

int safetyCheck2ShouldWarn() {
	int res,problem;
	S_olcxReferences *refs, *origrefs, *newrefs, *diffrefs;
	problem = 0;
	if (s_opt.cxrefs != OLO_SAFETY_CHECK2) return(0);
	if (! LANGUAGE(LAN_JAVA)) return(0);
	// compare hierarchies and deselect diff
	origrefs = newrefs = diffrefs = NULL;
	SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, problem);
	if (problem) return(0);
	assert(origrefs && newrefs && diffrefs);
	if (staticallyLinkedSymbolMenu(origrefs->menuSym)) return(0);
	res = scCompareVirtualHierarchies(origrefs->menuSym, newrefs->menuSym,
									  origrefs->command);
	if (res) {
		// hierarchy was changed, recompute refs
		olcxRecomputeSelRefs(newrefs);
	}
	return(res);
}

static int olMenuHashFileNumLess(S_olSymbolsMenu *s1, S_olSymbolsMenu *s2) {
	int fi1, fi2;
	fi1 = cxFileHashNumber(s1->s.name);
	fi2 = cxFileHashNumber(s2->s.name);
	if (fi1 < fi2) return(1);
	if (fi1 > fi2) return(0);
	if (s1->s.b.category == CatLocal) return(1);
	if (s1->s.b.category == CatLocal) return(0);
	// both files and categories equals ?
	return(0);
}

void getLineColCursorPositionFromCommandLineOption( int *l, int *c ) {
	assert(s_opt.olcxlccursor!=NULL);
	sscanf(s_opt.olcxlccursor,"%d:%d", l, c);
}

int getClassNumFromClassLinkName(char *name, int defaultResult) {
	int 				ii, clii;
	char				cfname[MAX_FILE_NAME_SIZE];
	S_fileItem			ffi;
	clii = defaultResult;
	SPRINT_FILE_TAB_CLASS_NAME(cfname, name);
//&fprintf(dumpOut,"looking for class %s\n",cfname);fflush(dumpOut);
	FILLF_fileItem(&ffi,cfname, 0, 0,0,0, 0,0,0,0,0,0,0,0,0,
				   s_noneFileIndex,NULL,NULL,s_noneFileIndex,NULL);
	if (idTabIsMember(&s_fileTab, &ffi, &ii)) clii = ii;
	return(clii);
}

static int olSpecialFieldCreateSelection(char *fieldName, int storage) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	S_symbol 			*rt;
	int					ii, clii;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	if (! LANGUAGE(LAN_JAVA)) {
		rstack->hkSelectedSym = NULL;
		error(ERR_ST,"This function is available only in Java language");
		return(s_noneFileIndex);
	}
	assert(s_javaObjectSymbol && s_javaObjectSymbol->u.s);
	clii = s_javaObjectSymbol->u.s->classFile;
//&fprintf(dumpOut,"class clii==%d==%s\n",clii,s_fileTab.tab[clii]->name);fflush(dumpOut);
	ss = rstack->hkSelectedSym;
	if (ss!=NULL && ss->next!=NULL) {
		// several cursor selected fields
		// probably constructor, look for a class type
		if (ss->next->s.b.symType == TypeStruct) ss = ss->next;
	}
	if (ss != NULL) {
//&fprintf(dumpOut, "sym %s of %s\n", ss->s.name, typesName[ss->s.b.symType]);
		if (ss->s.b.symType == TypeStruct) {
			clii = getClassNumFromClassLinkName(ss->s.name, clii);
		} else {
			if (s_opt.cxrefs == OLO_CLASS_TREE) {
				assert(s_olcxCurrentUser->ct.baseClassIndex!=s_noneFileIndex);
				clii = s_olcxCurrentUser->ct.baseClassIndex;
			} else {
				if (ss->s.vApplClass!=s_noneFileIndex) clii = ss->s.vApplClass;
			}
		}
	}
//&fprintf(dumpOut,"class clii==%d==%s\n",clii,s_fileTab.tab[clii]->name);fflush(dumpOut);
	olcxFreeResolutionMenu(ss);
	rstack->hkSelectedSym = olCreateSpecialMenuItem(fieldName, clii, storage);
	return(clii);
}


void olCreateSelectionMenu(int command) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	int					fnum;

	// I think this ordering is useless
	LIST_MERGE_SORT(S_olSymbolsMenu,
					s_olcxCurrentUser->browserStack.top->hkSelectedSym,
					htmlRefItemsOrderLess);
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	if (ss == NULL) return;
	renameCollationSymbols(ss);
	LIST_SORT(S_olSymbolsMenu, rstack->hkSelectedSym, olMenuHashFileNumLess);
	ss = rstack->hkSelectedSym;
	while (ss!=NULL) {
		readOneAppropReferenceFile(ss->s.name, s_cxSymbolMenuCreationTab);
		fnum = cxFileHashNumber(ss->s.name);
//&fprintf(dumpOut,"file %d readed\n", fnum);
		while (ss!=NULL && fnum==cxFileHashNumber(ss->s.name)) ss = ss->next;
	}
	refTabMap(&s_cxrefTab, mapCreateSelectionMenu);
	refTabMap(&s_cxrefTab, putOnLineLoadedReferences);
	setSelectedVisibleItems(rstack->menuSym, command, rstack->menuFilterLevel);
	assert(rstack->r==NULL);
	olProcessSelectedReferences(rstack, genOnLineReferences);
	// isn't ordering useless ?
	LIST_MERGE_SORT(S_olSymbolsMenu,
					s_olcxCurrentUser->browserStack.top->menuSym,
					htmlRefItemsOrderLess);
}

#if ZERO
static void olLoadSelectionMenu() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	int					fnum;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->menuSym;
	if (ss == NULL) return;
	LIST_SORT(S_olSymbolsMenu, rstack->menuSym, olMenuHashFileNumLess);
	ss = rstack->menuSym;
	while (ss!=NULL) {
		readOneAppropReferenceFile(ss->s.name, s_cxSymbolLoadMenuRefs);
		fnum = cxFileHashNumber(ss->s.name);
//&fprintf(dumpOut,"file %d readed\n", fnum);
		while (ss!=NULL && fnum==cxFileHashNumber(ss->s.name)) ss = ss->next;
	}
	refTabMap(&s_cxrefTab, putOnLineLoadedReferences);
}
#endif

int refOccursInRefs(S_reference *r, S_reference *list) {
	S_reference *place;
	SORTED_LIST_FIND2(place,S_reference, (*r),list);
	if (place==NULL || SORTED_LIST_NEQ(place, *r)) return(0);
	return(1);
}

static void olcxSingleReferenceCheck1( S_symbolRefItem *p, 
									   S_olcxReferences *rstack, 
									   S_reference *r 
	) {
	S_reference *dr;
	int prefixchar;
	if (refOccursInRefs(r, rstack->r)) {
		prefixchar = ' ';
		if (s_olcxCurrentUser->browserStack.top->r == NULL) {
			fprintf(ccOut,"%s",COLCX_LIST);
			prefixchar = '>';
		}
		fprintf(ccOut,"%c  %s:%d reference to '", prefixchar,
				simpleFileNameFromFileNum(r->p.file), r->p.line);
		printSymbolLinkNameString(ccOut, p->name);
		fprintf(ccOut,"' lost\n");
		olcxAppendReference(r, s_olcxCurrentUser->browserStack.top);
#if ZERO	// for the momet, it is difficult to get def. reference from cxfile
		dr = getDefinitionRef(p->refs);
		if (dr!=NULL && (dr->usg.base==UsageDefined || dr->usg.base==UsageDeclared)){
			fprintf(ccOut, "        defined at %s:%d\n", 
					simpleFileNameFromFileNum(dr->p.file), dr->p.line);
			olcxAppendReference(dr, s_olcxCurrentUser->browserStack.top);
		}
#endif
	}
}

void olcxCheck1CxFileReference(S_symbolRefItem *ss, S_reference *r) {
	S_symbolRefItem 	*sss;
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu 	*cms;
	int					pushed;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top->previous;
	assert(rstack && rstack->menuSym);
	sss = &rstack->menuSym->s;
//&fprintf(dumpOut,"checking 1 file refs %s at %s:%d\n", ss->name,simpleFileNameFromFileNum(r->p.file),r->p.line);
	pushed = itIsSymbolToPushOlRefences(ss, rstack, &cms, DEFAULT_VALUE);
	// this is very slow to check the symbol name for each reference
	if ((!pushed) && olcxItIsSameCxSymbol(ss, sss)) {
		olcxSingleReferenceCheck1(ss, rstack, r);
	}
}

static void olcxProceedSafetyCheck1OnInloadedRefs(S_olcxReferences *rstack, S_olSymbolsMenu *ccms) {
	S_symbolRefItem  	*p;
	S_symbolRefItem		*sss;
	S_reference 		*r, *dr;
	S_olSymbolsMenu 	*cms;
	int					pushed;
	p = &ccms->s;
	assert(rstack && rstack->menuSym);
	sss = &rstack->menuSym->s;
	pushed = itIsSymbolToPushOlRefences(p, rstack, &cms, DEFAULT_VALUE);
	// TODO, this can be simplified, as ccms == cms.
//&fprintf(dumpOut,":checking %s to %s (%d)\n",p->name, sss->name, pushed);
	if ((! pushed) && olcxItIsSameCxSymbol(p, sss)) {
//&fprintf(dumpOut,"checking %s references\n",p->name); 
		for(r=p->refs; r!=NULL; r=r->next) {
			olcxSingleReferenceCheck1( p, rstack, r);
		}
	}
}

void olcxPushSpecialCheckMenuSym(int pushCommand,char *symname) {
	S_olcxReferences  	*rstack;
	olcxSetCurrentUser(s_opt.user);
	olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	rstack->hkSelectedSym = olCreateSpecialMenuItem(
		symname, s_noneFileIndex, StorageDefault);
	rstack->menuSym = olCreateSpecialMenuItem(
		symname, s_noneFileIndex, StorageDefault);
}

static void olcxSafetyCheckInit() {
	S_olcxReferences  	*rstack;
	assert(s_opt.cxrefs == OLO_SAFETY_CHECK_INIT);
	olcxPushSpecialCheckMenuSym(s_opt.cxrefs,LINK_NAME_SAFETY_CHECK_MISSED);
	fprintf(ccOut,"* safety checks initialized");
	fflush(ccOut);
}

static S_olSymbolsMenu *mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(
	S_olSymbolsMenu *menuSym) {
	S_olSymbolsMenu *mm,*res;
	res = NULL;
	for(mm=menuSym; mm!=NULL; mm=mm->next) {
		if (mm->s.refs!=NULL && IS_DEFINITION_OR_DECL_USAGE(mm->s.refs->usg.base) &&
			(res==NULL || POSITION_LESS(mm->s.refs->p, res->s.refs->p))) {
				res = mm;
		}
	}
	return(res);
}

static S_olSymbolsMenu *mmFindSymWithCorrespondingRef(S_reference *ref,
													  S_olSymbolsMenu *osym,
													  S_olcxReferences *refs,
													  S_position *moveOffset
													  ) {
	S_reference 		sr, *rr, *place;
	S_olSymbolsMenu 	*mm;
	sr = *ref;
	POSITION_PLUS(sr.p, ref->p, *moveOffset);
	// now looks for the reference 'r'
	for(mm=refs->menuSym; mm!=NULL; mm=mm->next) {
		// do not check anything, but symbol type to avoid missresolution
		// of ambiguous references class <-> constructor
//&fprintf(dumpOut,";looking for correspondance %s <-> %s\n", osym->s.name,mm->s.name);
		if (mm->s.b.symType != osym->s.b.symType
			&& mm->s.b.symType != TypeInducedError) continue;
		// check also that maybe this references are not mixed with regular
		//&if (mm->s.name[0]==' ' || osym->s.name[0]==' ') {
		//&	if (mm->s.name[0]!=' ' || osym->s.name[0]!=' ') continue;
		//&}
//&fprintf(dumpOut,";looking references\n");
		SORTED_LIST_FIND2(place,S_reference, sr, mm->s.refs);
		if (place!=NULL && ! SORTED_LIST_NEQ(place, sr)) {
			// I have got it
//&fprintf(dumpOut,";I have got it!\n");
			return(mm);
		}
	}
	return(NULL);
}

int symbolsCorrespondWrtMoving(S_olSymbolsMenu *osym,
								   S_olSymbolsMenu *nsym,
								   int command) {
	int res;
	res = 0;
	switch (command) {
	case OLO_MM_PRE_CHECK:
		if (itIsSameCxSymbol(&osym->s, &nsym->s)
			&& osym->s.vApplClass == nsym->s.vApplClass) {
			res = 1;
		}
		break;
	case OLO_PP_PRE_CHECK:
		if (itIsSameCxSymbol(&osym->s, &nsym->s)) {
			if (osym->s.vApplClass == nsym->s.vApplClass) {
				res = 1;
			}
			if (osym->s.b.storage == StorageField
				&& osym->s.vFunClass == nsym->s.vFunClass) {
				res = 1;
			}
			if (osym->s.b.storage == StorageMethod) {
				res = 1;
			}
		}
		break;
	default:
		assert(0);
	}
	// do not report misinterpretations induced by previous errors
	if (nsym->s.b.symType == TypeInducedError) res = 1;
//&fprintf(dumpOut,";checking corr. %s %s res==%d\n", osym->s.name, nsym->s.name, res);
	return(res);
}

static int mmPreCheckMakeDifference(S_olcxReferences *origrefs,
									S_olcxReferences *newrefs,
									S_olcxReferences *diffrefs
	) {
	S_olSymbolsMenu *osym, *nsym, *diffsym, *ofirstsym, *nfirstsym;
	S_reference		*rr, *nsymrefs, *nrr, *drr, *ofirstref, *nfirstref;
	S_position 		moveOffset;
	int				rfound;
	FILL_position(&moveOffset, 0,0,0);
	ofirstsym = mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(origrefs->menuSym);
	nfirstsym = mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(newrefs->menuSym);
	if (ofirstsym!=NULL && nfirstsym!=NULL) {
		// TODO! Check here rather symbol name, then just column offsets
		assert(ofirstsym->s.refs && nfirstsym->s.refs);
		POSITION_MINUS(moveOffset,nfirstsym->s.refs->p,ofirstsym->s.refs->p);
//&fprintf(dumpOut,"!ofirstsym, nfirstsym == %s %s at %d,%d %d,%d\n", ofirstsym->s.name, nfirstsym->s.name, ofirstsym->s.refs->p.line, ofirstsym->s.refs->p.coll, nfirstsym->s.refs->p.line, nfirstsym->s.refs->p.coll);
		if (moveOffset.coll!=0) {
			error(ERR_ST, "method has to be moved into an empty line");
			return(1);
		}
	}
//&fprintf(dumpOut,": line Offset == %d\n", lineOffset);
	for(osym = origrefs->menuSym; osym!=NULL; osym=osym->next) {
//&fprintf(dumpOut,";check on %s\n", osym->s.name);
		// do not check recursive calls
		if (osym == ofirstsym) goto cont;
		// nor local variables
		if (osym->s.b.storage == StorageAuto) goto cont;
		// nor labels 
		if (osym->s.b.symType == TypeLabel) goto cont;
		// do not check also any symbols from classes defined in inner scope
		if (isStrictlyEnclosingClass(osym->s.vFunClass, ofirstsym->s.vFunClass)) goto cont;
		// (maybe I should not test any local symbols ???)

		// check the symbol
		diffsym = NULL;
		for(rr=osym->s.refs; rr!=NULL; rr=rr->next) {
			nsym = mmFindSymWithCorrespondingRef(rr,osym,newrefs,&moveOffset);
			if (nsym==NULL || ! symbolsCorrespondWrtMoving(osym, nsym, s_opt.cxrefs)) {
				if (diffsym == NULL) {
					diffsym = olAddBrowsedSymbol(
						&osym->s, &diffrefs->menuSym, 1, 1, 
						(OOC_PROFILE_EQUAL|OOC_VIRT_SAME_FUN_CLASS),
						USAGE_ANY, 0, &s_noPos, UsageNone);
				}
				olcxAddReferenceToOlSymbolsMenu(diffsym, rr, 0);
			}
		}
	cont:;
	}
	return(0);
}

static void olcxMMPreCheck() {
	S_olcxReferences  	*diffrefs, *origrefs, *newrefs;
	S_symbolRefItem		dri;
	int		precheck;
	olcxSetCurrentUser(s_opt.user);
	olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	assert(s_opt.cxrefs == OLO_MM_PRE_CHECK || s_opt.cxrefs == OLO_PP_PRE_CHECK);
	diffrefs = s_olcxCurrentUser->browserStack.top;
	assert(diffrefs && diffrefs->previous && diffrefs->previous->previous);
	newrefs = diffrefs->previous;
	origrefs = newrefs->previous;
	precheck = mmPreCheckMakeDifference(origrefs, newrefs, diffrefs);
	olStackDeleteSymbol(origrefs);
	olStackDeleteSymbol(newrefs);
	if (precheck==0) {
		if (diffrefs->menuSym!=NULL) {
			fillTrivialSpecialRefItem(&dri, "  references missinterpreted after refactoring");
			olAddBrowsedSymbol(&dri, &diffrefs->hkSelectedSym, 1, 1, 0,
				USAGE_ANY, 0, &s_noPos, UsageNone);
			olProcessSelectedReferences(diffrefs, genOnLineReferences);
			//&olcxPrintSelectionMenu(diffrefs->menuSym);
			olcxPrintRefList(";", diffrefs);
		} else {
			olStackDeleteSymbol(diffrefs);
			fprintf(ccOut,"* Method moving pre-check passed.");
		}
	}
	fflush(ccOut);
}

#ifdef OOOOOOOOOORRRRRRRRRRRRRRIIIIIIIIGGGGGGGGGGG
static S_olSymbolsMenu *mmPreCheckFindCorrespondingSym(
		S_olSymbolsMenu *sym,
		S_olcxReferences *rstack
	) {
	S_olSymbolsMenu *ss;
	for(ss=rstack->menuSym; ss!=NULL; ss=ss->next) {
		if (itIsSameCxSymbol(&ss->s, &sym->s)
			&& ss->s.vApplClass == sym->s.vApplClass) {
			return(ss);
		}
	}
	return(NULL);
}

static int mmRefOccursInRefs(S_reference *r, S_reference *list, 
							 S_position *offset) {
	S_reference 	rr;
	int				res;
	rr = *r;
	POSITION_PLUS(rr.p, r->p, *offset);
	res = refOccursInRefs(&rr, list);
	return(res);
}

static int mmPreCheckMakeDifference(S_olcxReferences *origrefs,
									S_olcxReferences *newrefs,
									S_olcxReferences *diffrefs
	) {
	S_olSymbolsMenu *osym, *nsym, *diffsym, *ofirstsym, *nfirstsym;
	S_reference		*rr, *nsymrefs, *nrr, *drr, *ofirstref, *nfirstref;
	S_position 		moveOffset;
	FILL_position(&moveOffset, 0,0,0);
	ofirstsym = mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(origrefs->menuSym);
	nfirstsym = mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(newrefs->menuSym);
	if (ofirstsym!=NULL && nfirstsym!=NULL) {
		assert(ofirstsym->s.refs && nfirstsym->s.refs);
		POSITION_MINUS(moveOffset,nfirstsym->s.refs->p,ofirstsym->s.refs->p);
		if (moveOffset.coll!=0) {
			error(ERR_ST, "method has to be moved into an empty line");
			return(1);
		}
	}
//&fprintf(dumpOut,": line Offset == %d\n", lineOffset);
	for(osym = origrefs->menuSym; osym!=NULL; osym=osym->next) {
		// do not check recursive calls and local variables
		if (osym == ofirstsym || osym->s.b.storage==StorageAuto) goto cont;
		// check the symbol
		nsym = mmPreCheckFindCorrespondingSym(osym, newrefs);
		if (nsym==NULL) nsymrefs = NULL;
		else nsymrefs = nsym->s.refs;
		diffsym = NULL;
		// check each reference
		for(rr=osym->s.refs; rr!=NULL; rr=rr->next) {
			if (! mmRefOccursInRefs(rr, nsymrefs, &moveOffset)) {
				if (diffsym == NULL) {
					diffsym = olAddBrowsedSymbol(
						&osym->s, &diffrefs->menuSym, 1, 1, 
						(OOC_PROFILE_EQUAL|OOC_VIRT_SAME_FUN_CLASS),
						USAGE_ANY, 0, &s_noPos, UsageNone);
				}
				addRefToOlSymbolMenu(diffsym, rr, 0);
			}
		}
	cont:;
	}
	return(0);
}

static void olcxMMPreCheck() {
	S_olcxReferences  	*diffrefs, *origrefs, *newrefs;
	int		precheck;
	olcxPushUser();
	assert(s_olcxCurrentUser && s_olcxCurrentUser->tops);
	assert(s_opt.cxrefs == OLO_MM_PRE_CHECK);
	diffrefs = s_olcxCurrentUser->tops;
	assert(diffrefs && diffrefs->previous && diffrefs->previous->previous);
	newrefs = diffrefs->previous;
	origrefs = newrefs->previous;
	precheck = mmPreCheckMakeDifference(origrefs, newrefs, diffrefs);
	olStackDeleteSymbol(origrefs);
	olStackDeleteSymbol(newrefs);
	if (precheck==0) {
		if (diffrefs->menuSym!=NULL) {
			olProcessSelectedReferences(diffrefs, genOnLineReferences);
			//&olcxPrintSelectionMenu(diffrefs->menuSym);
			olcxPrintRefList(";", diffrefs);
		} else {
			olStackDeleteSymbol(diffrefs);
			fprintf(ccOut,"* Method moving pre-check passed.");
		}
	}
	fflush(ccOut);
}
#endif


static void olcxSafetyCheck1() {
	S_olcxReferences  	*rstack;
	// in reality this is a hack, it takes references kept from
	// last file processing
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	assert(s_olcxCurrentUser->browserStack.top->previous);
	assert(s_opt.cxrefs == OLO_SAFETY_CHECK1);
//&fprintf(dumpOut,":here I am in safety check\n");fflush(dumpOut);
	rstack = s_olcxCurrentUser->browserStack.top->previous;
//&	setRefSuffix();
	olProcessSelectedReferences(rstack, olcxProceedSafetyCheck1OnInloadedRefs);
	if (s_olcxCurrentUser->browserStack.top->r == NULL) {
		fprintf(ccOut,"* check1 passed");
	} else {
		s_olcxCurrentUser->browserStack.top->act = s_olcxCurrentUser->browserStack.top->r;
		fprintf(ccOut," ** Shared references lost. Please, undo last refactoring\n");
	}
	fflush(ccOut);
}

static int refOccursInRefsCompareFileAndLineOnly(S_reference *rr, 
												 S_reference *list
	) {
	S_reference *r;
	for(r=list; r!=NULL; r=r->next) {
		if (r->p.file==rr->p.file && r->p.line==rr->p.line) return(1);
	}
	return(0);
}

static void olcxTopReferencesIntersection() {
	S_olcxReferences  	*top1,*top2;
	S_olcxReferences  	*rstack;
	S_reference			**r1,**r, *r2,*cr,*nr;
	// in reality this is a hack, it takes references kept from
	// last file processing
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	assert(s_olcxCurrentUser->browserStack.top->previous);
	assert(s_opt.cxrefs == OLO_INTERSECTION);
	top1 = s_olcxCurrentUser->browserStack.top;
	top2 = s_olcxCurrentUser->browserStack.top->previous;
	//TODO in linear time, not O(n^2) like now.
	r1 = & top1->r; 
	while (*r1!=NULL) {
		r = r1; r1 = &(*r1)->next;
		if (! refOccursInRefsCompareFileAndLineOnly(*r, top2->r)) {
			// remove the reference
			nr = *r1;
			OLCX_FREE_REFERENCE(*r);
			*r = nr;
			r1 = r;
		}
	}
	top1->act = top1->r;
	fprintf(ccOut,"*");
}

static void olcxRemoveRefWinFromRefList( S_reference **r1, 
										 int wdfile, 
										 S_position *fp, 
										 S_position *tp ) {
	S_reference **r, *cr, *nr;
	while (*r1!=NULL) {
		r = r1; r1 = &(*r1)->next;
		cr = *r;
//&fprintf(dumpOut,"! checking %d:%d\n", cr->p.line, cr->p.coll);
		if (cr->p.file == wdfile 
			&& POSITION_LESS(*fp, cr->p) && POSITION_LESS(cr->p, *tp)) {
			// remove the reference
			nr = *r1;
			OLCX_FREE_REFERENCE(*r);
			*r = nr;
			r1 = r;
//&fprintf(dumpOut,"! removing reference %d:%d\n", cr->p.line, cr->p.coll);
		}
	}
}


static void olcxTopReferencesRemoveWindow() {
	S_olcxReferences  	*top1;
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*mm;
	int					wdfile;
	S_position			fp, tp;
	// in reality this is a hack, it takes references kept from
	// last file processing
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	assert(s_olcxCurrentUser->browserStack.top->previous);
	assert(s_opt.cxrefs == OLO_REMOVE_WIN);
	wdfile = getFileNumberFromName(s_opt.olcxWinDelFile);
	FILL_position(&fp,wdfile,s_opt.olcxWinDelFromLine,s_opt.olcxWinDelFromCol);
	FILL_position(&tp,wdfile,s_opt.olcxWinDelToLine,s_opt.olcxWinDelToCol);
	top1 = s_olcxCurrentUser->browserStack.top;
	olcxRemoveRefWinFromRefList(&top1->r, wdfile, &fp, &tp);
	for(mm=top1->menuSym; mm!=NULL; mm=mm->next) {
		olcxRemoveRefWinFromRefList(&mm->s.refs, wdfile, &fp, &tp);
	}
	top1->act = top1->r;
	fprintf(ccOut,"*");
}

char *getXrefEnvironmentValue( char *name ) {
	char *val;
	int i, n;
	val = NULL;
	n = s_opt.setGetEnv.num;
	for(i=0; i<n; i++) {
//&fprintf(dumpOut,"checking (%s) %s\n",s_opt.setGetEnv.name[i], s_opt.setGetEnv.value[i]);
		if (strcmp(s_opt.setGetEnv.name[i], name)==0) {
			val = s_opt.setGetEnv.value[i];
			break;
		}
	}
	return(val);
}

static void olcxProcessGetRequest() {
	char *name, *val;
	int i,n;
	name = s_opt.getValue;
//&fprintf(dumpOut,"![get] looking for %s\n", name);
	val = getXrefEnvironmentValue( name);
	if (val != NULL) {
		// O.K. this is a special case, if input file is given
		// then make additional 'predefined' replacements
		if (s_opt.xref2) {
			ppcGenRecord(PPC_SET_INFO, expandSpecialFilePredefinedVariables_st(val), "\n");
		} else {
			fprintf(ccOut,"*%s", expandSpecialFilePredefinedVariables_st(val));
		}
	} else {
		if (s_opt.xref2) {
			sprintf(tmpBuff,"No \"-set %s <command>\" option specified for active project", name);
		} else {
			sprintf(tmpBuff,"No \"-set %s <command>\" option specified for active project", name);
		}
		error(ERR_ST, tmpBuff);
	}
}

void olcxPrintPushingAction(int opt, int afterMenu) {
	switch(opt) {
	case OLO_PUSH:
		if (olcxCheckSymbolExists()) {
			olcxOrderRefsAndGotoDefinition(afterMenu);
		} else {
			// to auto repush symbol by name, but I do not like it.
			//& if (s_opt.xref2) ppcGenRecord(PPC_NO_SYMBOL, "", "\n");
			//& else
				olcxNoSymbolFoundErrorMessage();
			olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
		}
		break;
	case OLO_PUSH_NAME:
		if (olcxCheckSymbolExists()) {
			olcxOrderRefsAndGotoDefinition(afterMenu);
		} else {
			olcxNoSymbolFoundErrorMessage();
			olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
		}
		break;
	case OLO_PUSH_SPECIAL_NAME:
		assert(0);		// called only from refactory
		break;
	case OLO_MENU_GO:
		if (olcxCheckSymbolExists()) {
			olcxOrderRefsAndGotoFirst();
		} else {
			olcxNoSymbolFoundErrorMessage();
			olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
		}
		break;
	case OLO_GLOBAL_UNUSED:
	case OLO_LOCAL_UNUSED:
		// no output for dead code detection ???
		break;
	case OLO_LIST:
		if (olcxCheckSymbolExists()) {
			olcxReferenceList(";");
		} else {
			olcxNoSymbolFoundErrorMessage();
			olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
		}
		break;
	case OLO_USELESS_LONG_NAME: 
	case OLO_USELESS_LONG_NAME_IN_CLASS: 
	case OLO_PUSH_ENCAPSULATE_SAFETY_CHECK:
		olcxPushOnly();
		break;
	case OLO_PUSH_ONLY: 
		if (olcxCheckSymbolExists()) {
			olcxPushOnly();
		} else {
			olcxNoSymbolFoundErrorMessage();
			olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
		}
		break;
	case OLO_PUSH_AND_CALL_MACRO: 
		if (olcxCheckSymbolExists()) {
			olcxPushAndCallMacro();
		} else {
			olcxNoSymbolFoundErrorMessage();
			olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
		}
		break;
	case OLO_PUSH_FOR_LOCALM:
		if (olcxCheckSymbolExists()) olcxPushOnly();
		else olcxNoSymbolFoundErrorMessage();
		break;
	case OLO_MAYBE_THIS:
	case OLO_NOT_FQT_REFS: case OLO_NOT_FQT_REFS_IN_CLASS: 
	case OLO_PUSH_ALL_IN_METHOD:
		if (olcxCheckSymbolExists()) olcxPushOnly();
		else olcxNoSymbolFoundErrorMessage();
		break;
	case OLO_RENAME: case OLO_VIRTUAL2STATIC_PUSH:
	case OLO_ARG_MANIP: case OLO_ENCAPSULATE:
		if (olcxCheckSymbolExists()) olcxRenameInit();
		else olcxNoSymbolFoundErrorMessage();
		break;
	case OLO_SAFETY_CHECK2:
		if (olcxCheckSymbolExists()) olcxSafetyCheck2();
		else olcxNoSymbolFoundErrorMessage();
		break;
	default:
		assert(0);
	}
}

static void olcxCreateClassTree() {
	S_olcxReferences 	*rstack;
	S_olSymbolsMenu		*ss;
	olcxFreeResolutionMenu(s_olcxCurrentUser->ct.tree);
	s_olcxCurrentUser->ct.tree = NULL;
	olSpecialFieldCreateSelection(LINK_NAME_CLASS_TREE_ITEM, StorageMethod);
	s_opt.ooChecksBits = (s_opt.ooChecksBits & ~OOC_VIRTUAL_MASK);
	s_opt.ooChecksBits |= OOC_VIRT_RELATED;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	olCreateSelectionMenu(rstack->command);
	strcpy(s_olcxCurrentUser->ct.refsuffix, rstack->refsuffix);
	s_olcxCurrentUser->ct.tree = rstack->menuSym;
	rstack->menuSym = NULL;
	olcxPrintClassTree(s_olcxCurrentUser->ct.tree);
	// now free special references, which will never be used
	for(ss=s_olcxCurrentUser->ct.tree; ss!=NULL; ss=ss->next) {
		olcxFreeReferences(ss->s.refs);
		ss->s.refs = NULL;
	}
	// delete it as last command, because the top command is tested
	// everywhere.
	olStackDeleteSymbol(rstack);
}

void olcxPushSpecial(char *fieldName, int command) {
	int 				clii, line, col, nn;
	S_olcxReferences 	*refs;
	S_olSymbolsMenu 	*ss,*sss;
	S_position 			callerPos;
	clii = olSpecialFieldCreateSelection(fieldName,StorageField);
	olCreateSelectionMenu(s_olcxCurrentUser->browserStack.top->command);
	assert(s_javaObjectSymbol && s_javaObjectSymbol->u.s);
	if (clii == s_javaObjectSymbol->u.s->classFile 
		|| command == OLO_MAYBE_THIS
		|| command == OLO_NOT_FQT_REFS
		|| command == OLO_NOT_FQT_REFS_IN_CLASS
		) {
		// object, so you have to select all
		refs = s_olcxCurrentUser->browserStack.top;
		for(ss=refs->menuSym; ss!=NULL; ss=ss->next) {
			ss->visible = ss->selected = 1;
		}
		olProcessSelectedReferences(refs, genOnLineReferences);
		getLineColCursorPositionFromCommandLineOption( &line, &col);
		FILL_position(&callerPos, s_input_file_number, line, col);
		olSetCallerPosition(&callerPos);
	}
}

static void olcxListSpecial(char *fieldName) {
	olcxPushSpecial(fieldName, s_opt.cxrefs);
	//&olcxPrintSelectionMenu(s_olcxCurrentUser->browserStack.top->menuSym);
	// previous do not work, because of automatic reduction in moving
	olcxPrintPushingAction(s_opt.cxrefs, DEFAULT_VALUE);
}

int isPushAllMethodsValidRefItem(S_symbolRefItem *ri) {
	if (ri->name[0]!=' ') return(1);
	if (ri->b.symType==TypeInducedError) return(1);
	//& if (strcmp(ri->name, LINK_NAME_MAYBE_THIS_ITEM)==0) return(1);
	return(0);
}

static void olPushAllReferencesInBetweenMapFun(S_symbolRefItem *ri,
											   void *ddd
	) {
	S_reference 			*rr, *defRef;
	S_position 				defpos;
	S_olcxReferences		*rstack;
	S_olSymbolsMenu 		*mm;
	S_pushAllInBetweenData 	*dd;
	int 					defusage,select,visible,ooBits,vlevel;
	dd = (S_pushAllInBetweenData *) ddd;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	if (! isPushAllMethodsValidRefItem(ri)) return;
	for(rr=ri->refs; rr!=NULL; rr=rr->next) {
//&fprintf(dumpOut,"checking %d.%d ref of %s\n", rr->p.line,rr->p.coll,ri->name); fflush(dumpOut);
		if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, dd)) {
			defRef = getDefinitionRef(ri->refs);
			if (defRef!=NULL && IS_DEFINITION_OR_DECL_USAGE(defRef->usg.base)) {
				defpos = defRef->p;
				defusage = defRef->usg.base;
			} else {
				defpos = s_noPos;
				defusage = UsageNone;
			}
			select = visible = 1;
			vlevel = 0;
			ooBits = (OOC_PROFILE_EQUAL | OOC_VIRT_SAME_FUN_CLASS);
//&fprintf(dumpOut,"adding symbol %s\n", ri->name);  fflush(dumpOut);
			mm = olAddBrowsedSymbol(ri, &rstack->menuSym, select, visible, ooBits, USAGE_ANY, vlevel, &defpos, defusage);
			assert(mm!=NULL);
			for(; rr!=NULL; rr=rr->next) {
//&fprintf(dumpOut,"checking reference of line %d, usage %s\n",rr->p.line, usagesName[rr->usg.base]); fflush(dumpOut);
				if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr,dd)) {
					//& olcxAddReferenceToOlSymbolsMenu(mm, rr, 0);	
//&fprintf(dumpOut,"adding reference of line %d\n",rr->p.line); fflush(dumpOut);
					olcxAddReferenceNoUsageCheck(&mm->s.refs, rr, 0);
				}
			}
			goto  fini;
		}
	}
 fini:;
}

void olPushAllReferencesInBetween(int minMemi, int maxMemi) {
	S_pushAllInBetweenData 	rr;
	S_olcxReferences		*rstack;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	FILL_pushAllInBetweenData(&rr, minMemi, maxMemi);
	refTabMap2(&s_cxrefTab, olPushAllReferencesInBetweenMapFun, &rr);
	olProcessSelectedReferences(rstack, genOnLineReferences);
	//&olcxPrintSelectionMenu(s_olcxCurrentUser->browserStack.top->menuSym);
}

static int tpCheckUnicityOfSymbol(char *fieldOrMethod) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	if (ss == NULL) {
		sprintf(tmpBuff,"No symbol selected, please position the cursor on a definition of a %s (on its name) before invoking this refactoring.", fieldOrMethod);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	if (ss->next != NULL) {
		error(ERR_ST,"Ambiguous symbol. You are probably trying to refactor a constructor or a native method. C-xrefactory does not know to handle those cases, sorry.");
		return(0);
	}
	return(1);
}

static int tpCheckThatCurrentReferenceIsDefinition(char *fieldOrMethod) {
	S_olcxReferences  	*rstack;
	int					found;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	if (rstack->act==NULL 
		|| ! IS_DEFINITION_OR_DECL_USAGE(rstack->act->usg.base)) {
		sprintf(tmpBuff,"This refactoring requires cursor (point) to be positioned at the definition of a %s (on its name). Please move to the definition first.", fieldOrMethod);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

static int tpCheckItIsAFieldOrMethod(int require, char *fieldOrMethod) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	if (ss->s.b.symType!=TypeDefault 
		|| (ss->s.b.storage!=StorageField && require==REQ_FIELD)
		|| (ss->s.b.storage!=StorageMethod && require==REQ_METHOD)) {
		sprintf(tmpBuff, "This does not look like a %s, please position the cursor on a definition of a %s (on its name) before invoking this refactoring.", 
				fieldOrMethod, fieldOrMethod);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

static int tpCheckStaticity(int require,char *fieldOrMethod) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	char				ttt[MAX_CX_SYMBOL_SIZE];
	char				*s;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	if (require==REQ_STATIC && (ss->s.b.accessFlags & ACC_STATIC) == 0) {
		linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
		sprintf(tmpBuff,"%c%s %s is not static, this refactoring requires a static %s.", 
				toupper(*fieldOrMethod), fieldOrMethod+1, ttt, fieldOrMethod);
		error(ERR_ST, tmpBuff);
		return(0);		
	} else if (require==REQ_NONSTATIC && (ss->s.b.accessFlags & ACC_STATIC)) {
		linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
		sprintf(tmpBuff,"%c%s %s is declared static, this refactoring requires a non static %s.", 
				toupper(*fieldOrMethod), fieldOrMethod+1, ttt, fieldOrMethod);
		error(ERR_ST, tmpBuff);
		return(0);		
	}
	return(1);
}

static S_symbol *javaGetClassSymbolFromClassDotName(char *fqName) {
	S_symbol 	*res;
	char		*dd, *sn;
	char		ttt[MAX_CX_SYMBOL_SIZE];
	strcpy(ttt, fqName);
	dd = sn = ttt;
	while ((dd=strchr(dd,'.'))!=NULL) {
		*dd = '/';
		sn = dd+1;
	}
	res = javaFQTypeSymbolDefinition(sn, ttt);
	return(res);
}

S_symbol *getMoveTargetClass() {
	if (s_opt.moveTargetClass == NULL) {
		error(ERR_INTERNAL,"pull up/push down pre-check without setting target class");
		return(NULL);
	}
	return(javaGetClassSymbolFromClassDotName(s_opt.moveTargetClass));
}

static int tpCheckItIsAPackage(int req, char *classOrPack) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	if ((req==REQ_PACKAGE && ss->s.b.symType!=TypePackage) ||
		(req==REQ_CLASS && ss->s.b.symType!=TypeStruct)) { 
		sprintf(tmpBuff, "This does not look like a %s name, please position the cursor on a %s name before invoking this refactoring.", classOrPack, classOrPack);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

static int tpCheckPrintClassMovingType() {
	S_olcxReferences  		*rstack;
	S_olSymbolsMenu			*ss;
	S_tpCheckMoveClassData	dd;
	char					spack[MAX_FILE_NAME_SIZE];
	char					tpack[MAX_FILE_NAME_SIZE];
	int						transPackageMove;

	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	if (ss->s.b.accessFlags & ACC_STATIC) {
		fprintf(ccOut,"*nested");
	} else {
		fprintf(ccOut,"*outer");
	}
	tpCheckFillMoveClassData(&dd, spack, tpack);
	if (dd.transPackageMove) {
		fprintf(ccOut, ";transpackage");
	}
	return(1);
}

static int tpCheckPrintSelectedSymbol() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	fprintf(ccOut,"*");
	printSymbolLinkName(ccOut, ss);
	return(1);
}

int javaGetSuperClassNumFromClassNum(int cn) {
	int 			res;
	S_chReference	*cl;
	for(cl = s_fileTab.tab[cn]->sups; cl!=NULL; cl=cl->next) {
		res = cl->clas;
		if (! s_fileTab.tab[res]->b.isInterface) return(res);
	}
	return(s_noneFileIndex);
}

int javaIsSuperClass(int superclas, int clas) {
	int ss;
	for (ss=javaGetSuperClassNumFromClassNum(clas); 
		 ss!=s_noneFileIndex && ss!=superclas; 
		 ss=javaGetSuperClassNumFromClassNum(ss)) ;
	if (ss == superclas) return(1);
	return(0);
}

static int tpCheckPrintPushingDownMethodType() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss,*mm;
	S_symbol 			*target;
	int					targetcn, srccn;
	fprintf(ccOut, "*** push_down_method trivial precheck passed.");
	return(1);
#if ZERO
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	srccn = ss->s.vApplClass;
	target = getMoveTargetClass();
	assert(target!=NULL && target->u.s);
	targetcn = target->u.s->classFile;
	// this is basically to print 'deletingAllowed' or not to print it
	for (mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
		if (itIsSameCxSymbol(&ss->s, &mm->s)) {
			// is there a reference to it from another subclass than target?
			if (mm->s.vFunClass == ss->s.vFunClass) {
				// rafn also avoids the original definition
				if (mm->refn!=0 && ! isSmallerOrEqClass(mm->s.vApplClass, targetcn)) {
					fprintf(ccOut,"* multiCopyPushDown");
					return(1);
				}
			}
			// is there an application to original class or some of super types?
			// you should not consider any recursive call from within the method, 
			// am I right?
			// avoid testing of super.sym() of source file
			// But HOW TO DO ALL THOSE TESTS?, maybe if done as post checks?
			if (mm->refn!=0 && isSmallerOrEqClass(srccn, mm->s.vApplClass)) {
				fprintf(ccOut,"* singleCopyPushDown");
				return(1);
			}
		}
	}
	fprintf(ccOut, "* singleDeletingPushDown");
	return(1);
#endif
}


static void olTrivialRefactoringPreCheck(int refcode) {
	S_olcxReferences *tpchsymbol;
	olCreateSelectionMenu(s_olcxCurrentUser->browserStack.top->command);
	tpchsymbol = s_olcxCurrentUser->browserStack.top;
	switch (refcode) {
	case TPC_MOVE_FIELD:
		tpCheckUnicityOfSymbol("field") &&
			tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
			tpCheckThatCurrentReferenceIsDefinition("field") &&
			fprintf(ccOut,"*** move_static_field trivial precheck passed.");
		break;
	case TPC_MOVE_STATIC_FIELD:
		tpCheckUnicityOfSymbol("field") &&
			tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
			tpCheckThatCurrentReferenceIsDefinition("field") &&
			tpCheckStaticity(REQ_STATIC,"field") &&
			fprintf(ccOut,"*** move_static_field trivial precheck passed.");
		break;
	case TPC_MOVE_STATIC_METHOD:
		tpCheckUnicityOfSymbol("method") &&
			tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
			tpCheckThatCurrentReferenceIsDefinition("method") &&
			tpCheckStaticity(REQ_STATIC,"method") &&
			fprintf(ccOut,"*** move_static_method trivial precheck passed.");
		break;
	case TPC_TURN_STATIC_METHOD_TO_DYN:
		tpCheckUnicityOfSymbol("method") &&
			tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
			tpCheckThatCurrentReferenceIsDefinition("method") &&
			tpCheckStaticity(REQ_STATIC,"method") &&
			fprintf(ccOut,"*** turn_static_method_to_dyn trivial precheck passed.");
		break;
	case TPC_TURN_DYN_METHOD_TO_STATIC:
		tpCheckUnicityOfSymbol("method") &&
			tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
			tpCheckThatCurrentReferenceIsDefinition("method") &&
			tpCheckStaticity(REQ_NONSTATIC,"method") &&
			tpCheckOuterScopeUsagesForDynToSt() &&
			tpCheckSuperMethodReferencesForDynToSt() &&
			fprintf(ccOut,"*** turn_static_method_to_dyn trivial precheck passed.");
		break;
	case TPC_PULL_UP_METHOD:
		tpCheckUnicityOfSymbol("method")
		&& tpCheckItIsAFieldOrMethod(REQ_METHOD, "method")
		&& tpCheckThatCurrentReferenceIsDefinition("method")
		&& tpCheckStaticity(REQ_NONSTATIC,"method") 
		&& tpCheckTargetToBeDirectSubOrSupClass(REQ_SUPERCLASS,"superclass")
		&& tpCheckSuperMethodReferencesForPullUp()
		&& tpCheckMethodReferencesWithApplOnSuperClassForPullUp()
		&& fprintf(ccOut,"*** pull_up_method trivial precheck passed.");
		break;
	case TPC_PUSH_DOWN_METHOD:
		tpCheckUnicityOfSymbol("method") &&
			tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
			tpCheckThatCurrentReferenceIsDefinition("method") &&
			tpCheckStaticity(REQ_NONSTATIC,"method") &&
			tpCheckTargetToBeDirectSubOrSupClass(REQ_SUBCLASS, "subclass") &&
			tpCheckPrintPushingDownMethodType();
		break;
	case TPC_PUSH_DOWN_METHOD_POST_CHECK:
		tpCheckSuperMethodReferencesAfterPushDown() &&
			fprintf(ccOut,"*** push_down_method trivial postcheck passed.");
		break;
	case TPC_PULL_UP_FIELD:
		tpCheckUnicityOfSymbol("field")
		&& tpCheckItIsAFieldOrMethod(REQ_FIELD, "field")
		&& tpCheckThatCurrentReferenceIsDefinition("field")
		&& tpCheckStaticity(REQ_NONSTATIC,"field") 
		&& tpCheckTargetToBeDirectSubOrSupClass(REQ_SUPERCLASS,"superclass")
		&& tpPullUpFieldLastPreconditions()
		&& fprintf(ccOut,"*** pull_up_field trivial precheck passed.");
		break;
	case TPC_PUSH_DOWN_FIELD:
		tpCheckUnicityOfSymbol("field") &&
			tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
			tpCheckThatCurrentReferenceIsDefinition("field") &&
			tpCheckStaticity(REQ_NONSTATIC,"field") &&
			tpCheckTargetToBeDirectSubOrSupClass(REQ_SUBCLASS, "subclass") &&
			tpPushDownFieldLastPreconditions() &&
			fprintf(ccOut,"*** push_down_field trivial precheck passed.");
		break;
	case TPC_RENAME_PACKAGE:
		tpCheckUnicityOfSymbol("package") &&
			tpCheckItIsAPackage(REQ_PACKAGE, "package") &&
			tpCheckPrintSelectedSymbol();
		break;
	case TPC_RENAME_CLASS:
		tpCheckUnicityOfSymbol("class") &&
			tpCheckItIsAPackage(REQ_CLASS, "class") &&
			tpCheckPrintSelectedSymbol();
		break;
	case TPC_MOVE_CLASS:
		tpCheckUnicityOfSymbol("class") &&
			tpCheckItIsAPackage(REQ_CLASS, "class") &&
			tpCheckSourceIsNotInnerClass() &&
			tpCheckMoveClassAccessibilities() &&
			tpCheckPrintClassMovingType();
		break;
	case TPC_GET_LAST_IMPORT_LINE:
		if (s_opt.xref2) {
			sprintf(tmpBuff, "%d", s_cps.lastImportLine);
			ppcGenRecord(PPC_SET_INFO, tmpBuff, "\n");
		} else {
			fprintf(ccOut,"*%d", s_cps.lastImportLine);
		}
		break;
	default:
		error(ERR_INTERNAL,"trivial precheck called with no valid check code");
	}
	// remove the checked symbol from stack
	olStackDeleteSymbol(tpchsymbol);
}

static void mainAnswerReferencePushingAction(int command) {
	S_olcxReferences *rstack;
	assert(creatingOlcxRefs());		
//&olcxPrintSelectionMenu(s_olcxCurrentUser->browserStack.top->hkSelectedSym);
//&olcxPrintSelectionMenu(s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	olCreateSelectionMenu(command);
//&olcxPrintSelectionMenu(s_olcxCurrentUser->browserStack.top->hkSelectedSym);
//&olcxDumpSelectionMenu(s_olcxCurrentUser->browserStack.top->menuSym);
	if (s_opt.manualResolve == RESOLVE_DIALOG_ALLWAYS 
		|| safetyCheck2ShouldWarn()
		|| (olcxShowSelectionMenu() 
			&& s_opt.manualResolve != RESOLVE_DIALOG_NEVER)) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "", "\n");
		} else {
			olcxPrintSelectionMenu(s_olcxCurrentUser->browserStack.top->menuSym);
		}
	} else {
		assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
		rstack = s_olcxCurrentUser->browserStack.top;
		//&olProcessSelectedReferences(rstack, genOnLineReferences);
		olcxPrintPushingAction(s_opt.cxrefs, DEFAULT_VALUE);
	}
}

static void mapAddLocalUnusedSymbolsToHkSelection(S_symbolRefItem *ss) {
	S_reference *rr, *dr;
	int used, thisFileSym;
	dr = NULL;
	used = 0;
	thisFileSym = 0;
	if (ss->b.category != CatLocal) return;
	for(rr = ss->refs; rr!=NULL; rr=rr->next) {
		if (IS_DEFINITION_OR_DECL_USAGE(rr->usg.base)) {
			if (rr->p.file == s_input_file_number) {
				if (IS_DEFINITION_USAGE(rr->usg.base)) {
					dr = rr;
				}
				if (dr == NULL) dr = rr;
			}
		} else {
			used = 1;
			break;
		}
	}
	if (used == 0 && dr!=NULL) {
		olAddBrowsedSymbol(ss,&s_olcxCurrentUser->browserStack.top->hkSelectedSym,
						   1,1,0,UsageDefined,0, &dr->p, dr->usg.base);
		
	}
}

void pushLocalUnusedSymbolsAction() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	int					fnum;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss == NULL);
	refTabMap(&s_cxrefTab, mapAddLocalUnusedSymbolsToHkSelection);
	olCreateSelectionMenu(s_opt.cxrefs);
}

static void answerPushLocalUnusedSymbolsAction() {
	pushLocalUnusedSymbolsAction();
	assert(s_opt.xref2);
	ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "", "\n");
}

static S_reference *getUsageRef(S_reference *rrr) {
	S_reference *res, *rr;
	res = NULL;
	for(rr=rrr; rr!=NULL; rr=rr->next) {
		if (OL_VIEWABLE_REFS(rr) && (! IS_DEFINITION_OR_DECL_USAGE(rr->usg.base))) {
			res = rr;
			break;
		}
	}
	return(res);
}

static void answerPushGlobalUnusedSymbolsAction() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	int					fnum;

	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss == NULL);
	scanReferenceFiles(s_opt.cxrefFileName, s_cxDeadCodeDetectionScanFunTab);
	olCreateSelectionMenu(s_opt.cxrefs);
	assert(s_opt.xref2);
	ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "", "\n");
}

static void getCallerPositionFromCommandLineOption(S_position *opos) {
	int f,l,c;
	assert(opos!=NULL);
	f = s_olOriginalFileNumber;
#ifdef ZERO
		assert(s_opt.lineFileName!=NULL);
		// this was here before putting out line_fileName
		assert(s_opt.lineFileName!=NULL);
		addFileTabItem(s_opt.lineFileName, &f);
#endif
	getLineColCursorPositionFromCommandLineOption( &l, &c);
	FILL_position(opos, f,l,c);
}

static void answerClassName(char *name) {
	char ttt[MAX_CX_SYMBOL_SIZE];
	if (*name!=0) {
		linkNamePrettyPrint(ttt, name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
		if (s_opt.xref2) {
			ppcGenRecord(PPC_SET_INFO, ttt, "\n");
		} else {
			fprintf(ccOut,"*%s", ttt);
		}
	} else {
		error(ERR_ST, "Not inside a class. Can't get curent class.");
	}
}

static void pushSymbolByName(char *name) {
	S_olcxReferences *rstack;
	if (s_cache.cpi>0) {
		int spass;
		spass = s_currCppPass; s_currCppPass=1;
		recoverCachePointZero();
		s_currCppPass = spass;
	}
	rstack = s_olcxCurrentUser->browserStack.top;
	rstack->hkSelectedSym = olCreateSpecialMenuItem(
		name, s_noneFileIndex, StorageDefault);
	getCallerPositionFromCommandLineOption(&rstack->cpos);
}

void mainAnswerEditAction() {
	S_olcxReferences  	*rstack, *nextrr;
	int 				category,f,l,c,lc;
	S_position 			opos;
	char				*ifname, *jdkcp;
	char 				dffname[MAX_FILE_NAME_SIZE];
	char 				dffsect[MAX_FILE_NAME_SIZE];
	assert(ccOut);
	switch (s_opt.cxrefs) {
	case OLO_CHECK_VERSION:
		assert(s_opt.checkVersion!=NULL);
		if (strcmp(s_opt.checkVersion, C_XREF_VERSION_NUMBER)!=0) {
			ppcGenRecord(PPC_VERSION_MISMATCH, C_XREF_VERSION_NUMBER, "\n");
		}
		break;
	case OLO_COMPLETION: case OLO_SEARCH:
		printCompletions(&s_completions);
		break;
	case OLO_EXTRACT:
		if (! s_cps.extractProcessedFlag) {
			fprintf(ccOut,"*** No function/method enclosing selected block found **");
		}
		break;
	case OLO_TAG_SEARCH:
		getCallerPositionFromCommandLineOption(&opos);
		//&olCompletionListInit(&opos);
		if (! s_opt.xref2) fprintf(ccOut,";");
		assert(s_olcxCurrentUser);
		olcxPushEmptyStackItem(&s_olcxCurrentUser->retrieverStack);
		s_olcxCurrentUser->retrieverStack.top->cpos = opos;
		if (containsWildCharacter(s_opt.olcxSearchString)) {
			s_wildCharSearch = 1;
		} else {
			s_wildCharSearch = 0;
		}
		if (s_opt.tagSearchSpecif==TSS_FULL_SEARCH) scanJarFilesForTagSearch();
		scanReferenceFiles(s_opt.cxrefFileName, s_cxSymbolSearchScanFunTab);
		printTagSearchResults();
		break;
	case OLO_TAG_SEARCH_BACK:
		olcxSetCurrentUser(s_opt.user);
		if (s_olcxCurrentUser->retrieverStack.top!=NULL &&
			s_olcxCurrentUser->retrieverStack.top->previous!=NULL) {
			s_olcxCurrentUser->retrieverStack.top = s_olcxCurrentUser->retrieverStack.top->previous;
			ppcGenGotoPositionRecord(&s_olcxCurrentUser->retrieverStack.top->cpos);
			printTagSearchResults();
		}
		break;
	case OLO_TAG_SEARCH_FORWARD:
		olcxSetCurrentUser(s_opt.user);
		nextrr = getNextTopStackItem(&s_olcxCurrentUser->retrieverStack);
		if (nextrr != NULL) {
			s_olcxCurrentUser->retrieverStack.top = nextrr;
			ppcGenGotoPositionRecord(&s_olcxCurrentUser->retrieverStack.top->cpos);
			printTagSearchResults();
		}
		break;
	case OLO_ACTIVE_PROJECT:
		if (s_opt.project != NULL) {
			if (s_opt.xref2) {
				ppcGenRecord(PPC_SET_INFO, s_opt.project, "\n");
			} else {
				fprintf(ccOut,"*%s", s_opt.project);
			}
		} else {
			if (s_olOriginalComFileNumber == s_noneFileIndex) {
				if (s_opt.xref2) {
					ppcGenRecord(PPC_ERROR, "No source file to identify project", "\n");
				} else {
					fprintf(ccOut,"!** No source file to identify project");
				}
			} else {
				ifname = s_fileTab.tab[s_olOriginalComFileNumber]->name;
				searchDefaultOptionsFile(ifname, dffname, dffsect);
				if (dffname[0]==0 || dffsect[0]==0) {
					if (s_opt.noErrors) {
						if (! s_opt.xref2) fprintf(ccOut,"^", ifname);
					} else {
						if (s_opt.xref2) {
							ppcGenRecord(PPC_NO_PROJECT, ifname, "\n");
						} else {
							fprintf(ccOut,"!** No project name matches %s", ifname);
						}
					}
				} else {
					if (s_opt.xref2) {
						ppcGenRecord(PPC_SET_INFO, dffsect, "\n");
					} else {
						fprintf(ccOut,"*%s", dffsect);
					}
				}
			}
		}
		break;
	case OLO_JAVA_HOME:
		jdkcp = getJavaHome();
		if (s_opt.xref2) {
			if (jdkcp==NULL) {
				if (! s_opt.noErrors) {
					ppcGenRecord(PPC_ERROR, "Can't infer Java home", "\n");
				}
			} else {
				ppcGenRecord(PPC_SET_INFO, jdkcp, "\n");
			}
		} else {
			if (jdkcp==NULL) {
				if (s_opt.noErrors) {
					fprintf(ccOut,"^");
				} else {
					fprintf(ccOut,"!* Can't find Java runtime library rt.jar", ifname);
				}
			} else {
				fprintf(ccOut,"*%s", jdkcp);
			}
		}
		break;
	case OLO_GET_ENV_VALUE:
		olcxProcessGetRequest();
		break;
	case OLO_PLUS:	olcxReferencePlus();
		break;
	case OLO_MINUS:	olcxReferenceMinus();
		break;
	case OLO_GOTO_DEF:	olcxReferenceGotoDef();
		break;
	case OLO_GOTO_CALLER:	olcxReferenceGotoCaller();
		break;
	case OLO_GOTO_CURRENT:	olcxReferenceGotoCurrent();
		break;
	case OLO_GET_CURRENT_REFNUM:	olcxReferenceGetCurrentRefn();
		break;
	case OLO_LIST_TOP:	olcxListTopReferences(";");
		break;
	case OLO_SHOW_TOP:	olcxShowTopSymbol();
		break;
	case OLO_SHOW_TOP_APPL_CLASS:	olcxShowTopApplClass();
		break;
	case OLO_SHOW_TOP_TYPE:	olcxShowTopType();
		break;
	case OLO_SHOW_CLASS_TREE:	olcxShowClassTree();
		break;
	case OLO_TOP_SYMBOL_RES:	olcxTopSymbolResolution();
		break;
	case OLO_CSELECT:	olCompletionSelect();
		break;
	case OLO_COMPLETION_BACK:	olCompletionBack();
		break;
	case OLO_COMPLETION_FORWARD:	olCompletionForward();
		break;
	case OLO_GOTO:	olcxReferenceGotoRef(s_opt.olcxGotoVal);
		break;
	case OLO_CGOTO:	olcxReferenceGotoCompletion(s_opt.olcxGotoVal);
		break;
	case OLO_TAGGOTO:	olcxReferenceGotoTagSearchItem(s_opt.olcxGotoVal);
		break;
	case OLO_TAGSELECT:	olcxReferenceSelectTagSearchItem(s_opt.olcxGotoVal);
		break;
	case OLO_CBROWSE:	olcxReferenceBrowseCompletion(s_opt.olcxGotoVal);
		break;
	case OLO_REF_FILTER_SET:	olcxReferenceFilterSet(s_opt.filterValue);
		break;
	case OLO_REPUSH:	olcxReferenceRePush();
		break;
	case OLO_POP:	olcxReferencePop();
		break;
	case OLO_POP_ONLY:	olcxPopOnly();
		break;
	case OLO_MENU_INSPECT_DEF: olcxSymbolMenuInspectDef();
		break;
	case OLO_MENU_INSPECT_CLASS: olcxSymbolMenuInspectClass();
		break;
	case OLO_MENU_SELECT: olcxMenuToggleSelect();
		break;
	case OLO_MENU_SELECT_ONLY: olcxMenuSelectOnly();
		break;
	case OLO_MENU_SELECT_ALL: olcxMenuSelectAll(1);
		break;
	case OLO_MENU_SELECT_NONE: olcxMenuSelectAll(0);
		break;
	case OLO_MENU_FILTER_SET: 
		olcxMenuSelectPlusolcxMenuSelectFilterSet(s_opt.filterValue);
		break;
	case OLO_RESET_REF_SUFFIX: olcxResetSuffix();
		break;
	case OLO_SAFETY_CHECK_INIT: olcxSafetyCheckInit();
		break;
	case OLO_SAFETY_CHECK1: olcxSafetyCheck1();
		break;
	case OLO_MM_PRE_CHECK: case OLO_PP_PRE_CHECK: 
		olcxMMPreCheck();	// the value of s_opt.cxrefs is checked inside
		break;
	case OLO_INTERSECTION: olcxTopReferencesIntersection();
		break;
	case OLO_REMOVE_WIN: olcxTopReferencesRemoveWindow();
		break;
	case OLO_MENU_GO:
		assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
		rstack = s_olcxCurrentUser->browserStack.top;
		//&olProcessSelectedReferences(rstack, genOnLineReferences);
		olcxPrintPushingAction(s_olcxCurrentUser->browserStack.top->command, 
							   PUSH_AFTER_MENU);
		break;
	case OLO_CT_INSPECT_DEF: olcxClassTreeInspectDef();
		break;
	case OLO_CLASS_TREE:  olcxCreateClassTree();
		break;
	case OLO_USELESS_LONG_NAME: case OLO_USELESS_LONG_NAME_IN_CLASS:  
		olcxListSpecial(LINK_NAME_IMPORTED_QUALIFIED_ITEM);
		break;
	case OLO_MAYBE_THIS:  olcxListSpecial(LINK_NAME_MAYBE_THIS_ITEM);
		break;
	case OLO_NOT_FQT_REFS:  case OLO_NOT_FQT_REFS_IN_CLASS:  
		olcxListSpecial(LINK_NAME_NOT_FQT_ITEM);
		break;
	case OLO_SET_MOVE_TARGET:						// xref1 target setting
		assert(!s_opt.xref2);
		if (*s_cps.setTargetAnswerClass!=0) {
			fprintf(ccOut,"*");
			//&printSymbolLinkNameString(ccOut, s_cps.setTargetAnswerClass);
			// very risky, will need a lot of adjustements in xref.el
			fprintf(ccOut, s_cps.setTargetAnswerClass);
		} else {
			error(ERR_ST, "Not a valid target position. The cursor has to be on a place where a new field/method can be inserted.");
		}
		break;
	case OLO_SET_MOVE_CLASS_TARGET:	case OLO_SET_MOVE_METHOD_TARGET:	// xref2 target
		assert(s_opt.xref2);
		if (!s_cps.moveTargetApproved) {
			ppcGenRecord(PPC_ERROR, "Invalid target place","\n");
		}
		break;
	case OLO_GET_CURRENT_CLASS:
		answerClassName(s_cps.currentClassAnswer);
		break;
	case OLO_GET_CURRENT_SUPER_CLASS:
		answerClassName(s_cps.setTargetAnswerClass);
		break;
	case OLO_GET_METHOD_COORD:
		if (s_cps.methodCoordEndLine!=0) {
			fprintf(ccOut,"*%d", s_cps.methodCoordEndLine);
		} else {
			error(ERR_ST, "No method found.");
		}
		break;
	case OLO_GET_CLASS_COORD:
		if (s_cps.classCoordEndLine!=0) {
			fprintf(ccOut,"*%d", s_cps.classCoordEndLine);
		} else {
			error(ERR_ST, "No class found.");
		}
		break;
	case OLO_GET_SYMBOL_TYPE:
		if (s_olstringServed) {
			fprintf(ccOut,"*%s", s_olSymbolType);
		} else if (s_opt.noErrors) {
			fprintf(ccOut,"*");
		} else {
			error(ERR_ST, "No symbol found.");
		}
		olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
		break;
	case OLO_GOTO_PARAM_NAME:
		// I hope this is not used anymore, put there assert(0);
		if (s_olstringServed && s_paramPosition.file != s_noneFileIndex) {
			generateOnlineCxref(&s_paramPosition, COLCX_GOTO_REFERENCE, 
								UsageDefined, s_olcxCurrentUser->browserStack.top->refsuffix, "");
			olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
		} else {
			sprintf(tmpBuff, "Parameter %d not found.", s_opt.olcxGotoVal);
			error(ERR_ST, tmpBuff);
		}
		break;
	case OLO_GET_PRIMARY_START:
		if (s_olstringServed && s_primaryStartPosition.file != s_noneFileIndex) {
			generateOnlineCxref(&s_primaryStartPosition, COLCX_GOTO_REFERENCE, 
								UsageDefined, s_olcxCurrentUser->browserStack.top->refsuffix, "");
			olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
		} else {
			error(ERR_ST, "Begin of primary expression not found.");
		}
		break;
	case OLO_PUSH_ALL_IN_METHOD:
//&fprintf(ccOut,":\n\n getting all references from %d to %d\n", s_cps.cxMemiAtMethodBeginning, s_cps.cxMemiAtMethodEnd);
		olPushAllReferencesInBetween(s_cps.cxMemiAtMethodBeginning, s_cps.cxMemiAtMethodEnd);
		olcxPrintPushingAction(s_opt.cxrefs, 0);
		break;
	case OLO_TRIVIAL_PRECHECK:
		olTrivialRefactoringPreCheck(s_opt.trivialPreCheckCode);
		break;
	case OLO_ENCAPSULATE_SAFETY_CHECK:
		olEncapsulationSafetyCheck();
		break;
	case OLO_GET_AVAILABLE_REFACTORINGS:
		olGetAvailableRefactorings();
		olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
		break;
	case OLO_PUSH_NAME:
		pushSymbolByName(s_opt.pushName);
		mainAnswerReferencePushingAction(s_opt.cxrefs);
		break;
	case OLO_GLOBAL_UNUSED:
		answerPushGlobalUnusedSymbolsAction();
		break;
	case OLO_LOCAL_UNUSED:
		answerPushLocalUnusedSymbolsAction();
		break;
	case OLO_PUSH_SPECIAL_NAME:
		assert(0);  // only refactory
		break;
	case OLO_ARG_MANIP:
		rstack = s_olcxCurrentUser->browserStack.top;
		assert(rstack!=NULL);
		if (rstack->hkSelectedSym == NULL ||
			(LANGUAGE(LAN_JAVA) && 
			 rstack->hkSelectedSym->s.b.storage!=StorageMethod &&
			 rstack->hkSelectedSym->s.b.storage!=StorageConstructor)) {
			sprintf(tmpBuff,"Cursor (point) has to be positioned on a method or contructor name before invocation of this refactoring, not on the parameter itself. Please move the cursor onto the method (contructor) name and reinvoke the refactoring.");
			error(ERR_ST, tmpBuff);
		} else {
			mainAnswerReferencePushingAction(s_opt.cxrefs);
		}
		break;
	default:
		mainAnswerReferencePushingAction(s_opt.cxrefs);
	} // switch

	fflush(ccOut);
	s_input_file_name = NULL;
//&RLM_FREE_COUNT(olcxMemory);
}

int byPassAcceptableSymbol(S_symbolRefItem *p) {
	int nlen,len;
	char *nn, *nnn;
	GET_NUDE_NAME(p->name, nn, len);
	GET_NUDE_NAME(s_opt.browsedSymName, nnn, nlen);
	if (len != nlen) return(0);
	if (strncmp(nn, nnn, len)) return(0);
	return(1);
}

// DOES NOT WORK!!! because it is not loading references from current file
// because cxfile does not know I am in s_opt.cxrefs==OLO_PUSH_NAME
static void oloPushByNameIfNothingPushed() {
	if (s_olcxCurrentUser!=NULL 
		&& s_olcxCurrentUser->browserStack.top!=NULL
		&& s_olcxCurrentUser->browserStack.top->hkSelectedSym==NULL
		&& s_olstring[0]!=0) {
		// O.K. here I am nothing found
		pushSymbolByName(s_olstring);
		// will result into recursive call, but do not worry, hkSelectedSym
		// is not NULL here, so no loop can occur
		mainAnswerReferencePushingAction(OLO_PUSH_NAME);
	}
	if (! olcxCheckSymbolExists()) {
		olcxNoSymbolFoundErrorMessage();
		olStackDeleteSymbol( s_olcxCurrentUser->browserStack.top);
	}
}

static int itIsSymbolToPushOlRefencesLess(S_olSymbolsMenu *ss, S_symbolRefItem *p) {
	return(olSymbolRefItemLess(&ss->s, p));
}

int itIsSymbolToPushOlRefences(S_symbolRefItem *p,
							   S_olcxReferences *rstack, 
							   S_olSymbolsMenu **rss, 
							   int checkSelFlag) {
	S_olSymbolsMenu		*ss;
	for(ss=rstack->menuSym; ss!=NULL; ss=ss->next) {
		if ((ss->selected || checkSelFlag==DO_NOT_CHECK_IF_SELECTED)
			&& ss->s.vApplClass == p->vApplClass
			&& ss->s.vFunClass == p->vFunClass
			&& itIsSameCxSymbol(p, &ss->s)) {
			*rss = ss;
			if (IS_BEST_FIT_MATCH(ss)) {
				return(2);
			} else {
				return(1);
			}
		}
	}
	*rss = NULL;
	return(0);
}


void putOnLineLoadedReferences(S_symbolRefItem *p) {
	int 				ols, num, nn;
	S_olSymbolsMenu 	*cms;
	S_reference 		*r, *rr, **place;
	S_olSymbolsMenu		*ss;
	ols = itIsSymbolToPushOlRefences(p,s_olcxCurrentUser->browserStack.top, 
									 &cms, DO_NOT_CHECK_IF_SELECTED);
	if (ols) {
		assert(cms);
//&LIST_LEN(nn, S_reference, p->refs);sprintf(tmpBuff,"!putting %d references of %s\n", nn, s_fileTab.tab[p->vApplClass]->name);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
		for(rr=p->refs; rr!=NULL; rr=rr->next) {
			olcxAddReferenceToOlSymbolsMenu(cms, rr, (ols == 2));
		}
//&LIST_LEN(nn, S_reference, cms->s.refs);sprintf(tmpBuff,"added %d refs for %s", nn, s_fileTab.tab[cms->s.vApplClass]->name);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
	}
}

void genOnLineReferences(S_olcxReferences *rstack, S_olSymbolsMenu *cms) {
	int ols, num;
	if (cms->selected) {
		assert(cms);
		olcxAddReferences(cms->s.refs, &rstack->r, ANY_FILE, 
						  IS_BEST_FIT_MATCH(cms));
	}
}

static int classCmp(int cl1, int cl2) {
	int res;
//&fprintf(dumpOut,"classCMP %s <-> %s\n", s_fileTab.tab[cl1]->name, s_fileTab.tab[cl2]->name);
	res = isSmallerOrEqClassR(cl2, cl1, 1);
	if (res == 0) {
		res = - isSmallerOrEqClassR(cl1, cl2, 1);
	}
//&fprintf(dumpOut,"result is %d\n",res);
	return(res);
}

static unsigned olcxOoBits(S_olSymbolsMenu *ols, S_symbolRefItem *p) {
	unsigned ooBits;
	int olusage,vFunCl,olvFunCl,vApplCl,olvApplCl;
	ooBits = 0;
	assert(olcxItIsSameCxSymbol(&ols->s,p));
	olvFunCl = ols->s.vFunClass;
	olvApplCl = ols->s.vApplClass;
	olusage = ols->olUsage;
	vFunCl = p->vFunClass;
	vApplCl = p->vApplClass;
	if (ols->s.b.symType!=TypeCppCollate) {
		if (ols->s.b.symType != p->b.symType) goto fini;
		if (ols->s.b.storage != p->b.storage) goto fini;
		if (ols->s.b.category != p->b.category)  goto fini;
	}
	if (strcmp(ols->s.name,p->name)==0) {
		ooBits |= OOC_PROFILE_EQUAL;
	}
	if (LANGUAGE(LAN_C) || LANGUAGE(LAN_YACC)
		|| JAVA_STATICALLY_LINKED(ols->s.b.storage, ols->s.b.accessFlags)) {
		if (vFunCl == olvFunCl) ooBits |= OOC_VIRT_SAME_APPL_FUN_CLASS;
	} else {
		// the following may be too strong, maybe only test FunCl ???
//&		} else if (vFunCl==olvFunCl) {
//&			ooBits |= OOC_VIRT_SAME_FUN_CLASS;
		if (vApplCl == olvFunCl && vFunCl==olvFunCl) {
			ooBits |= OOC_VIRT_SAME_APPL_FUN_CLASS;
		} else if (olcxVirtualyUsageAdequate(vApplCl, vFunCl, olusage, olvApplCl, olvFunCl)) {
			ooBits |= OOC_VIRT_APPLICABLE;
		} else if (vFunCl==olvFunCl) {
			ooBits |= OOC_VIRT_SAME_FUN_CLASS;
		} else if (isSmallerOrEqClass(olvApplCl, vApplCl)
			|| isSmallerOrEqClass(vApplCl, olvApplCl)) {
			ooBits |= OOC_VIRT_RELATED;
// following can be too slow !!!!!!!!! , if so, some marks must be put once
// moreover it is missplaced, all classes have common superclass Object.
//&		} else if (isRelatedClass(olvApplCl, vApplCl)) {
//&			ooBits |= OOC_VIRT_COMMON_SUP;
		}
	}
 fini:
	return(ooBits);
}

static unsigned ooBitsMax(unsigned oo1, unsigned oo2) {
	unsigned res;
	res = 0;
	if ((oo1&OOC_PROFILE_MASK) > (oo2&OOC_PROFILE_MASK)) {
		res |= (oo1&OOC_PROFILE_MASK);
	} else {
		res |= (oo2&OOC_PROFILE_MASK);
	}
	if ((oo1&OOC_VIRTUAL_MASK) > (oo2&OOC_VIRTUAL_MASK)) {
		res |= (oo1&OOC_VIRTUAL_MASK);
	} else {
		res |= (oo2&OOC_VIRTUAL_MASK);
	}
	return(res);
}

S_olSymbolsMenu *createSelectionMenu(S_symbolRefItem *p) {
	S_olSymbolsMenu		*ss, *res;
	S_olcxReferences	*rstack;
	S_position 			*defpos;
	unsigned			ooBits, oo, flag;
	int					select,visible,vlev,vlevel,defusage;
	res = NULL;
	rstack = s_olcxCurrentUser->browserStack.top;
	ooBits = 0; flag = 0; vlevel = 0; 
	defpos = &s_noPos; defusage = UsageNone;
//&fprintf(dumpOut,"ooBits for %s\n", s_fileTab.tab[p->vApplClass]->name);
	for(ss=rstack->hkSelectedSym; ss!=NULL; ss=ss->next) {
		if (olcxItIsSameCxSymbol(p, &ss->s)) {
			flag = 1;
			oo = olcxOoBits(ss, p);
			ooBits = ooBitsMax(oo, ooBits);
			if (defpos->file == s_noneFileIndex) {
				defpos = &ss->defpos;
				defusage = ss->defUsage;
//&fprintf(dumpOut,": propagating defpos (line %d) to menusym\n", defpos->line);
			}
			if (LANGUAGE(LAN_JAVA)) {
				vlev = classCmp(ss->s.vApplClass, p->vApplClass);
			} else { 
				vlev = 0;
			}
			if (vlevel==0 || ABS(vlevel)>ABS(vlev)) vlevel = vlev;
//&fprintf(dumpOut,"ooBits for %s <-> %s %o %o\n", s_fileTab.tab[ss->s.vApplClass]->name, p->name, oo, ooBits);
			// following line is a big hack to speed up class tree   !!!!!
			// and unused symbols displaying, hope it will work fine !!!!!
			//&if (s_opt.cxrefs == OLO_GLOBAL_UNUSED || s_opt.cxrefs==OLO_CLASS_TREE) break;
		}
	}
	if (flag) {
		select = visible = 0;  // for debug would be better 1 !
#if ZERO       // now moved into setSelectVisibleItems
		select = 0;
		visible = ooBitsGreaterOrEqual(ooBits, s_opt.ooChecksBits);
//&fprintf(dumpOut,"%s.%s bits==%o visible == %d\n", s_fileTab.tab[p->vApplClass]->name, p->name, ooBits, visible);
		if (visible) {
			select=ooBitsGreaterOrEqual(ooBits, DEFAULT_SELECTION_OO_BITS);
			if (p->b.symType==TypeCppCollate) select=0;
		}
#endif
		res = olAddBrowsedSymbol(p, &rstack->menuSym, select, visible, ooBits, USAGE_ANY, vlevel, defpos, defusage);
	}
	return(res);
}

void mapCreateSelectionMenu(S_symbolRefItem *p) {
	createSelectionMenu(p);
}


/* ********************************************************************** */

void olSetCallerPosition(S_position *pos) {
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	s_olcxCurrentUser->browserStack.top->cpos = *pos;
}

// if s==NULL, then the pos is taken as default position of this ref !!!

S_olCompletion * olCompletionListPrepend(	char *name, 
											char *fullText, char *vclass, int jindent,
											S_symbol *s,
											S_symbolRefItem *rr,
											S_reference *dfref,
											int cType,
											int vFunClass,
											S_olcxReferences *stack
	) {
	S_olCompletion 	**place,*cc;
	char 			*ss,*nn, *fullnn, *vclnn;
	int 			i, category, scope, storage, slen, nlen;
	S_symbolRefItem	sri;
	S_symbolRefItemBits srib;
	S_reference		dref;
	OLCX_ALLOC(cc, S_olCompletion);
	nlen = strlen(name);
	OLCX_ALLOCC(nn, nlen+1, char);
	strcpy(nn, name);
	fullnn = NULL;
	if (fullText!=NULL) {
		OLCX_ALLOCC(fullnn, strlen(fullText)+1, char);
		strcpy(fullnn, fullText);
	}
	vclnn = NULL;
	if (vclass!=NULL) {
		OLCX_ALLOCC(vclnn, strlen(vclass)+1, char);
		strcpy(vclnn, vclass);
	}
	if (rr!=NULL) {
		// probably a 'search in tag' file item
		slen = strlen(rr->name);
		OLCX_ALLOCC(ss, slen+1, char);
		strcpy(ss, rr->name);
		FILL_symbolRefItem(&sri, ss, cxFileHashNumber(ss),
						   rr->vApplClass, rr->vFunClass,
						   rr->b, NULL, NULL);
		FILL_olCompletion(cc, nn, fullnn, vclnn, jindent, 1, rr->b.category, cType, *dfref, sri, NULL);
	} else if (s==NULL) {
		dref = *dfref;
		dref.next = NULL;
		FILL_symbolRefItemBits(&srib, TypeUnknown, StorageNone,
							   ScopeAuto, ACC_DEFAULT, CatLocal, 0);
		FILL_symbolRefItem(&sri, "", cxFileHashNumber(""),
						   s_noneFileIndex, s_noneFileIndex,
						   srib, NULL, NULL);
		FILL_olCompletion(cc, nn, fullnn, vclnn, jindent, 1, CatLocal, cType, dref, sri, NULL);
	} else {
		getSymbolCxrefCategories(s, &category, &scope, &storage);
//&fprintf(dumpOut,":adding sym %s %d\n",s->linkName,category);fflush(dumpOut);
		slen = strlen(s->linkName);
		OLCX_ALLOCC(ss, slen+1, char);
		strcpy(ss, s->linkName);
		FILL_usageBits(&dref.usg, UsageDefined, 0, 0);
		FILL_reference(&dref, dref.usg, s->pos, NULL);
		FILL_symbolRefItemBits(&srib, s->b.symType, storage,
							   scope, s->b.accessFlags, category, 0);
		FILL_symbolRefItem(&sri, ss, cxFileHashNumber(ss),
						   vFunClass, vFunClass,
						   srib, NULL, NULL);
		FILL_olCompletion(cc, nn, fullnn, vclnn, jindent, 1, category, cType, dref, sri, NULL);
#ifdef ZERO // original
		if (category == CatLocal) {
			TODO;
		} else {
			FILL_symbolRefItemBits(&srib, s->b.symType, storage,
								   scope, s->b.accessFlags, category, 0);
			FILL_symbolRefItem(&sri, ss, cxFileHashNumber(ss),
							   vFunClass, vFunClass,
							   srib, NULL, NULL);
			FILL_olCompletion(cc, nn, fullnn, vclnn, jindent, 1, CatGlobal, cType, *dfref, sri, NULL);
		}
#endif
	}
	if (fullText!=NULL) {
		for(i=0; fullText[i]; i++) {
			if (fullText[i] == '\n') cc->lineCount++;
		}
	}
	cc->next = stack->cpls;
	stack->cpls = cc;
	return(cc);
}

void olCompletionListReverse() {
	LIST_REVERSE(S_olCompletion, s_olcxCurrentUser->completionsStack.top->cpls);
}

static int olTagSearchSortFunction(S_olCompletion *c1, S_olCompletion *c2) {
	return(strcmp(c1->name, c2->name) < 0);
}

static void tagSearchShortRemoveMultipleLines(S_olCompletion *list) {
	S_olCompletion *ll, *tmp;
	for(ll=list; ll!=NULL; ll=ll->next) {
	cont:
		if (ll->next!=NULL && strcmp(ll->name, ll->next->name)==0) {
			// O.K. remove redundand one
			tmp = ll->next;
			ll->next = ll->next->next;
			olcxFreeCompletion(tmp);
			goto cont;
		}
	}
}

void tagSearchCompactShortResults() {
	assert(s_olcxCurrentUser);
	LIST_MERGE_SORT(S_olCompletion, s_olcxCurrentUser->retrieverStack.top->cpls, olTagSearchSortFunction);
	if (s_opt.tagSearchSpecif==TSS_SEARCH_DEFS_ONLY_SHORT
		|| s_opt.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
		tagSearchShortRemoveMultipleLines(s_olcxCurrentUser->retrieverStack.top->cpls);
	}
}

void printTagSearchResults() {
	S_olCompletion *cc;
	int len1,len2,len3, len;
	char *ls;
	len1 = len2 = len3 = 0;
	tagSearchCompactShortResults();
	// the first loop is counting the lenght of fields
	assert(s_olcxCurrentUser->retrieverStack.top);
	for(cc=s_olcxCurrentUser->retrieverStack.top->cpls; cc!=NULL; cc=cc->next) {
		ls = crTagSearchLineStatic(cc->name, &cc->ref.p,
								   &len1, &len2, &len3); 
	}
	if (s_opt.olineLen >= 50000) {
		if (len1 > MAX_TAG_SEARCH_INDENT) len1 = MAX_TAG_SEARCH_INDENT;
	} else {
		if (len1 > (s_opt.olineLen*MAX_TAG_SEARCH_INDENT_RATIO)/100) {
			len1 = (s_opt.olineLen*MAX_TAG_SEARCH_INDENT_RATIO)/100;
		}
	}
	len = len1;
	// the second is writing
	if (s_opt.xref2) ppcGenRecordBegin(PPC_SYMBOL_LIST);
	assert(s_olcxCurrentUser->retrieverStack.top);
	for(cc=s_olcxCurrentUser->retrieverStack.top->cpls; cc!=NULL; cc=cc->next) {
		ls = crTagSearchLineStatic(cc->name, &cc->ref.p,
								   &len1, &len2, &len3); 
		if (s_opt.xref2) {
			ppcGenRecord(PPC_STRING_VALUE, ls, "\n");
		} else {
			fprintf(ccOut,"%s\n", ls);
		}
		len1 = len;
	}
	if (s_opt.xref2) ppcGenRecordEnd(PPC_SYMBOL_LIST);
}


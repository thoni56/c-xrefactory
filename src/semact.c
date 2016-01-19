/*
	$Revision: 1.13 $
	$Date: 2002/09/03 19:40:20 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "unigram.h"

int displayingErrorMessages() {
	// no error messages for file preloaded for symbols
	if (LANGUAGE(LAN_JAVA) && s_jsl!=NULL) return(0);
	if (s_opt.debug || s_opt.err) return(1);
	return(0);
}

int styyerror(char *s) {
	if (strcmp(s,"syntax error")!=0) {
		sprintf(tmpBuff,"YACC error: %s",s);
		error(ERR_INTERNAL,tmpBuff);
	}
	if (displayingErrorMessages()) {
		sprintf(tmpBuff,"on: %s",yytext);
		error(ERR_ST, tmpBuff);
	}
	return(0);
}

int styyErrorRecovery() {
	if (s_opt.debug && displayingErrorMessages()) {
		error(ERR_ST, " recovery");
	}
	return(0);
}

void setToNull(void *p) {
	void **pp;
	pp = (void **)p;
	*pp = NULL;
}

void deleteSymDef(void *p) {
	S_symbol 		*pp;
	S_javaStat		*scp;
	pp = (S_symbol *) p;
	DPRINTF3("deleting %s %s\n", pp->name, pp->linkName);
	if (symTabDelete(s_javaStat->locals,pp)) return;
	if (symTabDelete(s_symTab,pp)==0) {
		assert(s_opt.taskRegime);
		if (s_opt.taskRegime != RegimeEditServer) {
			error(ERR_INTERNAL,"symbol on deletion not found");
		}
	}
}

void unpackPointers(S_symbol *pp) {
	unsigned i;
	for (i=0; i<pp->b.npointers; i++) {
		appendComposedType(&pp->u.type, TypePointer);
	}
	pp->b.npointers=0;
}

void addSymbol(S_symbol *pp, S_symTab *tab) {
	/*	a bug can produce, if you add a symbol into old table, and the same
		symbol exists in a newer one. Then it will be deleted from the newer
		one. All this story is about storing information in trail. It should
		containt, both table and pointer !!!!
	*/
	DPRINTF4("adding symbol %s: %s %s\n",pp->name, typesName[pp->b.symType], storagesName[pp->b.storage]);
	assert(pp->b.npointers==0);
	AddSymbolNoTrail(pp,tab);
	addToTrail(deleteSymDef, pp  /* AND ALSO!!! , tab */ );
//if (WORK_NEST_LEVEL0()) {static int c=0;fprintf(dumpOut,"addsym0#%d\n",c++);}
}

void recFindPush(S_symbol *str, S_recFindStr *rfs) {
	S_symStructSpecific		*ss;
	assert(str && (str->b.symType==TypeStruct || str->b.symType==TypeUnion));
	if (rfs->recsClassCounter==0) {
		// this is hack to avoid problem when overloading to zero
		rfs->recsClassCounter++;
	}
	ss = str->u.s;
	rfs->nextRecord = ss->records;
	rfs->currClass = str;
//&	if (ss->super != NULL) { // this optimization makes completion info wrong
		rfs->st[rfs->sti] = ss->super;
		InternalCheck(rfs->sti < MAX_INHERITANCE_DEEP);
		rfs->sti ++;
//&	}
}

S_recFindStr * iniFind(S_symbol *s, S_recFindStr *rfs) {
	assert(s);
	assert(s->b.symType == TypeStruct || s->b.symType == TypeUnion);
	assert(s->u.s);
	assert(rfs);
	FILL_recFindStr(rfs, s, NULL, NULL,s_recFindCl++, 0, 0);
	recFindPush(s, rfs);
	return(rfs);
}

#if ZERO //DEBUG_ACCESS
#define DAP(xxx) xxx
#else
#define DAP(xxx) 
#endif

int javaOuterClassAccessible(S_symbol *cl) {
	S_javaStat 			*cs, *lcs;
	S_symStructSpecific	*nest;
	int					i,in,len;
DAP(fprintf(dumpOut,"testing class accessibility of %s\n",cl->linkName);fflush(dumpOut);)
	if (cl->b.accessFlags & ACC_PUBLIC) {
DAP(fprintf(dumpOut,"ret 1 access public\n"); fflush(dumpOut);)
		return(1);
	}
	/* default access, check whether it is in current package */
	assert(s_javaStat);
	if (javaClassIsInCurrentPackage(cl)) {
DAP(fprintf(dumpOut,"ret 1 default protection in current package\n"); fflush(dumpOut);)
		return(1);
	}
DAP(fprintf(dumpOut,"ret 0 on default\n"); fflush(dumpOut);)
	return(0);

}

static int javaRecordVisible(S_symbol *appcl, S_symbol *funcl, unsigned accessFlags) {
	// there is special case to check! Private symbols are not inherited!
	if (accessFlags & ACC_PRIVATE) {
		// check classes to string equality, just to be sure
		if (appcl!=funcl && strcmp(appcl->linkName, funcl->linkName)!=0) return(0);
	}
	return(1);
}

static int accessibleByDefaultAccessibility(S_recFindStr *rfs, S_symbol *funcl) {
	int 			i;
	S_symbol 		*cc;
	S_symbolList 	*sups;
	if (rfs==NULL) {
		// nested class checking, just check without inheritance checking
		return(javaClassIsInCurrentPackage(funcl));
	}
	// check accessibilities over inheritance hierarchy
	if (! javaClassIsInCurrentPackage(rfs->baseClass)) {
		return(0);
	}
	cc = rfs->baseClass;
	for(i=0; i<rfs->sti-1; i++) {
		assert(cc);
		for(sups=cc->u.s->super; sups!=NULL; sups=sups->next) {
			if (sups->next==rfs->st[i]) break;
		}
		if (sups!=NULL && sups->next == rfs->st[i]) {
			if (! javaClassIsInCurrentPackage(sups->d)) return(0);
		}
		cc = sups->d;
	}
	assert(cc==rfs->currClass);
	return(1);
}

// BERK, ther eis a copy of this function in jslsemact !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// when modifying this, you will need to change it there too
int javaRecordAccessible(S_recFindStr *rfs, S_symbol *appcl, S_symbol *funcl, S_symbol *rec, unsigned recAccessFlags) {
	S_javaStat 			*cs, *lcs;
	S_symStructSpecific	*nest;
	int					i,in,len;
	if (funcl == NULL) return(1);  /* argument or local variable */
	DAP(fprintf(dumpOut,"testing accessibility %s . %s of x%x\n",funcl->linkName,rec->linkName, recAccessFlags);fflush(dumpOut);)
		//&if ((s_opt.ooChecksBits & OOC_ACCESS_CHECK) == 0) {
//&DAP(fprintf(dumpOut,"ret 1, checking dissabled\n"); fflush(dumpOut);)
		//&	return(1);
		//&}
		assert(s_javaStat);
	if (recAccessFlags & ACC_PUBLIC) {
		DAP(fprintf(dumpOut,"ret 1 access public\n"); fflush(dumpOut);)
			return(1);
	}
	if (recAccessFlags & ACC_PROTECTED) {
		// doesn't it refers to application class?
		if (accessibleByDefaultAccessibility(rfs, funcl)) {
			DAP(fprintf(dumpOut,"ret 1 protected in current package\n"); fflush(dumpOut);)
				return(1);
		}
		for (cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
			if (cs->thisClass == funcl) {
				DAP(fprintf(dumpOut,"ret 1 as it is inside class\n"); fflush(dumpOut);)
					return(1);
			}
			if (cctIsMember(&cs->thisClass->u.s->casts, funcl, 1)) {
				DAP(fprintf(dumpOut,"ret 1 as it is inside subclass\n"); fflush(dumpOut);)
					return(1);
			}
		}
		DAP(fprintf(dumpOut,"ret 0 on protected\n"); fflush(dumpOut);)
			return(0);
	}
	if (recAccessFlags & ACC_PRIVATE) {
		// finally it seems that following is wrong and that private field
		// can be accessed from all classes within same major class
#if ZERO
		for(cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
			if (cs->thisClass == funcl) return(1);
		}
#endif
		// I thinked it was wrong, but it seems that it is definitely O.K.
		// it seems that if cl is defined inside top class, than it is O.K.
		for(lcs=cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
			lcs = cs;
		}
		if (lcs!=NULL && lcs->thisClass!=NULL) {
//&fprintf(dumpOut,"comparing %s and %s\n", lcs->thisClass->linkName, funcl->linkName);
			len = strlen(lcs->thisClass->linkName);
			if (strncmp(lcs->thisClass->linkName, funcl->linkName, len)==0) {
				DAP(fprintf(dumpOut,"ret 1 private inside the class\n"); fflush(dumpOut);)
					return(1);
			}
		}
		DAP(fprintf(dumpOut,"ret 0 on private\n"); fflush(dumpOut);)
			return(0);
	}
	/* default access */
	// it seems that here you should check rather if application class
	if (accessibleByDefaultAccessibility(rfs, funcl)) {
		DAP(fprintf(dumpOut,"ret 1 default protection in current package\n"); fflush(dumpOut);)
			return(1);
	}
	DAP(fprintf(dumpOut,"ret 0 on default\n"); fflush(dumpOut);)
		return(0);
}

int javaRecordVisibleAndAccessible(S_recFindStr *rfs, S_symbol *applCl, S_symbol *funCl, S_symbol *r) {
	return(
		javaRecordVisible(rfs->baseClass, rfs->currClass, r->b.accessFlags)
		&&
		javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, r->b.accessFlags)
		);
}

int javaGetMinimalAccessibility(S_recFindStr *rfs, S_symbol *r) {
	int acc, i;
	for(i=MAX_REQUIRED_ACCESS; i>0; i--) {
		acc = s_javaRequiredeAccessibilitiesTable[i];
		if (javaRecordVisible(rfs->baseClass, rfs->currClass, acc)
			&& javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, acc)) {
			return(i);
		}
	}
	return(i);
}

#define FSRS_RETURN_WITH_SUCCESS(ss,res,r) {\
	*res = r;\
	ss->nextRecord = r->next;\
	return(RETURN_OK);\
}


#define FSRS_RETURN_WITH_FAIL(ss,res) {\
	ss->nextRecord = NULL;\
	*res = &s_errorSymbol;\
	return(RETURN_NOT_FOUND);\
}

int findStrRecordSym(	S_recFindStr	*ss,
						char 			*recname,	 /* can be NULL */
						S_symbol 		**res,
						int 			javaClassif, /* classify to method/field*/
						int				accCheck,    /* java check accessibility */
						int 			visibilityCheck /* redundant, always equal to accCheck? */
	) {
	S_symbol 			*s,*r,*cclass;
	S_symbolList 		*sss;
	int 				m;

//&fprintf(dumpOut,":\nNEW SEARCH\n"); fflush(dumpOut);
	for(;;) {
		assert(ss);
		cclass = ss->currClass;
		if (cclass!=NULL&&cclass->u.s->recSearchCounter==ss->recsClassCounter){
			// to avoid multiple pass through the same super-class ??
//&fprintf(dumpOut,":%d==%d --> skipping class %s\n",cclass->u.s->recSearchCounter,ss->recsClassCounter,cclass->linkName);
			goto nextClass;
		}
//&if(cclass!=NULL)fprintf(dumpOut,":looking in class %s(%d)\n",cclass->linkName,ss->sti); fflush(dumpOut);
		for(r=ss->nextRecord; r!=NULL; r=r->next) {
			// special gcc extension of anonymous struct record
			if (r->name!=NULL && *r->name==0 && r->b.symType==TypeDefault
				&& r->u.type->m==TypeAnonymeField
				&& r->u.type->next!=NULL
				&& (r->u.type->next->m==TypeUnion || r->u.type->next->m==TypeStruct)) {
				// put the anonymous union as 'super class'
				if (ss->aui+1 < MAX_ANONYMOUS_FIELDS) {
					ss->au[ss->aui++] = r->u.type->next->u.t;
				}
			}
//&fprintf(dumpOut,":checking %s\n",r->name); fflush(dumpOut);
			if (recname==NULL || strcmp(r->name,recname)==0) {
				if (! LANGUAGE(LAN_JAVA)) {
					FSRS_RETURN_WITH_SUCCESS(ss, res, r);
				}
//&fprintf(dumpOut,"acc O.K., checking classif %d\n",javaClassif);fflush(dumpOut);
				if (javaClassif!=CLASS_TO_ANY) {
					assert(r->b.symType == TypeDefault);
					assert(r->u.type);
					m = r->u.type->m;
					if (m==TypeFunction && javaClassif!=CLASS_TO_METHOD) goto nextRecord;
					if (m!=TypeFunction && javaClassif==CLASS_TO_METHOD) goto nextRecord;
				}
//&if(cclass!=NULL)fprintf(dumpOut,"name O.K., checking accesibility %xd %xd\n",cclass->b.accessFlags,r->b.accessFlags); fflush(dumpOut);
				// I have it, check visibility and accessibility
				assert(r);
				if (visibilityCheck == VISIB_CHECK_YES) {
					if (! javaRecordVisible(ss->baseClass, cclass, r->b.accessFlags)) {
						// WRONG? return, Doesn't it iverrides any other of this name
						// Yes, definitely correct, in the first step determining
						// class to search
						FSRS_RETURN_WITH_FAIL(ss, res);
					}
				}
				if (accCheck == ACC_CHECK_YES) {
					if (! javaRecordAccessible(ss, ss->baseClass,cclass,r,r->b.accessFlags)){
						if (visibilityCheck == VISIB_CHECK_YES) {
							FSRS_RETURN_WITH_FAIL(ss, res);
						} else {
							goto nextRecord;
						}
					}
				}
				FSRS_RETURN_WITH_SUCCESS(ss, res, r);
			}
		nextRecord:;
		}
	nextClass:
		if (ss->aui!=0) {
			// O.K. try first to pas to anonymous record
			s = ss->au[--ss->aui];
		} else {
			// mark the class as processed
			if (cclass!=NULL) {
				cclass->u.s->recSearchCounter = ss->recsClassCounter;
			}

			while (ss->sti>0 && ss->st[ss->sti-1]==NULL) ss->sti--;
			if (ss->sti==0) {
				FSRS_RETURN_WITH_FAIL(ss, res);
			}
			sss = ss->st[ss->sti-1];
			s = sss->d;
			ss->st[ss->sti-1] = sss->next;
			assert(s && (s->b.symType==TypeStruct || s->b.symType==TypeUnion));
//&fprintf(dumpOut,":pass to super class %s(%d)\n",s->linkName,ss->sti); fflush(dumpOut);
		}
		recFindPush(s, ss);
	}
}

int findStrRecord(	S_symbol		*s,
					char 			*recname,	/* can be NULL */
					S_symbol 		**res,
					int 			javaClassif
				) {
	S_recFindStr rfs;
	return(findStrRecordSym(iniFind(s,&rfs),recname,res,javaClassif,
							ACC_CHECK_YES,VISIB_CHECK_YES));
}

/* and push reference */
// this should be split into two copies, different for C and Java.
S_reference *findStrRecordFromSymbol( S_symbol *sym, 
									  S_idIdent *record, 
									  S_symbol **res, 
									  int javaClassif,
									  S_idIdent *super /* covering special case when invoked
														  as SUPER.sym, berk */
	) {
	S_recFindStr 	rfs;
	S_reference 	rf, *ref;
	S_usageBits		ub;
	int rr, minacc;
	ref = NULL;
	// when in java, then always in qualified name, so access and visibility checks
	// are useless.
	rr = findStrRecordSym(iniFind(sym,&rfs),record->name,res,
						  javaClassif, ACC_CHECK_NO, VISIB_CHECK_NO);
	if (rr == RESULT_OK && rfs.currClass!=NULL &&
		((*res)->b.storage==StorageField 
		 || (*res)->b.storage==StorageMethod
		 || (*res)->b.storage==StorageConstructor)){
		assert(rfs.currClass->u.s && rfs.baseClass && rfs.baseClass->u.s);
		if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
			|| javaRecordVisibleAndAccessible(&rfs, rfs.baseClass, rfs.currClass, *res)) {
			minacc = javaGetMinimalAccessibility(&rfs, *res);
			FILL_usageBits(&ub, UsageUsed, minacc, 0);
			ref = addCxReferenceNew(*res,&record->p, &ub,
								 rfs.currClass->u.s->classFile,
								 rfs.baseClass->u.s->classFile);
			// this is adding reference to 'super', not to the field!
			// for pull-up/push-down
			if (super!=NULL) addThisCxReferences(s_javaStat->classFileInd,&super->p);
		}
	} else if (rr == RESULT_OK) {
		ref = addCxReference(*res,&record->p,UsageUsed, s_noneFileIndex, s_noneFileIndex);
	} else {
		noSuchRecordError(record->name);
	}
	return(ref);
}

S_reference * findStrRecordFromType(	S_typeModifiers *str,
										S_idIdent *record,
										S_symbol **res,
										int javaClassif
									) {
	S_reference *ref;
	int rr;
	assert(str);
	ref = NULL;
	if (str->m != TypeStruct && str->m != TypeUnion) {
		*res = &s_errorSymbol;
		goto fini;
	}
	ref = findStrRecordFromSymbol( str->u.t, record, res, javaClassif, NULL);
fini:
	return(ref);
}

void labelReference(S_idIdent *id, int usage) {
	char ttt[TMP_STRING_SIZE];
	char *tt;
	assert(id);
	if (LANGUAGE(LAN_JAVA)) {
		assert(s_javaStat&&s_javaStat->thisClass&&s_javaStat->thisClass->u.s);
		if (s_cp.function!=NULL) {
			sprintf(ttt,"%x-%s.%s",s_javaStat->thisClass->u.s->classFile,
					s_cp.function->name, id->name);
		} else {
			sprintf(ttt,"%x-.%s", s_javaStat->thisClass->u.s->classFile,
					id->name);
		}
	} else if (s_cp.function!=NULL) {
		tt = strmcpy(ttt, s_cp.function->name);
		*tt = '.';
		tt = strcpy(tt+1,id->name);
	} else {
		strcpy(ttt, id->name);
	}
	assert(strlen(ttt)<TMP_STRING_SIZE-1);
	addTrivialCxReference(ttt, TypeLabel,StorageDefault, &id->p, usage);
}

void setLocalVariableLinkName(struct symbol *p) {
	char ttt[TMP_STRING_SIZE];
	char nnn[TMP_STRING_SIZE];
	int len,tti;
	if (s_opt.cxrefs == OLO_EXTRACT) {
		// extract variable, I must pass all needed informations in linkname
		sprintf(nnn,"%c%s%c",	LINK_NAME_CUT_SYMBOL, p->name, 
				LINK_NAME_CUT_SYMBOL);
		ttt[0] = LINK_NAME_EXTRACT_DEFAULT_FLAG;
		// why it commented out ?
		//& if ((!LANGUAGE(LAN_JAVA)) 
		//& 	&& (p->u.type->m == TypeUnion || p->u.type->m == TypeStruct)) {
		//& 	ttt[0] = LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
		//& }
		sprintf(ttt+1,"%s", s_extractStorageName[p->b.storage]);
		tti = strlen(ttt);
		len = TMP_STRING_SIZE - tti;
		typeSPrint(ttt+tti, &len, p->u.type, nnn, LINK_NAME_CUT_SYMBOL, 0,1,SHORT_NAME, NULL);
		sprintf(ttt+tti+len,"%c%x-%x-%x-%x", LINK_NAME_CUT_SYMBOL,
			p->pos.file,p->pos.line,p->pos.coll, s_count.localVar++);
	} else {
		if (		p->b.storage==StorageExtern 
				|| 	p->b.storage==StorageTypedef
				||	p->b.storage==StorageConstant ) {
			sprintf(ttt,"%s", p->name);
		} else {
			// it is now better to have name allways accessible
//&			if (s_opt.taskRegime == RegimeHtmlGenerate) {
				// html symbol, must pass the name for cxreference list item
				sprintf(ttt,"%x-%x-%x%c%s",p->pos.file,p->pos.line,p->pos.coll,
						LINK_NAME_CUT_SYMBOL, p->name);
/*&
			} else {
				// no special information need to pass
				sprintf(ttt,"%x-%x-%x", p->pos.file,p->pos.line,p->pos.coll);
			}
&*/
		}
	}
	len = strlen(ttt);
	XX_ALLOCC(p->linkName, len+1, char);
	strcpy(p->linkName,ttt);
}

static void setStaticFunctionLinkName( S_symbol *p, int usage ) {
	char 		ttt[TMP_STRING_SIZE];
	int 		len,ii;
	char 		*ss,*basefname;
	S_symbol	*memb;
//&	if (! symTabIsMember(s_symTab, p, &ii, &memb)) {
	// follwing unifies static symbols taken from the same header files.
	// Static symbols can be used only after being defined, so it is sufficient
	// to do this on definition usage?
	// With exactPositionResolve interpret them as distinct symbols for
	// each compilation unit.
		if (usage==UsageDefined && ! s_opt.exactPositionResolve) {
			basefname=s_fileTab.tab[p->pos.file]->name;
		} else {
			basefname=s_input_file_name;
		}
		sprintf(ttt,"%s!%s", simpleFileName(basefname), p->name);
		len = strlen(ttt);
		InternalCheck(len < TMP_STRING_SIZE-2);
		XX_ALLOCC(ss, len+1, char);
		strcpy(ss, ttt);
		p->linkName = ss;
//&	} else {
//&		p->linkName=memb->linkName;
//&	}
}

S_symbol *addNewSymbolDef(S_symbol *p, unsigned theDefaultStorage, S_symTab *tab,
						  int usage) {
	S_typeModifiers *tt;
	if (p == &s_errorSymbol || p->b.symType==TypeError) return(p);
	if (p->b.symType == TypeError) return(p);
	assert(p && p->b.symType == TypeDefault && p->u.type);
	if (p->u.type->m == TypeFunction && p->b.storage == StorageDefault) {
		p->b.storage = StorageExtern;
	}
	if (p->b.storage == StorageDefault) {
		p->b.storage = theDefaultStorage;
	}
	if (p->b.symType==TypeDefault && p->b.storage==StorageTypedef) {
		// typedef HACK !!!
		XX_ALLOC(tt, S_typeModifiers);
		*tt = *p->u.type;
		p->u.type = tt;
		tt->typedefin = p;
	}
	// special care is given to linkNames for local variable 
	if (! WORK_NEST_LEVEL0()) {
		// local scope symbol
		setLocalVariableLinkName(p);
	} else if (p->b.symType==TypeDefault && p->b.storage==StorageStatic) {
		setStaticFunctionLinkName(p, usage);
	}
	//& if (IS_DEFINITION_OR_DECL_USAGE(usage)) addSymbol(p, tab); // maybe this is better
	addSymbol(p, tab);
	addCxReference(p, &p->pos, usage,s_noneFileIndex, s_noneFileIndex);
	return(p);
}

/* this function is dead man, nowhere used */
S_symbol *addNewCopyOfSymbolDef(S_symbol *def, unsigned storage) {
	S_symbol *p;
	p = StackMemAlloc(S_symbol);
	*p = *def;
	addNewSymbolDef(p,storage, s_symTab, UsageDefined);
	return(p);
}

S_symbol *addNewDeclaration(
							S_symbol *btype,
							S_symbol *decl, 
							unsigned storage,
							S_symTab *tab
							) {
	int usage;
	if (decl == &s_errorSymbol || btype == &s_errorSymbol
		|| decl->b.symType==TypeError || btype->b.symType==TypeError) {
		return(decl);
	}
	assert(decl->b.symType == TypeDefault);
	completeDeclarator(btype, decl);
	usage = UsageDefined;
	if (decl->u.type->m == TypeFunction) usage = UsageDeclared;
	else if (decl->b.storage == StorageExtern) usage = UsageDeclared;
	addNewSymbolDef(decl, storage, tab, usage);
	return(decl);
}

void addFunctionParameterToSymTable(S_symbol *function, S_symbol *p, int i, S_symTab *tab) {
	S_symbol 	*pp, *pa, *ppp;
	int			ii;
	if (p->name != NULL && p->b.symType!=TypeError) {
		assert(s_javaStat->locals!=NULL);
		XX_ALLOC(pa, S_symbol);
		*pa = *p;
		// here checks a special case, double argument definition do not
		// redefine him, so refactorings will detect problem
		for(pp=function->u.type->u.f.args; pp!=NULL && pp!=p; pp=pp->next) {
			if (pp->name!=NULL && pp->b.symType!=TypeError) {
				if (p!=pp && strcmp(pp->name, p->name)==0) break;
			}
		}
		if (pp!=NULL && pp!=p) {
			if (symTabIsMember(tab, pa, &ii, &ppp)) {
				addCxReference(ppp, &p->pos, UsageUsed, s_noneFileIndex, s_noneFileIndex);
			}
		} else {
			addNewSymbolDef(pa, StorageAuto, tab, UsageDefined);
		}
		if (s_opt.cxrefs == OLO_EXTRACT) {
			addCxReference(pa, &pa->pos, UsageLvalUsed, 
						   s_noneFileIndex, s_noneFileIndex);
		}
	}
	if (s_opt.cxrefs == OLO_GOTO_PARAM_NAME 
		&& i == s_opt.olcxGotoVal
		&& POSITION_EQ(function->pos, s_cxRefPos)) {
		s_paramPosition = p->pos;
	}
}

S_typeModifiers *crSimpleTypeMofifier(unsigned t) {
	S_typeModifiers	*p;
	assert(t>=0 && t<MAX_TYPE);
	if (s_preCrTypesTab[t] == NULL) {
		p = StackMemAlloc(S_typeModifiers);
		FILLF_typeModifiers(p,t,f,( NULL,NULL) ,NULL,NULL);
	} else {
		p = s_preCrTypesTab[t];
	}
/*fprintf(dumpOut,"t,p->m == %d %d == %s %s\n",t,p->m,typesName[t],typesName[p->m]); fflush(dumpOut);*/
	assert(p->m == t);
	return(p);
}

static S_typeModifiers *mergeBaseType(S_typeModifiers *t1,S_typeModifiers *t2){
	unsigned b,r;
	unsigned modif;
	assert(t1->m<MODIFIERS_END && t2->m<MODIFIERS_END);
	b=t1->m; modif=t2->m;// just to confuse compiler warning
	/* if both are types, error, return the new one only*/
	if (t1->m <= MODIFIERS_START && t2->m <= MODIFIERS_START) return(t2);
	/* if not use tables*/
	if (t1->m > MODIFIERS_START) {modif = t1->m; b = t2->m; }
	if (t2->m > MODIFIERS_START) {modif = t2->m; b = t1->m; }
	switch (modif) {
	case TmodLong:
		r = typeLongChange[b];
		break;
	case TmodShort:
		r = typeShortChange[b];
		break;
	case TmodSigned:
		r = typeSignedChange[b];
		break;
	case TmodUnsigned:
		r = typeUnsignedChange[b];
		break;
	case TmodShortSigned:
		r = typeSignedChange[b];
		r = typeShortChange[r];
		break;
	case TmodShortUnsigned:
		r = typeUnsignedChange[b];
		r = typeShortChange[r];
		break;
	case TmodLongSigned:
		r = typeSignedChange[b];
		r = typeLongChange[r];
		break;
	case TmodLongUnsigned:
		r = typeUnsignedChange[b];
		r = typeLongChange[r];
		break;
	default: assert(0);
	}
	return(crSimpleTypeMofifier(r));
}

static S_typeModifiers * mergeBaseModTypes(S_typeModifiers *t1, S_typeModifiers *t2) {
	assert(t1 && t2);
	if (t1->m == TypeDefault) return(t2);
	if (t2->m == TypeDefault) return(t1);
	assert(t1->m >=0 && t1->m<MAX_TYPE);
	assert(t2->m >=0 && t2->m<MAX_TYPE);
	if (s_preCrTypesTab[t2->m] == NULL) return(t2);  /* not base type*/
	if (s_preCrTypesTab[t1->m] == NULL) return(t1);  /* not base type*/
	return(mergeBaseType(t1, t2));
}

S_symbol *typeSpecifier2(S_typeModifiers *t) {
	S_symbol	*r;
	/* this is just temporary, when have not the tempmemory in java,c++ */
	if (LANGUAGE(LAN_C)) {
		SM_ALLOC(tmpWorkMemory, r, S_symbol);
	} else {
		XX_ALLOC(r, S_symbol);
	}
/*	XX_ALLOC(r, S_symbol);*/
	FILL_symbolBits(&r->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
	FILL_symbol(r,NULL,NULL,s_noPos,r->b,type,t,NULL);
	r->u.type = t;
	return(r);
}

S_symbol *typeSpecifier1(unsigned t) {
	S_symbol		*r;
	r = typeSpecifier2(crSimpleTypeMofifier(t));
	return(r);
}

void declTypeSpecifier1(S_symbol *d, unsigned t) {
	S_symbol	*r;
	assert(d && d->u.type);
	d->u.type = mergeBaseModTypes(d->u.type,crSimpleTypeMofifier(t));
}

void declTypeSpecifier2(S_symbol *d, S_typeModifiers *t) {
	unsigned b,rb;
	assert(d && d->u.type);
	d->u.type = mergeBaseModTypes(d->u.type, t);
}

void declTypeSpecifier21(S_typeModifiers *t, S_symbol *d) {
	unsigned 		b;
	assert(d && d->u.type);
	d->u.type = mergeBaseModTypes(t, d->u.type);
}

S_typeModifiers *appendComposedType(S_typeModifiers **d, unsigned t) {
	S_typeModifiers *p;
	p = StackMemAlloc(S_typeModifiers);
	FILLF_typeModifiers(p, t,f,( NULL,NULL) ,NULL,NULL);
	LIST_APPEND(S_typeModifiers, (*d), p);
	return(p);
}

S_typeModifiers *prependComposedType(S_typeModifiers *d, unsigned t) {
	S_typeModifiers *p;
	p = StackMemAlloc(S_typeModifiers);
	FILLF_typeModifiers(p, t,f,( NULL,NULL) ,NULL,d);
	return(p);
}

#if ZERO
void completeDeclarator(S_symbol *t, S_symbol *d) {
	assert(t && d);
	if (t == &s_errorSymbol || d == &s_errorSymbol) return;
	unpackPointers(d);
	LIST_APPEND(S_typeModifiers, d->u.type, t->u.type);
	d->b.storage = t->b.storage;
}
#endif

void completeDeclarator(S_symbol *t, S_symbol *d) {
	S_typeModifiers *tt,**dt;
//static int counter=0;
	assert(t && d);
	if (t == &s_errorSymbol || d == &s_errorSymbol
		|| t->b.symType==TypeError || d->b.symType==TypeError) return;
	d->b.storage = t->b.storage;
	assert(t->b.symType==TypeDefault);
	dt = &(d->u.type); tt = t->u.type;
	if (d->b.npointers) {
		if (d->b.npointers>=1 && (tt->m==TypeStruct||tt->m==TypeUnion)
			&& tt->typedefin==NULL) {
//fprintf(dumpOut,"saving 1 str pointer:%d\n",counter++);fflush(dumpOut);
			d->b.npointers--;
//if(d->b.npointers) {fprintf(dumpOut,"possible 2\n");fflush(dumpOut);}
			assert(tt->u.t && tt->u.t->b.symType==tt->m && tt->u.t->u.s);
			tt = & tt->u.t->u.s->sptrtype;
		} else if (d->b.npointers>=2 && s_preCrPtr2TypesTab[tt->m]!=NULL
			&& tt->typedefin==NULL) {
			assert(tt->next==NULL);	/* not a user defined type */
//fprintf(dumpOut,"saving 2 pointer\n");fflush(dumpOut);
			d->b.npointers-=2;
			tt = s_preCrPtr2TypesTab[tt->m];
		} else if (d->b.npointers>=1 && s_preCrPtr1TypesTab[tt->m]!=NULL
			&& tt->typedefin==NULL) {
			assert(tt->next==NULL);	/* not a user defined type */
//fprintf(dumpOut,"saving 1 pointer\n");fflush(dumpOut);
			d->b.npointers--;
			tt = s_preCrPtr1TypesTab[tt->m];
		}
	}
	unpackPointers(d);
	LIST_APPEND(S_typeModifiers, *dt, tt);
}

S_symbol *crSimpleDefinition(unsigned storage, unsigned t, S_idIdent *id) {
	S_typeModifiers *p;
	S_symbol	*r;
	p = StackMemAlloc(S_typeModifiers);
	FILLF_typeModifiers(p,t,f,( NULL,NULL) ,NULL,NULL);
	r = StackMemAlloc(S_symbol);
	if (id!=NULL) {
		FILL_symbolBits(&r->b,0,0,0,0,0,TypeDefault,storage,0);
		FILL_symbol(r,id->name,id->name,id->p,r->b,type,p,NULL);
	} else {
		FILL_symbolBits(&r->b,0,0,0,0,0,TypeDefault,storage,0);
		FILL_symbol(r,NULL, NULL, s_noPos,r->b,type,p,NULL);
	}
	r->u.type = p;
	return(r);
}

S_symbolList *crDefinitionList(S_symbol *d) {
	S_symbolList *p;
	assert(d);
	p = StackMemAlloc(S_symbolList);
	FILL_symbolList(p, d, NULL);
	return(p);
}

int mergeArguments(S_symbol *id, S_symbol *ty) {
	S_symbol *p;
	int res;
	res = RESULT_OK;
	/* if a type of non-exist. argument is declared, it is probably */
	/* only a missing ';', so syntax error should be raised */
	for(;ty!=NULL; ty=ty->next) {
		if (ty->name != NULL) {
			for(p=id; p!=NULL; p=p->next) {
				if (p->name!=NULL && !strcmp(p->name,ty->name)) break;
			}
			if (p==NULL) res = RESULT_ERR;
			else {
				if (p->u.type == NULL) p->u.type = ty->u.type;
			}
		}
	}
	return(res);
}

static S_typeModifiers *crSimpleEnumType(S_symbol *edef, int type) {
	S_typeModifiers *res;
	res = StackMemAlloc(S_typeModifiers);
	FILLF_typeModifiers(res, type,t,edef,NULL,NULL);
	res->u.t = edef;
	return(res);
}

S_typeModifiers *simpleStrUnionSpecifier(	S_idIdent *typeName, 
											S_idIdent *id, 
											int usage
										) {
	S_symbol p,*pp;
	int ii,type;
/*fprintf(dumpOut, "new str %s\n",id->name); fflush(dumpOut);*/
	assert(typeName && typeName->sd && typeName->sd->b.symType == TypeKeyword);
	assert(		typeName->sd->u.keyWordVal == STRUCT 
			|| 	typeName->sd->u.keyWordVal == CLASS
			|| 	typeName->sd->u.keyWordVal == UNION
		);
	if (typeName->sd->u.keyWordVal != UNION) type = TypeStruct;
	else type = TypeUnion;
	FILL_symbolBits(&p.b,0,0, 0,0,0, type, StorageNone,0);
	FILL_symbol(&p, id->name, id->name, id->p,p.b,s,NULL, NULL);
	p.u.s = NULL;
	if (! symTabIsMember(s_symTab,&p,&ii,&pp)){
//{static int c=0;fprintf(dumpOut,"str#%d\n",c++);}
		XX_ALLOC(pp, S_symbol);
		*pp = p;
		XX_ALLOC(pp->u.s, S_symStructSpecific);
		FILLF_symStructSpecific(pp->u.s, NULL,
							NULL,NULL,NULL,0,NULL,
							type,f,(NULL,NULL),NULL,NULL,
							TypePointer,f,(NULL,NULL),NULL,&pp->u.s->stype,
							0,0, -1,0);
		pp->u.s->stype.u.t = pp;
		setGlobalFileDepNames(id->name, pp, MEM_XX);
		addSymbol(pp, s_symTab);
	}
	addCxReference(pp, &id->p, usage,s_noneFileIndex, s_noneFileIndex);
	return(&pp->u.s->stype);
}

static int isStaticFun(S_symbol *p) {
	return(p->b.symType==TypeDefault && p->b.storage==StorageStatic
		   && p->u.type->m == TypeFunction);
}

void setGlobalFileDepNames(char *iname, S_symbol *pp, int memory) {
	char 			*mname, *fname;
	char			tmp[MACRO_NAME_SIZE];
	S_symbol		*memb;
	int				ii,rr,filen, order, len, len2;
	if (iname == NULL) iname="";
	assert(pp);
	if (s_opt.exactPositionResolve) {
		fname = simpleFileName(s_fileTab.tab[pp->pos.file]->name);
		sprintf(tmp, "%x-%s-%x-%x%c", 
				hashFun(s_fileTab.tab[pp->pos.file]->name),
				fname, pp->pos.line, pp->pos.coll, 
				LINK_NAME_CUT_SYMBOL);
	} else if (iname[0]==0) {
		// anonymous structure/union ...
		filen = pp->pos.file;
		pp->name=iname; pp->linkName=iname;
		order = 0;
		rr = symTabIsMember(s_symTab, pp, &ii, &memb);
		while (rr) {
			if (memb->pos.file==filen) order++;
			rr = symTabNextMember(pp, &memb);
		}
		fname = simpleFileName(s_fileTab.tab[filen]->name);
		sprintf(tmp, "%s%c%d%c", fname, SLASH, order, LINK_NAME_CUT_SYMBOL);
/*&		// macros will be identified by name only?
	} else if (pp->b.symType == TypeMacro) {
		sprintf(tmp, "%x%c", pp->pos.file, LINK_NAME_CUT_SYMBOL);
&*/
	} else {
		tmp[0] = 0;
	}
	len = strlen(tmp);
	len2 = len + strlen(iname);
	InternalCheck(len < MACRO_NAME_SIZE-2);
	if (memory == MEM_XX) {
		XX_ALLOCC(mname, len2+1, char);
	} else {
		PP_ALLOCC(mname, len2+1, char);
	}
	strcpy(mname, tmp);
	strcpy(mname+len,iname);
	pp->name = mname + len;
	pp->linkName = mname;
}

#if ZERO
void setGlobalFileDepNames(char *iname, S_symbol *pp, int memory) {
	char 			*mname, *fname;
	char			tmp[MACRO_NAME_SIZE];
	S_symbol		*memb;
	int				ii,rr, order, len, len2;
	if (iname == NULL) iname="";

	assert(pp);
	sprintf(tmp, "%x-%x%c", pp->pos.file, pp->pos.line, LINK_NAME_CUT_SYMBOL);

	len = strlen(tmp);
	len2 = len + strlen(iname);
	InternalCheck(len < MACRO_NAME_SIZE-2);
	if (memory == MEM_XX) {
		XX_ALLOCC(mname, len2+1, char);
	} else {
		PP_ALLOCC(mname, len2+1, char);
	}
	strcpy(mname, tmp);
	strcpy(mname+len,iname);
	pp->name = mname + len;
	pp->linkName = mname;
}
#endif

S_typeModifiers *crNewAnnonymeStrUnion(S_idIdent *typeName) {
	S_symbol *pp;
	int type,ti,line,coll,nlen,nnlen;
	char *name,*fname,*fsname;
	char  ttt[TMP_STRING_SIZE];
	assert(typeName);
	assert(typeName->sd);
	assert(typeName->sd->b.symType == TypeKeyword);
	assert(		typeName->sd->u.keyWordVal == STRUCT 
			||	typeName->sd->u.keyWordVal == CLASS
			||	typeName->sd->u.keyWordVal == UNION
		);
	if (typeName->sd->u.keyWordVal == STRUCT) type = TypeStruct;
	else type = TypeUnion;

	pp = StackMemAlloc(S_symbol);
	FILL_symbolBits(&pp->b,0,0, 0,0,0, type, StorageNone,0);
	FILL_symbol(pp, "", NULL, typeName->p,pp->b,type,NULL, NULL);
	setGlobalFileDepNames("", pp, MEM_XX);
#if ZERO
	if (s_opt.exactPositionResolve) {
		sprintf(ttt, "%s-%x%c", 
				simpleFileName(s_fileTab.tab[typeName->p.file]->name),
				typeName->p.line,
				LINK_NAME_CUT_SYMBOL);
		XX_ALLOCC(pp->linkName, strlen(ttt)+1, char);
		strcpy(pp->linkName, ttt);
	}
#endif
	XX_ALLOC(pp->u.s, S_symStructSpecific);
	FILLF_symStructSpecific(pp->u.s, NULL,
							NULL, NULL, NULL, 0, NULL,
							type,f,(NULL,NULL),NULL,NULL,
							TypePointer,f,(NULL,NULL),NULL,&pp->u.s->stype,
							0,0, -1,0);
	pp->u.s->stype.u.t = pp;
	addSymbol(pp, s_symTab);
	return(&pp->u.s->stype);
}

void specializeStrUnionDef(S_symbol *sd, S_symbol *rec) {
	S_symbol *dd;
	assert(sd->b.symType == TypeStruct || sd->b.symType == TypeUnion);
	assert(sd->u.s);
	if (sd->u.s->records!=NULL) return;
	sd->u.s->records = rec;
	addToTrail(setToNull, & (sd->u.s->records) );
	for(dd=rec; dd!=NULL; dd=dd->next) {
		if (dd->name!=NULL) {
			dd->linkName = string3ConcatInStackMem(sd->linkName,".",dd->name);
			dd->b.record = 1;
			if (	LANGUAGE(LAN_CCC) && dd->b.symType==TypeDefault 
					&&	dd->u.type->m==TypeFunction) {
				dd->u.type->u.f.thisFunList = &sd->u.s->records;
			}
			addCxReference(dd,&dd->pos,UsageDefined,s_noneFileIndex, s_noneFileIndex);
		}
	}
}

S_typeModifiers *simpleEnumSpecifier(S_idIdent *id, int usage) {
	S_symbol p,*pp;
	int ii;
	FILL_symbolBits(&p.b,0,0, 0,0,0, TypeEnum, StorageNone,0);
	FILL_symbol(&p, id->name, id->name, id->p,p.b,enums,NULL, NULL);
	p.u.enums = NULL;
	if (! symTabIsMember(s_symTab,&p,&ii,&pp)) {
		pp = StackMemAlloc(S_symbol);
		*pp = p;
		addSymbol(pp, s_symTab);
	}
	addCxReference(pp, &id->p, usage,s_noneFileIndex, s_noneFileIndex);
	return(crSimpleEnumType(pp,TypeEnum));
}

S_typeModifiers *crNewAnnonymeEnum(S_symbolList *enums) {
	S_symbol *pp;
	pp = StackMemAlloc(S_symbol);
	FILL_symbolBits(&pp->b,0,0, 0,0,0, TypeEnum, StorageNone,0);
	FILL_symbol(pp, "", "", s_noPos,pp->b,enums,enums, NULL);
	pp->u.enums = enums;
	return(crSimpleEnumType(pp,TypeEnum));
}

void appendPositionToList( S_positionLst **list,S_position *pos) {
	S_positionLst *ppl;
	XX_ALLOC(ppl, S_positionLst);
	FILL_positionLst(ppl, *pos, NULL);
	LIST_APPEND(S_positionLst, (*list), ppl);
}

void setParamPositionForFunctionWithoutParams(S_position *lpar) {
	s_paramBeginPosition = *lpar;
	s_paramEndPosition = *lpar;
}

void setParamPositionForParameter0(S_position *lpar) {
	s_paramBeginPosition = *lpar;
	s_paramEndPosition = *lpar;
}

void setParamPositionForParameterBeyondRange(S_position *rpar) {
	s_paramBeginPosition = *rpar;
	s_paramEndPosition = *rpar;
}

static void handleParameterPositions(S_position *lpar, S_positionLst *commas, 
									 S_position *rpar, int hasParam) {
	int i, argn;
	S_position *p1, *p2;
	S_positionLst *pp;
	if (! hasParam) {
		setParamPositionForFunctionWithoutParams(lpar);
		return;
	}
	argn = s_opt.olcxGotoVal;
	if (argn == 0) {
		setParamPositionForParameter0(lpar);
	} else {
		pp = commas;
		p1 = lpar;
		i = 1;
		if (pp != NULL) p2 = &pp->p;
		else p2 = rpar;
		for(i++; pp!=NULL && i<=argn; pp=pp->next,i++) {
			p1 = &pp->p;
			if (pp->next != NULL) p2 = &pp->next->p;
			else p2 = rpar;
		}
		if (pp==NULL && i<=argn) {
			setParamPositionForParameterBeyondRange(rpar);
		} else {
			s_paramBeginPosition = *p1;
			s_paramEndPosition = *p2;
		}
	}
}

S_symbol *crEmptyField() {
	S_symbol *res;
	S_typeModifiers *p;
	p = StackMemAlloc(S_typeModifiers);
	FILLF_typeModifiers(p,TypeAnonymeField,f,( NULL,NULL) ,NULL,NULL);
	res = StackMemAlloc(S_symbol);
	FILL_symbolBits(&res->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
	FILL_symbol(res, "", "", s_noPos,res->b,type,p,NULL);
	res->u.type = p;
	return(res);
}

void handleDeclaratorParamPositions(S_symbol *decl, S_position *lpar,
									S_positionLst *commas, S_position *rpar,
									int hasParam
	) {
	if (s_opt.taskRegime != RegimeEditServer) return;
	if (s_opt.cxrefs != OLO_GOTO_PARAM_NAME && s_opt.cxrefs != OLO_GET_PARAM_COORDINATES) return;
	if (POSITION_NEQ(decl->pos, s_cxRefPos)) return;
	handleParameterPositions(lpar, commas, rpar, hasParam);
}

void handleInvocationParamPositions(S_reference *ref, S_position *lpar,
									S_positionLst *commas, S_position *rpar,
									int hasParam
	) {
	if (s_opt.taskRegime != RegimeEditServer) return;
	if (s_opt.cxrefs != OLO_GOTO_PARAM_NAME && s_opt.cxrefs != OLO_GET_PARAM_COORDINATES) return;
	if (ref==NULL || POSITION_NEQ(ref->p, s_cxRefPos)) return;
	handleParameterPositions(lpar, commas, rpar, hasParam);
}

void javaHandleDeclaratorParamPositions(S_position *sym, S_position *lpar,
										S_positionLst *commas, S_position *rpar
	) {
	if (s_opt.taskRegime != RegimeEditServer) return;
	if (s_opt.cxrefs != OLO_GOTO_PARAM_NAME && s_opt.cxrefs != OLO_GET_PARAM_COORDINATES) return;
	if (POSITION_NEQ(*sym, s_cxRefPos)) return;
	if (commas==NULL) {
		handleParameterPositions(lpar, NULL, rpar, 0);
	} else {
		handleParameterPositions(lpar, commas->next, rpar, 1);
	}
}


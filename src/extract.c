/*
	$Revision: 1.9 $
	$Date: 2002/08/20 16:17:03 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "unigram.h"
#include "protocol.h"
//

#define EXTRACT_GEN_BUFFER_SIZE 500000

static unsigned s_javaExtractFromFunctionMods=ACC_DEFAULT;
static char	*rb;
static char *s_extractionName;

static void dumpProgram(S_programGraphNode *program) {
	S_programGraphNode *p;
	fprintf(dumpOut,"[ProgramDump]\n");
	for(p=program; p!=NULL; p=p->next) {
		fprintf(dumpOut,"%x: %2d %2d %s %s",p,
			p->posBits,p->stateBits,
			p->symRef->name,
			usagesName[p->ref->usg.base]+5);
		if (p->symRef->b.symType==TypeLabel && p->ref->usg.base!=UsageDefined) {
			fprintf(dumpOut," %x",p->jump);
		}
		fprintf(dumpOut,"\n");
	}
	fflush(dumpOut);
}

#define LOCAL_LABEL_NAME(ttt, counter) {\
    sprintf(ttt,"%%L%d",counter);\
    InternalCheck(strlen(ttt) < TMP_STRING_SIZE-1);\
}

void genInternalLabelReference(int counter, int usage) {
	char ttt[TMP_STRING_SIZE];
	S_idIdent ll;
	LOCAL_LABEL_NAME(ttt,counter);
	FILLF_idIdent(&ll, ttt, NULL, cFile.lb.cb.fileNumber, 0,0);
	if (usage != UsageDefined) ll.p.line++; 
	// line == 0 or 1 , (hack to get definition first)
	labelReference(&ll, usage);
}


S_symbol * addContinueBreakLabelSymbol(int labn, char *name) {
	S_symbol *s;
	XX_ALLOC(s, S_symbol);
	FILL_symbolBits(&s->b,0,0,0,0,0,TypeLabel,StorageAuto,0);
	FILL_symbol(s,name,name,s_noPos,s->b,labn,labn,NULL);
	AddSymbolNoTrail(s, s_symTab);
	return(s);
}


void deleteContinueBreakLabelSymbol(char *name) {
	S_symbol 	ss,*memb;
	int 		ii;
	FILL_symbolBits(&ss.b, 0,0,0,0,0,TypeLabel,StorageAuto,0);
	FILL_symbol(&ss, name, name, s_noPos, ss.b,labn,0, NULL);
	if (symTabIsMember(s_symTab, &ss, &ii, &memb)) {
	  ExtrDeleteContBreakSym(memb);
	} else {
	  assert(0);
	}
}

void genContinueBreakReference(char *name) {
	S_symbol 	ss,*memb;
	int 		ii;
	FILL_symbolBits(&ss.b, 0,0,0,0,0,TypeLabel,StorageAuto,0);
	FILL_symbol(&ss, name, name, s_noPos, ss.b,labn,0, NULL);
	if (symTabIsMember(s_symTab, &ss, &ii, &memb)) {
		genInternalLabelReference(memb->u.labn, UsageUsed);
	}
}

void genSwitchCaseFork(int lastFlag) {
	S_symbol 	ss,*memb;
	int 		ii;
	FILL_symbolBits(&ss.b, 0,0,0,0,0,TypeLabel,StorageAuto,0);
	FILL_symbol(&ss, SWITCH_LABEL_NAME, SWITCH_LABEL_NAME, s_noPos, ss.b,labn,0,NULL);
	if (symTabIsMember(s_symTab, &ss, &ii, &memb)) {
		genInternalLabelReference(memb->u.labn, UsageDefined);
		if (! lastFlag) {
			memb->u.labn++;
			genInternalLabelReference(memb->u.labn, UsageFork);
		}
	}
}

static void extractFunGraphRef(S_symbolRefItem *rr, void *prog) {
	S_reference *r;
	S_programGraphNode *p,**ap;
	ap = (S_programGraphNode **) prog;
	for(r=rr->refs; r!=NULL; r=r->next) {
		if (DM_IS_BETWEEN(cxMemory,r,s_cp.cxMemiAtFunBegin,s_cp.cxMemiAtFunEnd)){
			CX_ALLOC(p, S_programGraphNode);
			FILL_programGraphNode(p, r, rr, NULL, 0, 0, EXTRACT_NONE, *ap);
			*ap = p;
		}
	}
}

static S_programGraphNode *getGraphAddress(	S_programGraphNode	*program,
											S_reference			*ref
										) {
	S_programGraphNode *p,*res;
	res = NULL;
	for(p=program; res==NULL && p!=NULL; p=p->next) {
		if (p->ref == ref) res = p;
	}
	return(res);
}

static S_reference *getDefinitionReference(S_symbolRefItem *lab) {
	S_reference *res;
	for(res=lab->refs; res!=NULL && res->usg.base!=UsageDefined; res=res->next) ;
	if (res == NULL) {
		sprintf(tmpBuff,"jump to unknown label '%s'\n",lab->name);
		error(ERR_ST,tmpBuff);
	}
	return(res);
}

static S_programGraphNode *getLabelGraphAddress(S_programGraphNode *program,
												S_symbolRefItem 	*lab
											) {
	S_programGraphNode 	*res;
	S_reference			*defref;
	assert(lab->b.symType == TypeLabel);
	defref = getDefinitionReference(lab);
	res = getGraphAddress(program, defref);
	return(res);
}

static int linearOrder(S_programGraphNode *n1, S_programGraphNode *n2) {
	return(n1->ref < n2->ref);
}

static S_programGraphNode * extMakeProgramGraph() {
	S_programGraphNode *program,*p;
	program = NULL;
	refTabMap5(&s_cxrefTab, extractFunGraphRef, ((void *) &program));
	LIST_SORT(S_programGraphNode, program, linearOrder);
//&dumpProgram(program);
	for(p=program; p!=NULL; p=p->next) {
		if (p->symRef->b.symType==TypeLabel && p->ref->usg.base!=UsageDefined) {
			// resolve the jump
			p->jump = getLabelGraphAddress(program, p->symRef);
		}
	}
	return(program);
}

#define IS_SETTING_USAGE(usage) (\
	(usage)==UsageLvalUsed || (usage)==UsageAddrUsed \
)
#define TOGGLE_IN_OUT_BLOCK(pos) {\
	if (pos==INSP_INSIDE_BLOCK) pos = INSP_OUTSIDE_BLOCK;\
	else if (pos==INSP_OUTSIDE_BLOCK) pos = INSP_INSIDE_BLOCK;\
	else assert(0);\
}

#define IS_STR_UNION_SYMBOL(ref) (ref->symRef->name[0]==LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG)

static void extSetSetStates(	S_programGraphNode *p,
								S_symbolRefItem	*symRef,
								unsigned cstate
							) {
	unsigned cpos,oldStateBits;
	for(; p!=NULL; p=p->next) {
		cont:
		if (p->stateBits == cstate) return;
		oldStateBits = p->stateBits;
		cstate = p->stateBits = (cstate | oldStateBits | INSP_VISITED);
		cpos = p->posBits | INSP_VISITED;
		if (p->symRef == symRef) {			// the examined variable
			if (p->ref->usg.base == UsageAddrUsed) {
				cstate = cpos;
				// change only state, so usage is kept
			} else if (p->ref->usg.base == UsageLvalUsed
					|| (p->ref->usg.base == UsageDefined
						&& ! IS_STR_UNION_SYMBOL(p))
				) {
				// change also current value, because there is no usage
				p->stateBits = cstate = cpos;
			}
		} else if (p->symRef->b.symType==TypeBlockMarker &&
					(p->posBits&INSP_INSIDE_BLOCK) == 0) {
			// leaving the block
			if (cstate & INSP_INSIDE_BLOCK) {
				// leaving and value is set from inside, preset for possible
				// reentering
				cstate |= INSP_INSIDE_REENTER;
			}
			if (cstate & INSP_OUTSIDE_BLOCK) {
				// leaving and value is set from outside, set value passing flag
				cstate |= INSP_INSIDE_PASSING;
			}
		} else if (p->symRef->b.symType==TypeLabel) {
			if (p->ref->usg.base==UsageUsed) {	// goto
				p = p->jump;
				goto cont;
			} else if (p->ref->usg.base==UsageFork) {	// branching
				extSetSetStates(p->jump, symRef, cstate);
			}
		}
	}
}

static int extCategorizeLocalVar0(	S_programGraphNode *program,
									S_programGraphNode *varRef
								) {
	S_programGraphNode *p;
	S_symbolRefItem		*symRef;
	unsigned	inUsages,outUsages,outUsageBothExists;
	symRef = varRef->symRef;
	for(p=program; p!=NULL; p=p->next) {
		p->stateBits = 0;
	}
//&dumpProgram(program);
	extSetSetStates(program, symRef, INSP_VISITED);
//&dumpProgram(program);
	inUsages = outUsages = outUsageBothExists = 0;
	for(p=program; p!=NULL; p=p->next) {
		if (p->symRef == varRef->symRef && p->ref->usg.base != UsageNone) {
			if (p->posBits==INSP_INSIDE_BLOCK) {
				inUsages |= p->stateBits;
			} else if (p->posBits==INSP_OUTSIDE_BLOCK) {
				outUsages |= p->stateBits;
				//&if (	(p->stateBits & INSP_INSIDE_BLOCK)
				//&	&&	(p->stateBits & INSP_OUTSIDE_BLOCK)) {
					// a reference outside using values set in both in and out
				//&		outUsageBothExists = 1;
				//&}
			} else assert(0);
		}
	}
//&fprintf(dumpOut,"%% ** variable '%s' ", varRef->symRef->name);fprintf(dumpOut,"in, out Usages %o %o\n",inUsages,outUsages);
	// inUsages marks usages in the block (from inside, or from ouside)
	// outUsages marks usages out of block (from inside, or from ouside)
	if (varRef->posBits == INSP_OUTSIDE_BLOCK) {
		// a variable defined outside of the block
		if (outUsages & INSP_INSIDE_BLOCK) {
			// a value set in the block is used outside
//&sprintf(tmpBuff,"testing %s: %d %d %d\n", varRef->symRef->name, (inUsages & INSP_INSIDE_REENTER),(inUsages & INSP_OUTSIDE_BLOCK), outUsageBothExists);ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
			if ((inUsages & INSP_INSIDE_REENTER) == 0
				&& (inUsages & INSP_OUTSIDE_BLOCK) == 0
				/*& && outUsageBothExists == 0 &*/
				&& (outUsages & INSP_INSIDE_PASSING) == 0
				) {
				return(EXTRACT_OUT_ARGUMENT);
			} else {
				return(EXTRACT_IN_OUT_ARGUMENT);
			}
		}
		if (inUsages & INSP_INSIDE_REENTER) return(EXTRACT_IN_OUT_ARGUMENT);
		if (inUsages & INSP_OUTSIDE_BLOCK) return(EXTRACT_VALUE_ARGUMENT);
		if (inUsages) return(EXTRACT_LOCAL_VAR);
		return(EXTRACT_NONE);
	} else {
		if (	outUsages & INSP_INSIDE_BLOCK
			||	outUsages & INSP_OUTSIDE_BLOCK) {
			// a variable defined inside the region used outside
//&fprintf(dumpOut,"%% ** variable '%s' defined inside the region used outside", varRef->symRef->name);
			return(EXTRACT_LOCAL_OUT_ARGUMENT);
		} else {
			return(EXTRACT_NONE);
		}
	}
}

static int extCategorizeLocalVar(	S_programGraphNode *program,
									S_programGraphNode *varRef
								) {
	int res;
	res = extCategorizeLocalVar0(program, varRef);
//&fprintf(dumpOut,"classified to %s\n", miscellaneousName[res]);
	if (IS_STR_UNION_SYMBOL(varRef)
		&& res!=EXTRACT_NONE && res!=EXTRACT_LOCAL_VAR) {
		return(EXTRACT_ADDRESS_ARGUMENT);
	}
	return(res);
}

static void extSetInOutBlockFields(S_programGraphNode *program) {
	S_programGraphNode *p;
	unsigned	pos;
	pos = INSP_OUTSIDE_BLOCK;
	for(p=program; p!=NULL; p=p->next) {
		if (p->symRef->b.symType == TypeBlockMarker) TOGGLE_IN_OUT_BLOCK(pos);
		p->posBits = pos;
	}
}

static int extIsJumpInOutBlock(S_programGraphNode *program) {
	S_programGraphNode *p;
	for(p=program; p!=NULL; p=p->next) {
		assert(p->symRef!=NULL)
		if (p->symRef->b.symType==TypeLabel) {
			assert(p->ref!=NULL);
			if (p->ref->usg.base==UsageUsed || p->ref->usg.base==UsageFork) {
				assert(p->jump != NULL);
				if (p->posBits != p->jump->posBits) {
//&fprintf(dumpOut,"jump in/out at %s : %x\n",p->symRef->name, p);
					return(1);
				}
			}
		}
	}
	return(0);
}

#if ZERO
#define EXT_LOCAL_VAR_REF(ppp) (\
        ppp->ref->usg.base==UsageDefined \
    &&  ppp->symRef->b.symType==TypeDefault \
)
#else
#define EXT_LOCAL_VAR_REF(ppp) (\
		ppp->ref->usg.base==UsageDefined \
	&& 	ppp->symRef->b.symType==TypeDefault \
	&&	ppp->symRef->b.scope==ScopeAuto \
) 
#endif

static void extClassifyLocalVariables(S_programGraphNode *program) {
	S_programGraphNode *p;
	for(p=program; p!=NULL; p=p->next) {
		if (EXT_LOCAL_VAR_REF(p)) {
			p->classifBits = extCategorizeLocalVar(program,p);
//&fprintf(dumpOut,": checking var %s\n",p->symRef->name);dumpProgram(program);fprintf(dumpOut,": var %s will be %s\n\n\n",p->symRef->name,miscellaneousName[p->classifBits]);fflush(dumpOut);
		}
	}
}

/*	linkName -> oName  == name of the variable
	         -> oDecl  == declaration of the variable
			              (decl contains declStar string(pt_), before the name)
			 -> oDecla == declarator
			 ( if cpName == 0, then do not copy the original var name )
*/
#define GetLocalVarStringFromLinkName(linkName,oDecla,oName,oDecl,declStar,cpName) {\
	register char *s,*d,*dn,*dd;\
	int nLen;\
/*&fprintf(dumpOut, ":analyzing '%s'\n",linkName);fflush(dumpOut);&*/\
	for(	s=linkName+1,d=oDecl,dd=oDecla;\
			*s!=0 && *s!=LINK_NAME_CUT_SYMBOL; \
			s++,d++,dd++) {\
		*d = *dd = *s;\
	}\
    *dd = 0;\
	assert(*s);\
	for(s++; *s!=0 && *s!=LINK_NAME_CUT_SYMBOL;	s++,d++) {\
		*d = *s;\
	}\
	assert(*s);\
	d = strmcpy(d, declStar);\
	for(dn=oName,s++; *s!=0 && *s!=LINK_NAME_CUT_SYMBOL; s++,dn++) {\
		*dn = *s;\
		if (cpName) *d++ = *s;\
	}\
	assert(*s);\
	*dn = 0;\
	assert(*s);\
	for(s++; *s!=0 && *s!=LINK_NAME_CUT_SYMBOL; s++,d++) {\
		*d = *s;\
	}\
	*d = 0;\
}

static void extReClassifyIOVars(S_programGraphNode *program) {
	S_programGraphNode 	*p,*op;
	int 				uniqueOutFlag;

	op = NULL; uniqueOutFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (s_opt.extractMode == EXTR_FUNCTION_ADDRESS_ARGS) {
			if (		p->classifBits == EXTRACT_OUT_ARGUMENT
					||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
					||	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
				) {
				p->classifBits = EXTRACT_ADDRESS_ARGUMENT;
			}
		} else if (s_opt.extractMode == EXTR_FUNCTION) {
			if (p->classifBits == EXTRACT_OUT_ARGUMENT
				|| p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
				) {
				if (op == NULL) op = p;
				else uniqueOutFlag = 0;
				// re-classify to in_out
			}
		}
	}

	if (op!=NULL && uniqueOutFlag) {
		if (op->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
			op->classifBits = EXTRACT_LOCAL_RESULT_VALUE;
		} else {
			op->classifBits = EXTRACT_RESULT_VALUE;
		}
		return;
	}

	op = NULL; uniqueOutFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (s_opt.extractMode == EXTR_FUNCTION) {
			if (p->classifBits == EXTRACT_IN_OUT_ARGUMENT) {
				if (op == NULL) op = p;
				else uniqueOutFlag = 0;
				// re-classify to in_out
			}
		}
	}

	if (op!=NULL && uniqueOutFlag) {
		op->classifBits = EXTRACT_IN_RESULT_VALUE;
		return;
	}

}

/* ************************** macro ******************************* */

static void extGenNewMacroCall(S_programGraphNode *program) {
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[TMP_STRING_SIZE];
	char 				name[TMP_STRING_SIZE];
	S_programGraphNode 	*p;
	char				*declAddr;
	int 				i, fFlag=1;

	rb[0]=0;

	sprintf(rb+strlen(rb),"\t%s",s_extractionName);

	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_VALUE_ARGUMENT
			||	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_ADDRESS_ARGUMENT
			||	p->classifBits == EXTRACT_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_RESULT_VALUE
			||	p->classifBits == EXTRACT_IN_RESULT_VALUE
			||	p->classifBits == EXTRACT_LOCAL_VAR) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
			fFlag = 0;
		}
	}
	sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");

	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

static void extGenNewMacroHead(S_programGraphNode *program) {
	char				nhead[MAX_EXTRACT_FUN_HEAD_SIZE];
	int					nhi;
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[MAX_EXTRACT_FUN_HEAD_SIZE];
	char 				name[MAX_EXTRACT_FUN_HEAD_SIZE];
	S_programGraphNode 	*p,*op;
	char				*declAddr;
	int 				i, uniqueOutFlag, fFlag=1;

	rb[0]=0;

	sprintf(rb+strlen(rb),"#define %s",s_extractionName);
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_VALUE_ARGUMENT
			||	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_ADDRESS_ARGUMENT
			||	p->classifBits == EXTRACT_RESULT_VALUE
			||	p->classifBits == EXTRACT_IN_RESULT_VALUE
			||	p->classifBits == EXTRACT_LOCAL_VAR) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "%s%s", fFlag?"(":"," , name);
			fFlag = 0;
		}
	}
	sprintf(rb+strlen(rb), "%s) {\\\n", fFlag?"(":"");
	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

static void extGenNewMacroTail(S_programGraphNode *program) {
	rb[0]=0;

	sprintf(rb+strlen(rb),"}\n\n");

	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}



/* ********************** C function **************************** */


static void extGenNewFunCall(S_programGraphNode *program) {
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[TMP_STRING_SIZE];
	char 				name[TMP_STRING_SIZE];
	S_programGraphNode 	*p;
	char				*declAddr;
	int 				i, fFlag=1;

	rb[0]=0;

	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_RESULT_VALUE
			|| p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE
			) break;
	}
	if (p!=NULL) {
		GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
		if (p->classifBits == EXTRACT_LOCAL_RESULT_VALUE) {
			sprintf(rb+strlen(rb),"\t%s = ", decl);
		} else {
			sprintf(rb+strlen(rb),"\t%s = ", name);
		}
	} else {
		sprintf(rb+strlen(rb),"\t");
	}
	sprintf(rb+strlen(rb),"%s",s_extractionName);

	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_VALUE_ARGUMENT
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
			fFlag = 0;
		}
	}
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_ADDRESS_ARGUMENT
			||	p->classifBits == EXTRACT_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  s_opt.olExtractAddrParPrefix,1);
			sprintf(rb+strlen(rb), "%s&%s", fFlag?"(":", " , name);
			fFlag = 0;
		}
	}
	sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");
	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

static void removeSymbolFromSymRefList(S_symbolRefItemList **ll, S_symbolRefItem *s) {
	S_symbolRefItemList **r;
	r=ll; 
	while (*r!=NULL) {
		if (isSmallerOrEqClass(getClassNumFromClassLinkName((*r)->d->name, s_noneFileIndex), 
							   getClassNumFromClassLinkName(s->name, s_noneFileIndex))) {
			*r = (*r)->next;
		} else {
			r= &(*r)->next;
		}
	}
}

static void addSymbolToSymRefList(S_symbolRefItemList **ll, S_symbolRefItem *s) {
	S_symbolRefItemList *r, *rr;
	r = *ll; 
	while (r!=NULL) {
		if (isSmallerOrEqClass(getClassNumFromClassLinkName(s->name, s_noneFileIndex), 
							   getClassNumFromClassLinkName(r->d->name, s_noneFileIndex))) {
			return;
		}
		r= r->next;
	}
	// first remove all superceeded by this one
	removeSymbolFromSymRefList(ll, s);
	CX_ALLOC(rr, S_symbolRefItemList);
	FILL_symbolRefItemList(rr, s, *ll);
	*ll = rr;
}

static S_symbolRefItemList *computeExceptionsThrownBetween(S_programGraphNode *bb,
														   S_programGraphNode *ee
	) {
	S_symbolRefItemList	*res, *excs, *catched, *noncatched, **cl;
	S_programGraphNode 	*p, *b, *e;
	int 				deep;
	catched = NULL; noncatched = NULL; cl = &catched;
	for(p=bb; p!=NULL && p!=ee; p=p->next) {
		if (p->symRef->b.symType == TypeTryCatchMarker && p->ref->usg.base == UsageTryCatchBegin) {
			deep = 0;
			for(e=p; e!=NULL && e!=ee; e=e->next) {
				if (e->symRef->b.symType == TypeTryCatchMarker) {
					if (e->ref->usg.base == UsageTryCatchBegin) deep++;
					else deep --;
					if (deep == 0) break;
				}
			}
			if (deep==0) {
				// add exceptions thrown in block
				for(excs=computeExceptionsThrownBetween(p->next,e); excs!=NULL; excs=excs->next){
					addSymbolToSymRefList(cl, excs->d);
				}
			}
		}
		if (p->ref->usg.base == UsageThrown) {
			// thrown exception add it to list
			addSymbolToSymRefList(cl, p->symRef);
		} else if (p->ref->usg.base == UsageCatched) {
			// catched, remove it from list
			removeSymbolFromSymRefList(&catched, p->symRef);
			cl = &noncatched;
		}
	}
	res = catched;
	for(excs=noncatched; excs!=NULL; excs=excs->next) {
		addSymbolToSymRefList(&res, excs->d);
	}
	return(res);
}

static S_symbolRefItemList *computeExceptionsThrownInBlock(S_programGraphNode *program) {
	S_programGraphNode 	*pp, *ee;
	for(pp=program; pp!=NULL && pp->symRef->b.symType!=TypeBlockMarker; pp=pp->next) ;
	if (pp==NULL) return(NULL);
	for(ee=pp->next; ee!=NULL && ee->symRef->b.symType!=TypeBlockMarker; ee=ee->next) ;
	return(computeExceptionsThrownBetween(pp, ee));
}

static void extractSprintThrownExceptions(char *nhead, S_programGraphNode *program) {
	S_symbolRefItemList 	*exceptions, *ee;
	int						nhi;
	char					*sname;
	nhi = 0;
	exceptions = computeExceptionsThrownInBlock(program);
	if (exceptions!=NULL) {
		sprintf(nhead+nhi, " throws");
		nhi += strlen(nhead+nhi);
		for(ee=exceptions; ee!=NULL; ee=ee->next) {
			sname = lastOccurenceInString(ee->d->name, '$');
			if (sname==NULL) sname = lastOccurenceInString(ee->d->name, '/');
			if (sname==NULL) {
				sname = ee->d->name;
			} else {
				sname ++;
			}
			sprintf(nhead+nhi, "%s%s", ee==exceptions?" ":", ", sname);
			nhi += strlen(nhead+nhi);
		}
	}
}

static void extGenNewFunHead(S_programGraphNode *program) {
	char				nhead[MAX_EXTRACT_FUN_HEAD_SIZE];
	int					nhi,ldclaLen;
	char 				ldcla[TMP_STRING_SIZE];
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[MAX_EXTRACT_FUN_HEAD_SIZE];
	char 				name[MAX_EXTRACT_FUN_HEAD_SIZE];
	S_programGraphNode 	*p,*op;
	char				*declAddr;
	int 				i, uniqueOutFlag, fFlag=1;
	S_symbolRefItemList	*exceptions, *ee;


	rb[0]=0;

	/* function header */

	nhi = 0;
	sprintf(nhead+nhi,"%s",s_extractionName);
	nhi += strlen(nhead+nhi);
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_VALUE_ARGUMENT
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
			nhi += strlen(nhead+nhi);
			fFlag = 0;
		}
	}
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
				||	p->classifBits == EXTRACT_OUT_ARGUMENT
				||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  s_opt.olExtractAddrParPrefix,1);
			sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
			nhi += strlen(nhead+nhi);
			fFlag = 0;
		}
	}
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_ADDRESS_ARGUMENT) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  EXTRACT_REFERENCE_ARG_STRING, 1);
			sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
			nhi += strlen(nhead+nhi);
			fFlag = 0;
		}
	}
	sprintf(nhead+nhi, "%s)", fFlag?"(":"");
	nhi += strlen(nhead+nhi);

	//throws
	extractSprintThrownExceptions(nhead+nhi, program);
	nhi += strlen(nhead+nhi);

	if (LANGUAGE(LAN_JAVA)) {
		// this makes renaming after extraction much faster
		sprintf(rb+strlen(rb), "private ");
	}

	if (LANGUAGE(LAN_JAVA) && (s_javaExtractFromFunctionMods&ACC_STATIC)==0){
		sprintf(rb+strlen(rb), "");
	} else {
		sprintf(rb+strlen(rb), "static ");
	}
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_RESULT_VALUE
			|| p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE) break;
	}
	if (p==NULL) {
		sprintf(rb+strlen(rb),"void %s",nhead);
	} else {
		GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,nhead,0);
		sprintf(rb+strlen(rb), "%s", decl);
	}

	/* function body */

	sprintf(rb+strlen(rb), " {\n");
	ldcla[0] = 0; ldclaLen = 0; fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (		p->classifBits == EXTRACT_IN_OUT_ARGUMENT
					||	p->classifBits == EXTRACT_LOCAL_VAR
					||	p->classifBits == EXTRACT_OUT_ARGUMENT
					||  p->classifBits == EXTRACT_RESULT_VALUE
					||	(	p->symRef->b.storage == StorageExtern
							&& p->ref->usg.base == UsageDeclared)
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			if (strcmp(ldcla,dcla)==0) {
				sprintf(rb+strlen(rb), ",%s",decl+ldclaLen);
			} else {
				strcpy(ldcla,dcla); ldclaLen=strlen(ldcla);
				if (fFlag) sprintf(rb+strlen(rb), "\t%s",decl);
				else sprintf(rb+strlen(rb), ";\n\t%s",decl);
			}
			if (p->classifBits == EXTRACT_IN_OUT_ARGUMENT) {
				sprintf(rb+strlen(rb), " = %s%s", s_opt.olExtractAddrParPrefix, name);
			}
			fFlag = 0;
		}
	}
	if (fFlag == 0) sprintf(rb+strlen(rb), ";\n");
	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

static void extGenNewFunTail(S_programGraphNode *program) {
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[TMP_STRING_SIZE];
	char 				name[TMP_STRING_SIZE];
	S_programGraphNode	*p;

	rb[0]=0;

	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "\t%s%s = %s;\n", s_opt.olExtractAddrParPrefix,
					name, name);
		}
	}
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_RESULT_VALUE
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE
			|| p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "\treturn(%s);\n", name);
		}
	}
	sprintf(rb+strlen(rb),"}\n\n");
	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}


/* ********************** java function **************************** */


static char * extJavaNewClassName() {
  static char classname[TMP_STRING_SIZE];
  if (s_extractionName[0]==0) {
	sprintf(classname,"NewClass");
  } else {
	sprintf(classname, "%c%s", toupper(s_extractionName[0]),
			s_extractionName+1);
  }
  return(classname);
}


static void extJavaGenNewClassCall(S_programGraphNode *program) {
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[TMP_STRING_SIZE];
	char 				name[TMP_STRING_SIZE];
	S_programGraphNode 	*p;
	char				*declAddr,*classname;
	int 				i, fFlag=1;

	rb[0]=0;

	classname = extJavaNewClassName();

#if ZERO
	// declarations of local out variables
	fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
	  if (p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
		GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
		if (fFlag) sprintf(rb+strlen(rb), "\t\t");
		sprintf(rb+strlen(rb), "%s; ", decl);
		fFlag = 0;
	  }
	}
	sprintf(rb+strlen(rb), "\n");
#endif

	// contructor invocation
	sprintf(rb+strlen(rb),"\t\t%s %s = new %s",classname, s_extractionName,classname);
	fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
	  if (p->classifBits == EXTRACT_IN_OUT_ARGUMENT) {
		GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
		sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
		fFlag = 0;
	  }
	}
	sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");

	// "perform" invocation
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_RESULT_VALUE
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE
			|| p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
			) break;
	}
	if (p!=NULL) {
		GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
		if (p->classifBits == EXTRACT_LOCAL_RESULT_VALUE) {
			sprintf(rb+strlen(rb),"\t\t%s = ", decl);
		} else {
			sprintf(rb+strlen(rb),"\t\t%s = ", name);
		}
	} else {
		sprintf(rb+strlen(rb),"\t\t");
	}
	sprintf(rb+strlen(rb),"%s.perform",s_extractionName);

	fFlag=1;
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_VALUE_ARGUMENT
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
			fFlag = 0;
		}
	}
	sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");

	sprintf(rb+strlen(rb), "\t\t");
	// 'out' arguments value recovering
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_OUT_ARGUMENT
			||	p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  "",1);
			if (p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
				sprintf(rb+strlen(rb), "%s=%s.%s; ", decl, s_extractionName, name);
			} else {
				sprintf(rb+strlen(rb), "%s=%s.%s; ", name, s_extractionName, name);
			}
			fFlag = 0;
		}
	}
	sprintf(rb+strlen(rb), "\n");
	sprintf(rb+strlen(rb),"\t\t%s = null;\n", s_extractionName);

	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

static void extJavaGenNewClassHead(S_programGraphNode *program) {
	char				nhead[MAX_EXTRACT_FUN_HEAD_SIZE];
	int					nhi,ldclaLen;
	char 				ldcla[TMP_STRING_SIZE];
	char 				dcla[TMP_STRING_SIZE];
	char 				*classname;
	char 				decl[MAX_EXTRACT_FUN_HEAD_SIZE];
	char 				name[MAX_EXTRACT_FUN_HEAD_SIZE];
	S_programGraphNode 	*p,*op;
	char				*declAddr, *mattrib;
	int 				i, uniqueOutFlag, fFlag=1;

	rb[0]=0;

	classname = extJavaNewClassName();

	// class header
	sprintf(rb+strlen(rb), "\t");
	if (s_javaExtractFromFunctionMods & ACC_STATIC){
	  sprintf(rb+strlen(rb), "static ");
	}
	sprintf(rb+strlen(rb), "class %s {\n", classname);
	//sprintf(rb+strlen(rb), "\t\t// %s 'out' arguments\n", s_opt.extractName);
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_OUT_ARGUMENT
			|| p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			|| p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  "",1);
			sprintf(rb+strlen(rb), "\t\t%s;\n", decl);
		}
	}

	// the constructor
	//sprintf(rb+strlen(rb),"\t\t// constructor for %s 'in-out' args\n",s_opt.extractName);
	sprintf(rb+strlen(rb), "\t\t%s", classname);
	fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_IN_OUT_ARGUMENT){
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  "",1);
			sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , decl);
			fFlag = 0;
		}
	}
	sprintf(rb+strlen(rb), "%s) {", fFlag?"(":"");
	fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_IN_OUT_ARGUMENT){
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  "",1);
			if (fFlag) sprintf(rb+strlen(rb), "\n\t\t\t");
			sprintf(rb+strlen(rb), "this.%s = %s; ", name, name);
			fFlag = 0;
		}
	}
	if (! fFlag) sprintf(rb+strlen(rb),"\n\t\t");
	sprintf(rb+strlen(rb),"}\n");
	//sprintf(rb+strlen(rb),"\t\t// perform with %s 'in' args\n",s_opt.extractName);

	// the "perform" method
	nhi = 0; fFlag = 1;
	sprintf(nhead+nhi,"%s", "perform");
	nhi += strlen(nhead+nhi);
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_VALUE_ARGUMENT
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  "", 1);
			sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
			nhi += strlen(nhead+nhi);
			fFlag = 0;
		}
	}
	sprintf(nhead+nhi, "%s)", fFlag?"(":"");
	nhi += strlen(nhead+nhi);

	// throws
	extractSprintThrownExceptions(nhead+nhi, program);
	nhi += strlen(nhead+nhi);

	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_RESULT_VALUE
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE) break;
	}
	if (p==NULL) {
		sprintf(rb+strlen(rb),"\t\tvoid %s", nhead);
	} else {
		GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,nhead,0);
		sprintf(rb+strlen(rb), "\t\t%s", decl);
	}

	/* function body */

	sprintf(rb+strlen(rb), " {\n");
	ldcla[0] = 0; ldclaLen = 0; fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (	p->classifBits == EXTRACT_LOCAL_VAR
				||  p->classifBits == EXTRACT_RESULT_VALUE
				||	(	p->symRef->b.storage == StorageExtern
					 && p->ref->usg.base == UsageDeclared)
					) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			if (strcmp(ldcla,dcla)==0) {
				sprintf(rb+strlen(rb), ",%s",decl+ldclaLen);
			} else {
				strcpy(ldcla,dcla); ldclaLen=strlen(ldcla);
				if (fFlag) sprintf(rb+strlen(rb), "\t\t\t%s",decl);
				else sprintf(rb+strlen(rb), ";\n\t\t\t%s",decl);
			}
			fFlag = 0;
		}
	}
	if (fFlag == 0) sprintf(rb+strlen(rb), ";\n");
	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

static void extJavaGenNewClassTail(S_programGraphNode *program) {
	char 				dcla[TMP_STRING_SIZE];
	char 				decl[TMP_STRING_SIZE];
	char 				name[TMP_STRING_SIZE];
	S_programGraphNode	*p;
	int					fFlag;

	rb[0]=0;


	// local 'out' arguments value setting
	fFlag = 1;
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
										  "",1);
			if (fFlag) sprintf(rb+strlen(rb), "\t\t\t");
			sprintf(rb+strlen(rb), "this.%s=%s; ", name, name);
			fFlag = 0;
		}
	}
	if (! fFlag) sprintf(rb+strlen(rb), "\n");


	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_RESULT_VALUE
			|| p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
			|| p->classifBits == EXTRACT_IN_RESULT_VALUE
			) {
			GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
			sprintf(rb+strlen(rb), "\t\t\treturn(%s);\n", name);
		}
	}
	sprintf(rb+strlen(rb),"\t\t}\n\t}\n\n");

	InternalCheck(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
	if (s_opt.xref2) {
		ppcGenRecord(PPC_STRING_VALUE, rb, "\n");
	} else {
		fprintf(ccOut, "%s", rb);
	}
}

/* ******************************************************************* */

static int extJavaIsNewClassNecesary(S_programGraphNode *program) {
	S_programGraphNode 	*p;
	for(p=program; p!=NULL; p=p->next) {
		if (p->classifBits == EXTRACT_OUT_ARGUMENT
			|| p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
			|| p->classifBits == EXTRACT_IN_OUT_ARGUMENT
			) break;
	}
	if (p==NULL) return(0);
	return(1);
}

static void extMakeExtraction() {
	S_programGraphNode *program,*p;
	int cat,newClassExt;
	if (s_cp.cxMemiAtFunBegin > s_cps.cxMemiAtBlockBegin
		|| s_cps.cxMemiAtBlockBegin > s_cps.cxMemiAtBlockEnd
		|| s_cps.cxMemiAtBlockEnd > s_cp.cxMemiAtFunEnd
		|| s_cps.workMemiAtBlockBegin != s_cps.workMemiAtBlockEnd) {
		error(ERR_ST, "Region / program structure mismatch");
		return;
	}
//&fprintf(dumpOut,"!cxMemories: funBeg, blockBeb, blockEnd, funEnd: %x, %x, %x, %x\n", s_cp.cxMemiAtFunBegin, s_cps.cxMemiAtBlockBegin, s_cps.cxMemiAtBlockEnd, s_cp.cxMemiAtFunEnd);
	assert(s_cp.cxMemiAtFunBegin);
	assert(s_cps.cxMemiAtBlockBegin);
	assert(s_cps.cxMemiAtBlockEnd);
	assert(s_cp.cxMemiAtFunEnd);

	program = extMakeProgramGraph();
	extSetInOutBlockFields(program);
//&dumpProgram(program);

	if (s_opt.extractMode!=EXTR_MACRO && extIsJumpInOutBlock(program)) {
		error(ERR_ST, "There are jumps in or out of region");
		return;
	}
	extClassifyLocalVariables(program);
	extReClassifyIOVars(program);

	newClassExt = 0;
	if (LANGUAGE(LAN_JAVA)) newClassExt =  extJavaIsNewClassNecesary(program);

	if (LANGUAGE(LAN_JAVA)) {
		if (newClassExt) s_extractionName = "newClass_";
		else s_extractionName = "newMethod_";
	} else {
		if (s_opt.extractMode==EXTR_MACRO) s_extractionName = "NEW_MACRO_";
		else s_extractionName = "newFunction_";
	}

	if (s_opt.xref2) ppcGenRecordWithAttributeBegin(PPC_EXTRACTION_DIALOG, PPCA_TYPE, s_extractionName);
	CX_ALLOCC(rb, EXTRACT_GEN_BUFFER_SIZE, char);

	if (! s_opt.xref2) {
		fprintf(ccOut,
"%%!\n------------------------ The Invocation ------------------------\n!\n");
	}
	if (s_opt.extractMode==EXTR_MACRO) extGenNewMacroCall(program);
	else if (newClassExt) extJavaGenNewClassCall(program);
	else extGenNewFunCall(program);
	if (! s_opt.xref2) {
		fprintf(ccOut,
"!\n--------------------------- The Head ---------------------------\n!\n");
	}
	if (s_opt.extractMode==EXTR_MACRO) extGenNewMacroHead(program);
	else if (newClassExt) extJavaGenNewClassHead(program);
	else extGenNewFunHead(program);
	if (! s_opt.xref2) {
		fprintf(ccOut,
"!\n--------------------------- The Tail ---------------------------\n!\n");
	}
	if (s_opt.extractMode==EXTR_MACRO) extGenNewMacroTail(program);
	else if (newClassExt) extJavaGenNewClassTail(program);
	else extGenNewFunTail(program);

	if (s_opt.xref2) {
		ppcGenNumericRecord(PPC_INT_VALUE, s_cp.funBegPosition, "", "\n");
	} else {
		fprintf(ccOut,"!%d!\n", s_cp.funBegPosition);
		fflush(ccOut);
	}

	if (s_opt.xref2) ppcGenRecordEnd(PPC_EXTRACTION_DIALOG);
}


void actionsBeforeAfterExternalDefinition() {
	int pi;
	if (s_cps.cxMemiAtBlockEnd != 0
		// you have to check for matching class method
		// i.e. for case 'void mmm() { //blockbeg; ...; //blockend; class X { mmm(){}!!}; }'
		&& s_cp.cxMemiAtFunBegin != 0
		&& s_cp.cxMemiAtFunBegin <= s_cps.cxMemiAtBlockBegin
		// is it an extraction action ?
		&& s_opt.cxrefs == OLO_EXTRACT
		&& (! s_cps.extractProcessedFlag)) {
			// O.K. make extraction
			s_cp.cxMemiAtFunEnd = cxMemory->i;
			extMakeExtraction();
			s_cps.extractProcessedFlag = 1;
		/* here should be a longjmp to stop file processing !!!! */
		/* No, all this extraction should be after parsing ! */
	}
	s_cp.cxMemiAtFunBegin = cxMemory->i;
	if (inStacki) {						// ??????? burk ????
		s_cp.funBegPosition = inStack[0].lineNumber+1;
	} else {
		s_cp.funBegPosition = cFile.lineNumber+1;
	}
}


void extractActionOnBlockMarker() {
	S_position pos;
	if (s_cps.cxMemiAtBlockBegin == 0) {
		s_cps.cxMemiAtBlockBegin = cxMemory->i;
		s_cps.workMemiAtBlockBegin = s_topBlock->previousTopBlock;
		if (LANGUAGE(LAN_JAVA)) {
			s_javaExtractFromFunctionMods = s_javaStat->cpMethodMods;
		}
	} else {
		assert(s_cps.cxMemiAtBlockEnd == 0);
		s_cps.cxMemiAtBlockEnd = cxMemory->i;
		s_cps.workMemiAtBlockEnd = s_topBlock->previousTopBlock;
	}
	FILL_position(&pos, cFile.lb.cb.fileNumber, 0, 0);
	addTrivialCxReference("Block", TypeBlockMarker,StorageDefault, &pos, UsageUsed);
}

/*
	$Revision: 1.21 $
	$Date: 2002/08/27 23:08:25 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/

#include "unigram.h"
#include "protocol.h"
//

#define FULL_COMPLETION_INTEND_CHARS	2

static void formatFullCompletions(char *tt, int indent, int inipos) {
	int 	pos;
	char	*nlpos,*p;
//&sprintf(tmpBuff,"formatting '\%s' indent==%d, inipos==%d, linelen==%d",tt,indent,inipos,s_opt.olineLen);ppcGenTmpBuff();
	pos = inipos; nlpos=NULL;
	p = tt;
	for(;;) {
		while (pos<s_opt.olineLen || nlpos==NULL) {
			p++; pos++;
			if (*p == 0) goto formatFullCompletionsEnd;
			if (*p ==' ' && pos>indent) nlpos = p;
		}
		*nlpos = '\n'; pos = indent+(p-nlpos); nlpos=NULL;
	}
 formatFullCompletionsEnd:
	return;
//&sprintf(tmpBuff,"result '\%s'",tt);ppcGenTmpBuff();	
}

void formatOutputLine(char *tt, int startingColumn) {
	int 	pos, n;
	char	*nlp,*p;
	pos = startingColumn; nlp=NULL;
	assert(s_opt.tabulator>1);
	p = tt;
	for(;;) {
		while (pos<s_opt.olineLen || nlp==NULL) {
			if (*p == 0) return;
			if (*p == ' ') nlp = p;
			if (*p == '\t') {
				nlp = p;
				n = s_opt.tabulator-(pos-1)%s_opt.tabulator-1;
				pos += n;
			} else {
				pos++;
			}
			p++;
		}
		*nlp = '\n'; p = nlp+1; nlp=NULL; pos = 0;
	}
}

static int printJavaModifiers(char *buf, int *size, unsigned acc) {
	int i;
	i = 0;
	if (1 || (s_opt.ooChecksBits & OOC_ACCESS_CHECK)==0) {
		if (acc & ACC_PUBLIC) {
			sprintf(buf+i,"public "); i+=strlen(buf+i);
			InternalCheck(i< *size);
		}
		if (acc & ACC_PRIVATE) {
			sprintf(buf+i,"private "); i+=strlen(buf+i);
			InternalCheck(i< *size);
		}
		if (acc & ACC_PROTECTED) {
			sprintf(buf+i,"protected "); i+=strlen(buf+i);
			InternalCheck(i< *size);
		}
#if ZERO
		if (acc & ACC_NATIVE) {
			sprintf(buf+i,"native "); i+=strlen(buf+i);
			InternalCheck(i< *size);
		}
#endif
	}
	if (acc & ACC_STATIC) {
		sprintf(buf+i,"static "); i+=strlen(buf+i);
		InternalCheck(i< *size);
	}
	if (acc & ACC_FINAL) {
		sprintf(buf+i,"final "); i+=strlen(buf+i);
		InternalCheck(i< *size);
	}
	*size -= i;
	return(i);
}

static char *getCompletionClassFieldString( S_cline *cl) {
	char *cname;
//&	if (cl->t->b.accessFlags & ACC_STATIC) {
// statics, get class from link name
//&		cname = javaGetNudePreTypeName_st(cl->t->linkName,CUT_OUTERS);
//&				sprintf(tmpBuff,"%s", cname);
//&} else 
	if (cl->vFunClass!=NULL) {
		cname = javaGetShortClassName(cl->vFunClass->linkName);
	} else {
		assert(cl->t);
		cname = javaGetNudePreTypeName_st(cl->t->linkName,CUT_OUTERS);
//&				sprintf(tmpBuff,"%s", cname);
	}
	return(cname);
}


static void sprintFullCompletionInfo(S_completions* c, int ii, int indent) {
	int size,ll,i,tdexpFlag,vFunCl,cindent, ttlen;
	char tt[COMPLETION_STRING_SIZE];
	char ttt[TMP_STRING_SIZE];
	char *pname,*cname;
	char *ppc;
	ppc = ppcTmpBuff;
	if (c->a[ii].symType == TypeUndefMacro) return;
	// remove parenthesis (if any)
	strcpy(tt, c->a[ii].s);
	ttlen = strlen(tt);
	if (ttlen>0 && tt[ttlen-1]==')' && c->a[ii].symType!=TypeInheritedFullMethod) {
		ttlen--;
		tt[ttlen]=0;
	}
	if (ttlen>0 && tt[ttlen-1]=='(') {
		ttlen--;
		tt[ttlen]=0;
	}
	sprintf(ppc, "%-*s:", indent+FULL_COMPLETION_INTEND_CHARS, tt);
	cindent = strlen(ppc);
	ppc += strlen(ppc);
	vFunCl = s_noneFileIndex;
	size = COMPLETION_STRING_SIZE; 
	ll = 0;
	if (c->a[ii].symType==TypeDefault) {
		assert(c->a[ii].t && c->a[ii].t->u.type);
		if (LANGUAGE(LAN_JAVA)) {
			cname = getCompletionClassFieldString(&c->a[ii]);
			if (c->a[ii].virtLevel>NEST_VIRT_COMPL_OFFSET) {
				sprintf(tmpBuff,"(%d.%d)%s: ", 
						c->a[ii].virtLevel/NEST_VIRT_COMPL_OFFSET, 
						c->a[ii].virtLevel%NEST_VIRT_COMPL_OFFSET,
						cname);
			} else if (c->a[ii].virtLevel>0) {
				sprintf(tmpBuff,"(%d)%s: ", c->a[ii].virtLevel, cname);
			} else {
				sprintf(tmpBuff,"   : ");
			}
			cindent += strlen(tmpBuff);
			sprintf(ppc,"%s", tmpBuff);
			ppc += strlen(ppc);
		}
		tdexpFlag = 1;
		if (c->a[ii].t->b.storage == StorageTypedef) {
			sprintf(tt,"typedef ");
			ll = strlen(tt);
			size -= ll;
			tdexpFlag = 0;
		}
		/*		if (c->a[ii].t->u.type->m == TypeFunction) {*/
//		if (LANGUAGE(LAN_JAVA) && c->a[ii].t->b.storage != StorageAuto) {
//			pname = c->a[ii].t->linkName;
//		} else {
		pname = c->a[ii].t->name;
//		}
		if (LANGUAGE(LAN_JAVA)) {
			ll += printJavaModifiers(tt+ll, &size, c->a[ii].t->b.accessFlags);
			if (c->a[ii].vFunClass!=NULL) {
				vFunCl = c->a[ii].vFunClass->u.s->classFile;
				if (vFunCl == -1) vFunCl = s_noneFileIndex;
			}
		}
		typeSPrint(tt+ll, &size, c->a[ii].t->u.type, pname,' ', 0, tdexpFlag,SHORT_NAME, NULL);
		if (LANGUAGE(LAN_JAVA) 
			&& (c->a[ii].t->b.storage == StorageMethod 
				|| c->a[ii].t->b.storage == StorageConstructor)) {
			throwsSprintf(tt+ll+size, COMPLETION_STRING_SIZE-ll-size, c->a[ii].t->u.type->u.m.exceptions);
		}
	} else if (c->a[ii].symType==TypeMacro) {
		macDefSPrintf(tt, &size, "", c->a[ii].s,
					  c->a[ii].margn, c->a[ii].margs, NULL);
	} else if (LANGUAGE(LAN_JAVA) && c->a[ii].symType==TypeStruct ) {
		if (c->a[ii].t!=NULL) {
			ll += printJavaModifiers(tt+ll, &size, c->a[ii].t->b.accessFlags);			
			if (c->a[ii].t->b.accessFlags & ACC_INTERFACE) {
				sprintf(tt+ll,"interface ");
			} else {
				sprintf(tt+ll,"class ");
			}
			ll = strlen(tt);
			javaTypeStringSPrint(tt+ll, c->a[ii].t->linkName,LONG_NAME, NULL);
		} else {
			sprintf(tt,"class ");
		}
	} else if (c->a[ii].symType == TypeInheritedFullMethod) {
		if (c->a[ii].vFunClass!=NULL) {
			sprintf(tt,"%s \t:%s", c->a[ii].vFunClass->name, typesName[c->a[ii].symType]);
			vFunCl = c->a[ii].vFunClass->u.s->classFile;
			if (vFunCl == -1) vFunCl = s_noneFileIndex;
		} else {
			sprintf(tt,"%s", typesName[c->a[ii].symType]);
		}
	} else {
		assert(c->a[ii].symType>=0 && c->a[ii].symType<MAX_TYPE);
		sprintf(tt,"%s", typesName[c->a[ii].symType]);
	}
	formatFullCompletions(tt, indent+FULL_COMPLETION_INTEND_CHARS+2, cindent);
	for(i=0; tt[i]; i++) {
		sprintf(ppc,"%c",tt[i]);
		ppc += strlen(ppc);
		if (tt[i] == '\n') {
			sprintf(ppc,"%-*s ",indent+FULL_COMPLETION_INTEND_CHARS, " ");
			ppc += strlen(ppc);
		}
	}
}

static void sprintFullJeditCompletionInfo(S_completions* c, int ii, int *nindent, char **vclass) {
	int size,ll,i,tdexpFlag,cindent;
	char *pname,*cname;
	static char vlevelBuff[TMP_STRING_SIZE];
	size = COMPLETION_STRING_SIZE; 
	ll = 0;
	sprintf(vlevelBuff," ");
	if (vclass != NULL) *vclass = vlevelBuff;
	if (c->a[ii].symType==TypeDefault) {
		assert(c->a[ii].t && c->a[ii].t->u.type);
		if (LANGUAGE(LAN_JAVA)) {
			cname = getCompletionClassFieldString(&c->a[ii]);
			if (c->a[ii].virtLevel>NEST_VIRT_COMPL_OFFSET) {
				sprintf(vlevelBuff,"  : (%d.%d) %s ", 
						c->a[ii].virtLevel/NEST_VIRT_COMPL_OFFSET, 
						c->a[ii].virtLevel%NEST_VIRT_COMPL_OFFSET,
						cname);
			} else if (c->a[ii].virtLevel>0) {
				sprintf(vlevelBuff,"  : (%d) %s ", c->a[ii].virtLevel, cname);
			}
		}
		tdexpFlag = 1;
		if (c->a[ii].t->b.storage == StorageTypedef) {
			sprintf(ppcTmpBuff,"typedef ");
			ll = strlen(ppcTmpBuff);
			size -= ll;
			tdexpFlag = 0;
		}
		pname = c->a[ii].t->name;
		if (LANGUAGE(LAN_JAVA)) {
			ll += printJavaModifiers(ppcTmpBuff+ll, &size, c->a[ii].t->b.accessFlags);
		}
		typeSPrint(ppcTmpBuff+ll, &size, c->a[ii].t->u.type, pname,' ', 0, tdexpFlag,SHORT_NAME, nindent);
		*nindent += ll;
		if (LANGUAGE(LAN_JAVA) 
			&& (c->a[ii].t->b.storage == StorageMethod 
				|| c->a[ii].t->b.storage == StorageConstructor)) {
			throwsSprintf(ppcTmpBuff+ll+size, COMPLETION_STRING_SIZE-ll-size, c->a[ii].t->u.type->u.m.exceptions);
		}
	} else if (c->a[ii].symType==TypeMacro) {
		macDefSPrintf(ppcTmpBuff, &size, "", c->a[ii].s,
					  c->a[ii].margn, c->a[ii].margs, nindent);
	} else if (LANGUAGE(LAN_JAVA) && c->a[ii].symType==TypeStruct ) {
		if (c->a[ii].t!=NULL) {
			ll += printJavaModifiers(ppcTmpBuff+ll, &size, c->a[ii].t->b.accessFlags);			
			if (c->a[ii].t->b.accessFlags & ACC_INTERFACE) {
				sprintf(ppcTmpBuff+ll,"interface ");
			} else {
				sprintf(ppcTmpBuff+ll,"class ");
			}
			ll = strlen(ppcTmpBuff);
			javaTypeStringSPrint(ppcTmpBuff+ll, c->a[ii].t->linkName, LONG_NAME, nindent);
			*nindent += ll;
		} else {
			sprintf(ppcTmpBuff,"class ");
			ll = strlen(ppcTmpBuff);
			*nindent = ll;
			sprintf(ppcTmpBuff+ll,"%s", c->a[ii].s);
		}
	} else if (c->a[ii].symType == TypeInheritedFullMethod) {
		sprintf(ppcTmpBuff,"%s", c->a[ii].s);
		if (c->a[ii].vFunClass!=NULL) {
			sprintf(vlevelBuff,"  : %s ", c->a[ii].vFunClass->name);
		}
		*nindent = 0;
	} else {
		sprintf(ppcTmpBuff,"%s", c->a[ii].s);
		*nindent = 0;
	}
}

void olCompletionListInit(S_position *originalPos) {
	olcxSetCurrentUser(s_opt.user);
	olcxFreeOldCompletionItems(&s_olcxCurrentUser->completionsStack);
	olcxPushEmptyStackItem(&s_olcxCurrentUser->completionsStack);
	s_olcxCurrentUser->completionsStack.top->cpos = *originalPos;
}

#if ZERO
// if it return 1 - so, the original order is prefered
static int reorderCmpCompletions(S_cline *c1, S_cline *c2) {
	int c;
	char *n1,*n2;
	if (c1->symType==TypeKeyword) return(1);
	c = c1->virtLevel - c2->virtLevel;
	if (c<0) return(-1);
	if (c>0) return(1);
	if (c1->symType!=c2->symType) return(1); // uncomparable types
	if (c1->symType!=TypeDefault) return(1);
	if (c2->vFunClass==NULL) return(1);
	if (c1->vFunClass==NULL) return(-1);
	n1 = javaGetShortClassName(c1->vFunClass->linkName);
	n2 = javaGetShortClassName(c2->vFunClass->linkName);
	c = strcmp(n1, n2);
	if (c<0) return(-1);
	if (c>0) return(1);
	return(1);	// by default, keep original order
}

static void reorderCompletionArray(S_completions* cc) {
	S_cline *a,tl;
	int ai;
	int l,r,x,c,i;
	a = cc->a;
	for (ai=0; ai<cc->ai; ai++) {
		tl = a[ai];
		l = 0; r = ai-1;
		while (l<=r) {
			x = (l+r)/2;
			c = reorderCmpCompletions(&tl, &a[x]);
			if (c<0) r=x-1; else l=x+1;
		}
		assert(l==r+1);
		for(i=ai-1; i>=l; i--) a[i+1] = a[i];
		a[l] = tl;
	}
}
#endif

#define ALL_COMPLETIONS_IN_ONE 1

static int completionsWillPrintEllipsis(S_olCompletion *olc) {
	int max, ellipsis;
	LIST_LEN(max, S_olCompletion, olc);
	ellipsis = 0;
	if (max >= MAX_COMPLETIONS - 2 || max == s_opt.maxCompletions) {
		ellipsis = 1;
	}
	return(ellipsis);
}

void printCompletionsBeginning(S_olCompletion *olc, int noFocus) {
	int 				max;
	S_olCompletion 		*cc;
	int					tlen;
	LIST_LEN(max, S_olCompletion, olc);
	if (s_opt.xref2) {
		if (s_opt.editor == ED_JEDIT && ! s_opt.jeditOldCompletions) {
			ppcGenTwoNumericAndRecordBegin(PPC_FULL_MULTIPLE_COMPLETIONS, 
										   PPCA_NUMBER, max,
										   PPCA_NO_FOCUS, noFocus);
		} else {
#			ifdef ALL_COMPLETIONS_IN_ONE
			tlen = 0;
			for(cc=olc; cc!=NULL; cc=cc->next) {
				tlen += strlen(cc->fullName);
				if (cc->next!=NULL) tlen++;
			}
			if (completionsWillPrintEllipsis(olc)) tlen += 4;
			ppcGenAllCompletionsRecordBegin(noFocus, tlen);
#			else
			ppcGenRecordWithNumAttributeBegin(PPC_MULTIPLE_COMPLETIONS, PPCA_NO_FOCUS, noFocus);
#			endif
		}
	} else {
		fprintf(ccOut,";");
	}
}

void printOneCompletion(S_olCompletion *olc) {
	if (s_opt.editor == ED_JEDIT && ! s_opt.jeditOldCompletions) {
		fprintf(ccOut,"<%s %s=\"%s\" %s=%d %s=%d>", PPC_MULTIPLE_COMPLETION_LINE,
				PPCA_VCLASS, olc->vclass,
				PPCA_VALUE, olc->jindent,
				PPCA_LEN, strlen(olc->fullName));
		fprintf(ccOut, "%s", olc->fullName);
		fprintf(ccOut, "</%s>\n", PPC_MULTIPLE_COMPLETION_LINE);
	} else {
#		ifdef ALL_COMPLETIONS_IN_ONE
		fprintf(ccOut, "%s", olc->fullName);
#		else
		if (s_opt.xref2) {
			ppcGenRecord(PPC_MULTIPLE_COMPLETION_LINE, olc->fullName, "\n");
		} else {
			fprintf(ccOut, "%s", olc->fullName);
		}
#		endif
	}
}

void printCompletionsEnding(S_olCompletion *olc) {
	if (completionsWillPrintEllipsis(olc)) {
		if (s_opt.editor == ED_JEDIT && ! s_opt.jeditOldCompletions) {
		} else {
#			ifdef ALL_COMPLETIONS_IN_ONE
			fprintf(ccOut,"\n...");
#			endif
		}
	}
	if (s_opt.xref2) {
		if (s_opt.editor == ED_JEDIT && ! s_opt.jeditOldCompletions) {
			ppcGenRecordEnd(PPC_FULL_MULTIPLE_COMPLETIONS);
		} else {
#			ifdef ALL_COMPLETIONS_IN_ONE
			ppcGenRecordEnd(PPC_ALL_COMPLETIONS);
#			else
			ppcGenRecordEnd(PPC_MULTIPLE_COMPLETIONS);
#			endif
		}
	}
}

void printCompletionsList(int noFocus) {
	S_olCompletion *cc, *olc;
	olc = s_olcxCurrentUser->completionsStack.top->cpls;
	printCompletionsBeginning(olc, noFocus);
	for(cc=olc; cc!=NULL; cc=cc->next) {
		printOneCompletion(cc);
		if (cc->next!=NULL) fprintf(ccOut,"\n");
	}
	printCompletionsEnding(olc);
}

void printCompletions(S_completions* c) {
	int 				i, ii, indent, jindent, max, vFunCl, tlen, ellipsis;
	S_olCompletion 		*olc;
	char				*vclass;

	jindent = 0; vclass = NULL;
	// O.K. there will be a menu diplayed, clear the old one
	olCompletionListInit(&c->idToProcessPos);
	if (c->ai == 0) {
		if (s_opt.xref2) {
			ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 0, "** No completion possible **","\n");
		} else {
			fprintf(ccOut,"-");
		}
		goto finiWithoutMenu;
	}
	if ((! c->fullMatchFlag) && c->ai==1) {
		if (s_opt.xref2) {
			ppcGenGotoPositionRecord(&s_olcxCurrentUser->completionsStack.top->cpos);
			ppcGenRecord(PPC_SINGLE_COMPLETION, c->a[0].s,"\n");
		} else {
			fprintf(ccOut,".%s", c->comPrefix+c->idToProcessLen);
		}
		goto finiWithoutMenu;
	}
	if ((! c->fullMatchFlag) && strlen(c->comPrefix) > c->idToProcessLen) {
		if (s_opt.xref2) {
			ppcGenGotoPositionRecord(&s_olcxCurrentUser->completionsStack.top->cpos);
			ppcGenRecord(PPC_SINGLE_COMPLETION, c->comPrefix,"\n");
			ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 1, "Multiple completions","\n");
		} else {
			fprintf(ccOut,",%s", c->comPrefix+c->idToProcessLen);
		}
		goto finiWithoutMenu;
	}
	// this can't be ordered directly, because of overloading
//&		if (LANGUAGE(LAN_JAVA)) reorderCompletionArray(c);
	indent = c->maxLen;
	if (s_opt.olineLen - indent < MIN_COMPLETION_INDENT_REST) {
		indent = s_opt.olineLen - MIN_COMPLETION_INDENT_REST;
		if (indent < MIN_COMPLETION_INDENT) indent = MIN_COMPLETION_INDENT;
	}
	if (indent > MAX_COMPLETION_INDENT) indent = MAX_COMPLETION_INDENT;
	if (c->ai > s_opt.maxCompletions) max = s_opt.maxCompletions;
	else max = c->ai;
	for(ii=0; ii<max; ii++) {
		if (s_opt.editor == ED_JEDIT && ! s_opt.jeditOldCompletions) {
			sprintFullJeditCompletionInfo(c, ii, &jindent, &vclass);
		} else {
			sprintFullCompletionInfo(c, ii, indent);
		}
		vFunCl = s_noneFileIndex;
		if (LANGUAGE(LAN_JAVA)  && c->a[ii].vFunClass!=NULL) {
			vFunCl = c->a[ii].vFunClass->u.s->classFile;
			if (vFunCl == -1) vFunCl = s_noneFileIndex;
		}
		olc = olCompletionListPrepend(c->a[ii].s, ppcTmpBuff, vclass, jindent, c->a[ii].t,NULL, &s_noRef, c->a[ii].symType, vFunCl, s_olcxCurrentUser->completionsStack.top);
	}
	olCompletionListReverse();
	printCompletionsList(c->noFocusOnCompletions);
	fflush(ccOut);
	return;
 finiWithoutMenu:
	s_olcxCurrentUser->completionsStack.top = s_olcxCurrentUser->completionsStack.top->previous;			
	fflush(ccOut);
}

/* *********************************************************************** */

static int isTheSameSymbol(S_cline *c1, S_cline *c2) {
	int r;
	if (strcmp(c1->s, c2->s) != 0) return(0);
/*fprintf(dumpOut,"st %d %d\n",c1->symType,c2->symType);*/
	if (c1->symType != c2->symType) return(0);
	if (s_language != LAN_JAVA) return(1);
	if (c1->symType == TypeStruct) {
		if (c1->t!=NULL && c2->t!=NULL) {
			return(strcmp(c1->t->linkName, c2->t->linkName)==0);
		}
	}
	if (c1->symType != TypeDefault) return(1);
	assert(c1->t && c1->t->u.type);
	assert(c2->t && c2->t->u.type);
/*fprintf(dumpOut,"tm %d %d\n",c1->t->u.type->m,c2->t->u.type->m);*/
	if (c1->t->u.type->m != c2->t->u.type->m) return(0);
	if (c1->vFunClass != c2->vFunClass) return(0);
	if (c2->t->u.type->m != TypeFunction) return(1);
/*fprintf(dumpOut,"sigs %s %s\n",c1->t->u.type->u.sig,c2->t->u.type->u.sig);*/
	assert(c1->t->u.type->u.m.sig && c2->t->u.type->u.m.sig);
	if (strcmp(c1->t->u.type->u.m.sig,c2->t->u.type->u.m.sig)) return(0);
	return(1);
}

static int symbolIsInTab(S_cline *a, int ai, int *ii, char *s, S_cline *t) {
	int i,j;
	for(i= *ii-1; i>=0 && strcmp(a[i].s,s)==0; i--) ;
	for(j= *ii+1; j<ai && strcmp(a[j].s,s)==0; j++) ;
	/* from a[i] to a[j] symbols have the same names */
	for (i++; i<j; i++) if (isTheSameSymbol(&a[i],t)) return(1);
	return(0);
}

static int compareCompletionClassName(S_cline *c1, S_cline *c2) {
	char 	n1[TMP_STRING_SIZE];
	int 	c;
	char 	*n2;
	strcpy(n1, getCompletionClassFieldString(c1));
	n2 = getCompletionClassFieldString(c2);
	c = strcmp(n1, n2);
	if (c<0) return(-1);
	if (c>0) return(1);
	return(0);
}

static int completionOrderCmp(S_cline *c1, S_cline *c2) {
	int 	c, l1, l2;
	char 	*s1, *s2;
	if (! LANGUAGE(LAN_JAVA)) {
		// exact matches goes first
		s1 = strchr(c1->s, '(');
		if (s1 == NULL) l1 = strlen(c1->s);
		else l1 = s1 - c1->s;
		s2 = strchr(c2->s, '(');
		if (s2 == NULL) l2 = strlen(c2->s);
		else l2 = s2 - c2->s;
		if (l1 == s_completions.idToProcessLen && l2 != s_completions.idToProcessLen) return(-1);
		if (l1 != s_completions.idToProcessLen && l2 == s_completions.idToProcessLen) return(1);
		return(strcmp(c1->s, c2->s));
	} else {
		if (c1->symType==TypeKeyword && c2->symType!=TypeKeyword) return(1);
		if (c1->symType!=TypeKeyword && c2->symType==TypeKeyword) return(-1);

		if (c1->symType==TypeNonImportedClass && c2->symType!=TypeNonImportedClass) return(1);
		if (c1->symType!=TypeNonImportedClass && c2->symType==TypeNonImportedClass) return(-1);

		if (c1->symType!=TypeInheritedFullMethod && c2->symType==TypeInheritedFullMethod) return(1);
		if (c1->symType==TypeInheritedFullMethod && c2->symType!=TypeInheritedFullMethod) return(-1);
		if (c1->symType==TypeInheritedFullMethod && c2->symType==TypeInheritedFullMethod) {
			if (c1->t == NULL) return(1);    // "main"
			if (c2->t == NULL) return(-1);
			c = compareCompletionClassName(c1, c2);
			if (c!=0) return(c);
		}
		if (c1->symType!=TypeDefault && c2->symType==TypeDefault) return(1);
		if (c1->symType==TypeDefault && c2->symType!=TypeDefault) return(-1);

		// exact matches goes first
		l1 = strlen(c1->s);
		l2 = strlen(c2->s);
		if (l1 == s_completions.idToProcessLen && l2 != s_completions.idToProcessLen) return(-1);
		if (l1 != s_completions.idToProcessLen && l2 == s_completions.idToProcessLen) return(1);

		if (c1->symType==TypeDefault && c2->symType==TypeDefault) {
			c = c1->virtLevel - c2->virtLevel;
			if (c<0) return(-1);
			if (c>0) return(1);
			c = compareCompletionClassName(c1, c2);
			if (c!=0) return(c);
			// compare storages, fields goes first, then methods
			if (c1->t!=NULL && c2->t!=NULL) {
				if (c1->t->b.storage==StorageField && c2->t->b.storage==StorageMethod) return(-1);
				if (c1->t->b.storage==StorageMethod && c2->t->b.storage==StorageField) return(1);
			}
		}
		if (c1->symType==TypeNonImportedClass && c2->symType==TypeNonImportedClass) {
			// order by class name, not package
			s1 = lastOccurenceInString(c1->s, '.');
			if (s1 == NULL) s1 = c1->s;
			s2 = lastOccurenceInString(c2->s, '.');
			if (s2 == NULL) s2 = c2->s;
			return(strcmp(s1, s2));
		}
		return(strcmp(c1->s, c2->s));
	}
}

static int realyInsert(	S_cline *a, 
						int *aip, 
						char *s, 
						S_cline *t,
						int orderFlag
	) {
	int ai;
	int l,r,x,c,i,tmp;
	ai = *aip;
	l = 0; r = ai-1;
	assert(t->s == s);
	if (orderFlag) {
		// binary search
		while (l<=r) {
			x = (l+r)/2;
			c = completionOrderCmp(t, &a[x]);
//&		c = strcmp(s, a[x].s);
			if (c==0) { /* identifier yet in completions */
				if (s_language != LAN_JAVA) return(0); /* no overloading, so ... */
				if (symbolIsInTab(a, ai, &x, s, t)) return(0);
				r = x; l = x + 1;
				break;
			}
			if (c<0) r=x-1; else l=x+1;
		}
		assert(l==r+1);
	} else {
		// linear search
		for(l=0; l<ai; l++) {
			if (isTheSameSymbol(t, &a[l])) return(0); 
		}
	}
	if (orderFlag) {
		//& for(i=ai-1; i>=l; i--) a[i+1] = a[i];
		// should be faster, but frankly the weak point is not here.
		memmove(a+l+1, a+l, sizeof(S_cline)*(ai-l));
	} else {
		l = ai;
	}

	a[l] = *t;
	a[l].s = s;
	if (ai < MAX_COMPLETIONS-2) ai++;
	*aip = ai;
	return(1);
}

static void computeComPrefix(char *d, char *s) {
	while (*d == *s /*& && *s!='(' || (s_opt.completionCaseSensitive==0 && tolower(*d)==tolower(*s)) &*/) {
		if (*d == 0) return;
		d++; s++;
	}
	*d = 0;
}

static int completionTestPrefix(S_completions *ci, char *s) {
	char *d;
	d = ci->idToProcess;
	while (*d == *s || (s_opt.completionCaseSensitive==0 && tolower(*d)==tolower(*s))) {
		if (*d == 0) {
			ci->isCompleteFlag = 1;    /* complete, but maybe not unique*/
			return(0);
		}
		d++; s++;
	}
	if (*d == 0) return(0);
	return(1);
}

int stringContainsCaseInsensitive(char *s1, char *s2) {
	register char *p,*a,*b;
	for (p=s1; *p; p++) {
		for(a=p,b=s2; tolower(*a)==tolower(*b); a++,b++) ;
		if (*b==0) return(1);
	}
	return(0);
}

static void completionInsertName(char *name, S_cline *compLine, int orderFlag, 
								 S_completions *ci) {
	int len,l;
	//&compLine->s  = name;
	name = compLine->s;
	len = ci->idToProcessLen;
	if (ci->ai == 0) {
		strcpy(ci->comPrefix, name);
		ci->a[ci->ai] = *compLine;
		ci->a[ci->ai].s = name/*+len*/;
		ci->ai++;
		ci->maxLen = strlen(name/*+len*/);
		ci->fullMatchFlag = ((len==ci->maxLen) 
							 || (len==ci->maxLen-1 && name[len]=='(')
							 || (len==ci->maxLen-2 && name[len]=='('));
	} else {
/*		InternalCheck(ci->ai < MAX_COMPLETIONS-1);*/
		if (realyInsert(ci->a, &ci->ai, name/*+len*/, compLine, orderFlag)) {
			ci->fullMatchFlag = 0;
			l = strlen(name/*+len*/);
			if (l > ci->maxLen) ci->maxLen = l;
			computeComPrefix( ci->comPrefix, name);
		}
	}
}

void completeName(char *name, S_cline *compLine, int orderFlag, 
				S_completions *ci) {
	int len,l;
	if (name == NULL) return;
	if (completionTestPrefix(ci, name)) return;
	completionInsertName(name, compLine, orderFlag, ci);
}

void searchName(char *name, S_cline *compLine, int orderFlag, 
				S_completions *ci) {
	int len,l;
	if (name == NULL) return;

	if (s_opt.olcxSearchString==NULL || *s_opt.olcxSearchString==0) {
		// old fashioned search
		if (stringContainsCaseInsensitive(name, ci->idToProcess) == 0) return;
	} else {
		// the new one
		// since 1.6.0 this may not work, because switching to
		// regular expressions
		if (searchStringFitness(name, strlen(name)) == 0) return;
	}
	//&compLine->s = name;
	name = compLine->s;
	if (ci->ai == 0) {
		ci->fullMatchFlag = 0;
		ci->comPrefix[0]=0;
		ci->a[ci->ai] = *compLine;
		ci->a[ci->ai].s = name;
		ci->ai++;
		ci->maxLen = strlen(name);
	} else {
/*		InternalCheck(ci->ai < MAX_COMPLETIONS-1);*/
		if (realyInsert(ci->a, &ci->ai, name, compLine, 1)) {
			ci->fullMatchFlag = 0;
			l = strlen(name);
			if (l > ci->maxLen) ci->maxLen = l;
		}
	}
}

void processName(char *name, S_cline *compLine, int orderFlag, void *c) {
	S_completions *ci;
	ci = (S_completions *) c;
	if (s_opt.cxrefs == OLO_SEARCH) {
		searchName(name, compLine, orderFlag, ci);
	} else {
		completeName(name, compLine, orderFlag, ci);
	}
}

static void completeFun(S_symbol *s, void *c) {
	S_completionSymInfo *cc;
	S_cline compLine;
	cc = (S_completionSymInfo *) c;
	assert(s && cc);
	if (s->b.symType != cc->symType) return;
/*&fprintf(dumpOut,"testing %s\n",s->linkName);fflush(dumpOut);&*/
	if (s->b.symType != TypeMacro) {
		FILL_cline(&compLine, s->name, s, s->b.symType,0, 0, NULL,NULL);
	} else {
		if (s->u.mbody==NULL) {
			FILL_cline(&compLine, s->name, s, TypeUndefMacro,0, 0, NULL,NULL);
		} else {
			FILL_cline(&compLine, s->name, s, s->b.symType,0,
							s->u.mbody->argn, s->u.mbody->args,NULL);
		}
	}
	processName(s->name, &compLine, 1, cc->res);
}

#define CONST_CONSTRUCT_NAME(ccstorage,sstorage,completionName) {\
	if (ccstorage!=StorageConstructor && sstorage==StorageConstructor) {\
		completionName = NULL;\
	}\
	if (ccstorage==StorageConstructor && sstorage!=StorageConstructor) {\
		completionName = NULL;\
	}\
}

static void completeFunctionOrMethodName(S_completions *c, int orderFlag, int vlevel, S_symbol *r, S_symbol *vFunCl) {
	S_cline 		compLine;
	int 			cnamelen;
	char 			*cn, *cname, *psuff, *msig;
	cname = r->name;
	cnamelen = strlen(cname);
	if (s_opt.completeParenthesis == 0) {
		cn = cname;
	} else {
		assert(r->u.type!=NULL);
		if (LANGUAGE(LAN_JAVA)) {
			msig = r->u.type->u.m.sig;
			assert(msig!=NULL);
			if (msig[0]=='(' && msig[1]==')') {
				psuff = "()";
			} else {
				psuff = "(";
			}
		} else {
			if (r->u.type!=NULL && r->u.type->u.f.args == NULL) {
				psuff = "()";
			} else {
				psuff = "(";
			}
		}
		XX_ALLOCC(cn, cnamelen+strlen(psuff)+1, char);
		strcpy(cn, cname);
		strcpy(cn+cnamelen, psuff);
	}
	FILL_cline(&compLine, cn, r, TypeDefault,
			   vlevel,0,NULL,vFunCl);
	processName(cn, &compLine, orderFlag, (void*) c);
}

static void completeSymFun(S_symbol *s, void *c) {
	S_completionSymFunInfo *cc;
	S_cline compLine;
	char	*completionName;
	cc = (S_completionSymFunInfo *) c;
	assert(s);
	if (s->b.symType != TypeDefault) return;
	assert(s);
	if (cc->storage==StorageTypedef && s->b.storage!=StorageTypedef) return;
	completionName = s->name;
	CONST_CONSTRUCT_NAME(cc->storage,s->b.storage,completionName);
	if (completionName!=NULL) {
		if (s->b.symType == TypeDefault && s->u.type!=NULL && s->u.type->m == TypeFunction) {
			completeFunctionOrMethodName(cc->res, 1, 0, s, NULL);
		} else {
			FILL_cline(&compLine, completionName, s, s->b.symType,0, 0, NULL,NULL);
			processName(completionName, &compLine, 1, cc->res);
		}
	}
}

void completeStructs(S_completions*c) {
	S_completionSymInfo ii;
	FILL_completionSymInfo(&ii, c, TypeStruct);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
	FILL_completionSymInfo(&ii, c, TypeUnion);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
}

static int javaLinkable(unsigned storage, unsigned accessFlags) {
/*fprintf(dumpOut,"testing linkability %x %x\n",storage,accessFlags);fflush(dumpOut);*/
	if (s_language != LAN_JAVA) return(1);
	if (storage == ACC_ALL) return(1);
	if ((s_opt.ooChecksBits & OOC_LINKAGE_CHECK) == 0) return(1);
	if (storage & ACC_STATIC) return((accessFlags & ACC_STATIC) != 0);
	return(1);
}

static void processSpecialInheritedFullCompletion( S_completions *c, int orderFlag, int vlevel, S_symbol *r, S_symbol *vFunCl, char *cname) {
	int 	size, ll;
	char 	*fcc;
	char	tt[MAX_CX_SYMBOL_SIZE];
	S_cline compLine;

	tt[0]=0; ll=0; size=MAX_CX_SYMBOL_SIZE;
	if (LANGUAGE(LAN_JAVA)) {
		ll+=printJavaModifiers(tt+ll, &size, r->b.accessFlags);
	}
	typeSPrint(tt+ll, &size, r->u.type, cname, ' ', 0, 1,SHORT_NAME, NULL);
	XX_ALLOCC(fcc, strlen(tt)+1, char);
	strcpy(fcc,tt);
//&fprintf(dumpOut,":adding %s\n",fcc);fflush(dumpOut);
	FILL_cline(&compLine, fcc, r, TypeInheritedFullMethod,
			   vlevel,0,NULL,vFunCl);
	processName(fcc, &compLine, orderFlag, (void*) c);
}
static int getAccCheckOption() {
	int accCheck;
	if (s_opt.ooChecksBits & OOC_ACCESS_CHECK) {
		accCheck = ACC_CHECK_YES;
	} else {
		accCheck = ACC_CHECK_NO;
	}
	return(accCheck);
}

#define STR_REC_INFO(cc) (* cc->idToProcess == 0 && s_language!=LAN_JAVA)

static void completeRecordsNames(
							S_completions *c,
							S_symbol *s,
							unsigned accessMod,
							int classification,
							int constructorOpt,
							int completionType,
							int vlevelOffset
							) {
	S_cline 		compLine;
	int 			orderFlag,rr,vlevel, accCheck,  visibCheck, cnamelen;
	S_symbol 		*r, *vFunCl;
	S_recFindStr 	rfs;
	char			*cname, *cn, *fcc, *psuff, *msig;
	if (s==NULL) return;
	if (STR_REC_INFO(c)) {
		orderFlag = 0;
	} else {
		orderFlag = 1;
	}
	assert(s->u.s);
	iniFind(s, &rfs);
//&fprintf(dumpOut,"checking records of %s\n", s->linkName);
	for(;;) {
		// this is in fact about not cutting all records of the class,
		// not about visibility checks
		visibCheck = VISIB_CHECK_NO;
		accCheck = getAccCheckOption();
		//&if (s_opt.ooChecksBits & OOC_VISIBILITY_CHECK) {
		//&	visibCheck = VISIB_CHECK_YES;
		//&} else {
		//&	visibCheck = VISIB_CHECK_NO;
		//&}
		rr = findStrRecordSym(&rfs, NULL, &r, classification, accCheck, visibCheck);
		if (rr != RETURN_OK) break;
		if (constructorOpt==StorageConstructor && rfs.currClass!=s) break;
		/* because constructors are not inherited */
		assert(r);
		cname = r->name;
		CONST_CONSTRUCT_NAME(constructorOpt,r->b.storage,cname);
//&fprintf(dumpOut,"record %s\n", cname);
		if (	cname!=NULL
				&& *cname != 0
				&& r->b.symType != TypeError
				// Hmm. I hope it will not filter out something important
				&& (! symbolNameShouldBeHiddenFromReports(r->linkName))
				//	I do not know whether to check linkability or not
				//	What is more natural ???
				&& javaLinkable(accessMod,r->b.accessFlags)) {
//&fprintf(dumpOut,"passed\n", cname);
			assert(rfs.currClass && rfs.currClass->u.s);
			assert(r->b.symType == TypeDefault);
			vFunCl = rfs.currClass;
			if (vFunCl->u.s->classFile == -1) {
				vFunCl = NULL;
			}
			vlevel = rfs.sti + vlevelOffset;
			if (completionType == TypeInheritedFullMethod) {
				// TODO customizable completion level
				if (vlevel > 1 
					&& vlevel <= s_opt.completionOverloadWizardDeep+1
					&&  (r->b.accessFlags & ACC_PRIVATE)==0
					&&  (r->b.accessFlags & ACC_STATIC)==0) {
					processSpecialInheritedFullCompletion(c,orderFlag,vlevel,
														  r, vFunCl, cname);
				}
				c->comPrefix[0]=0;
			} else if (completionType == TypeSpecialConstructorCompletion) {
				FILL_cline(&compLine, c->idToProcess, r, TypeDefault,
						   vlevel,0,NULL,vFunCl);
				completionInsertName(c->idToProcess, &compLine, orderFlag, (void*) c);
			} else if (s_opt.completeParenthesis
					   && (r->b.storage==StorageMethod || r->b.storage==StorageConstructor)) {
				completeFunctionOrMethodName(c, orderFlag, vlevel, r, vFunCl);
			} else {
				FILL_cline(&compLine, cname, r, TypeDefault,
						   vlevel,0,NULL,vFunCl);
				processName(cname, &compLine, orderFlag, (void*) c);
			}
		}
	}
	if (STR_REC_INFO(c)) c->comPrefix[0] = 0;  // no common prefix completed
}


void completeRecNames(S_completions *c) {
    S_typeModifiers *str;
	S_symbol *s;
	assert(s_structRecordCompletionType);
	str = s_structRecordCompletionType;
	if (str->m == TypeStruct || str->m == TypeUnion) {
		s = str->u.t;
		assert(s);
		completeRecordsNames(c, s, ACC_DEFAULT,CLASS_TO_ANY, 
							 StorageDefault,TypeDefault,0);
	}
	s_structRecordCompletionType = &s_errorModifier;
}

static void completeFromSymTab(	S_completions*c, 
								unsigned storage
							  ){
	S_completionSymFunInfo 	ii;
	S_javaStat				*cs;
	int						vlevelOffset;
	FILL_completionSymFunInfo(&ii, c, storage);
	if (s_language == LAN_JAVA) {
		vlevelOffset = 0;
		for(cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL ;cs=cs->next) {
			symTabMap2(cs->locals, completeSymFun, (void*) &ii);
			completeRecordsNames(c, cs->thisClass, s_javaStat->cpMethodMods,
								 CLASS_TO_ANY,
								storage,TypeDefault,vlevelOffset);
			vlevelOffset += NEST_VIRT_COMPL_OFFSET;
		}
	} else {
		symTabMap2(s_symTab, completeSymFun, (void*) &ii);
	}
}

void completeEnums(S_completions*c) {
	S_completionSymInfo ii;
	FILL_completionSymInfo(&ii, c, TypeEnum);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
}

void completeLabels(S_completions*c) {
	S_completionSymInfo ii;
	FILL_completionSymInfo(&ii, c, TypeLabel);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
}

void completeMacros(S_completions*c) {
	S_completionSymInfo ii;
	FILL_completionSymInfo(&ii, c, TypeMacro);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
}

void completeTypes(S_completions* c) {
	completeFromSymTab(c, StorageTypedef);
}

void completeOthers(S_completions *c) {
	completeFromSymTab(c, StorageDefault);
	completeMacros(c);		/* handle macros as functions */
}

/* very costly function in time !!!! */
static S_symbol * getSymFromRef(S_reference *rr) {
	int 				i;
	S_symbolRefItem 	*ss;
	S_reference			*r;
	S_symbol			*sym;
	r = NULL; ss = NULL;
	// first visit all references, looking for symbol link name
	for(i=0; i<s_cxrefTab.size; i++) {
		ss = s_cxrefTab.tab[i];
		if (ss!=NULL) {
			for(r=ss->refs; r!=NULL; r=r->next) {
				if (rr == r) goto cont;
			}
		}
	}
 cont:
	if (i>=s_cxrefTab.size) return(NULL);
	assert(r==rr);
	// now look symbol table to find the symbol , berk!
	for(i=0; i<s_symTab->size; i++) {
		for(sym=s_symTab->tab[i]; sym!=NULL; sym=sym->next) {
			if (strcmp(sym->linkName,ss->name)==0) return(sym);
		}
	}
	return(NULL);
}

static int isEqualType(S_typeModifiers *t1, S_typeModifiers *t2) {
	S_typeModifiers *s1,*s2;
	S_symbol		*ss1,*ss2;
	assert(t1 && t2);
	for(s1=t1,s2=t2; s1->next!=NULL&&s2->next!=NULL; s1=s1->next,s2=s2->next) {
		if (s1->m!=s2->m) return(0);
	}
	if (s1->next!=NULL || s2->next!=NULL) return(0);
	if (s1->m != s2->m) return(0);
	if (s1->m==TypeStruct || s1->m==TypeUnion || s1->m==TypeEnum) {
		if (s1->u.t != s2->u.t) return(0);
	} else if (s1->m==TypeFunction) {
		if (LANGUAGE(LAN_JAVA)) {
			if (strcmp(s1->u.m.sig,s2->u.m.sig)!=0) return(0);
		} else {
			for(ss1=s1->u.f.args, ss2=s2->u.f.args;
				ss1!=NULL&&ss2!=NULL;
				ss1=ss1->next, ss2=ss2->next) {
				if (! isEqualType(ss1->u.type, ss2->u.type)) return(0);
			}
			if (ss1!=NULL||ss2!=NULL) return(0);
		}
	}
	return(1);
}

static char *spComplFindNextRecord(S_exprTokenType *tok) {
	S_recFindStr    rfs;
	int				rr;
	S_symbol		*r,*s;
	char			*cname,*res;
	static char		*cnext="next";
	static char		*cprevious="previous";
	s = tok->t->next->u.t;
	res = NULL;
	assert(s->u.s);
	iniFind(s, &rfs);
	for(;;) {
		rr = findStrRecordSym(&rfs, NULL, &r, CLASS_TO_ANY, ACC_CHECK_YES, VISIB_CHECK_YES);
		if (rr != RETURN_OK) break;
		assert(r);
		cname = r->name;
		CONST_CONSTRUCT_NAME(StorageDefault,r->b.storage,cname);
		if (	cname!=NULL && 
				javaLinkable(ACC_ALL,r->b.accessFlags)) {
			assert(rfs.currClass && rfs.currClass->u.s);
			assert(r->b.symType == TypeDefault);
			if (isEqualType(r->u.type, tok->t)) {
				// there is a record of the same type
				if (res == NULL) res = cname;
				else if (strcmp(cname,cnext)==0) res = cnext;
				else if (res!=cnext&&strcmp(cname,"previous")==0)res=cprevious;
			}
		}
	}
	return(res);
}

static int isForCompletionSymbol(S_completions *c,
								 S_exprTokenType *tok,
								 S_symbol **sym,
								 char	**nextRecord
	) {
	S_symbol	*ss,*sy;
	char		*rec;
	if (s_opt.cxrefs != OLO_COMPLETION)  return(0);
	if (tok->t==NULL) return(0);
	if (c->idToProcessLen != 0) return(0);
	if (tok->t->m == TypePointer) {
		assert(tok->t->next);
		if (tok->t->next->m == TypeStruct) {
			*sym = sy = getSymFromRef(tok->r);
			if (sy==NULL) return(0);
			*nextRecord = spComplFindNextRecord(tok);
			return(1);
		}
	}
	return(0);
}

void completeForSpecial1(S_completions* c) {
	static char 		ss[TMP_STRING_SIZE];
	char 				*rec;
	S_cline 			compLine;
	S_symbol			*sym;
	if (isForCompletionSymbol(c,&s_forCompletionType,&sym,&rec)) {
		sprintf(ss,"%s!=NULL; ", sym->name);
		FILL_cline(&compLine,ss,NULL,TypeSpecialComplet,0,0,NULL,NULL);
		completeName(ss, &compLine, 0, c);
	}
}

void completeForSpecial2(S_completions* c) {
	static char 		ss[TMP_STRING_SIZE];
	char 				*rec;
	S_cline 			compLine;
	S_symbol			*sym;
	if (isForCompletionSymbol(c, &s_forCompletionType,&sym,&rec)) {
		if (rec!=NULL) {
			sprintf(ss,"%s=%s->%s) {", sym->name, sym->name, rec);
			FILL_cline(&compLine,ss,NULL,TypeSpecialComplet,0,0,NULL,NULL);
			completeName(ss, &compLine, 0, c);
		}
	}
}

void completeUpFunProfile(S_completions* c) {
	S_symbol *dd;
	if (s_upLevelFunctionCompletionType!=NULL 
		&& c->idToProcess[0] == 0
		&& c->ai==0
		) {
		XX_ALLOC(dd, S_symbol);
		FILL_symbolBits(&dd->b,0,0, 0,0, 0, TypeDefault, StorageDefault,0);
		FILL_symbol(dd, "    ", "    ",s_noPos, dd->b, type ,
					s_upLevelFunctionCompletionType,NULL);
		FILL_cline(&c->a[0], "    ", dd, TypeDefault,0, 0, NULL, NULL);
		c->fullMatchFlag = 1;
		c->comPrefix[0]=0;
		c->ai++;
	}
}

/* *************************** JAVA completions ********************** */

static S_symbol * javaGetFileNameClass(char *fname) {
	char 		*pp;
	S_symbol 	*res;
	pp = javaCutClassPathFromFileName(fname);
	res = javaGetFieldClass(pp,NULL);
	return(res);
}


static void completeConstructorsFromFile(S_completions *c, char *fname) {
	S_symbol *memb;
//&fprintf(dumpOut,"comp %s\n", fname);
	memb = javaGetFileNameClass(fname);
//&fprintf(dumpOut,"comp %s <-> %s\n", memb->name, c->idToProcess);
	if (strcmp(memb->name, c->idToProcess)==0) {
		/* only when exact match, otherwise it would be too memory costly*/
//&fprintf(dumpOut,"O.K. %s\n", memb->linkName);
		javaLoadClassSymbolsFromFile(memb);
		completeRecordsNames(c, memb, ACC_ALL,CLASS_TO_ANY, StorageConstructor,TypeDefault,0);
	}
}

static void completeJavaConstructors(S_symbol *s, void *c) {
	if (s->b.symType != TypeStruct) return;
	completeConstructorsFromFile((S_completions *)c, s->linkName);
}

static void javaPackageNameCompletion(
									char 			*fname, 
									char		    *path, 
									char		 	*pack, 
									S_completions	*c,
									void 			*idp,
									int				*pstorage
								) {
	S_cline 		compLine;
	char			*cname;
	struct stat		stt;
	S_idIdentList	*id;
	id = (S_idIdentList *) idp;
	if (strchr(fname,'.')!=NULL) return; 		/* not very proper */
	/*& // original; Why this is commented out?
	FILLF_idIdentList(&ttl, fname, NULL, -1, 0, 0, fname, TypePackage, id);
	cname=javaCreateComposedName(path,&ttl,SLASH,NULL,tmpMemory,SIZE_TMP_MEM);
	if (statb(cname,&stt)!=0 || (stt.st_mode & S_IFMT) != S_IFDIR) return;
	&*/
	XX_ALLOCC(cname, strlen(fname)+1, char);
	strcpy(cname, fname);
 	FILL_cline(&compLine, cname, NULL, TypePackage,0, 0 , NULL,NULL);
	processName(cname, &compLine, 1, (void*) c);
}

static void javaTypeNameCompletion(
									char 			*fname, 
									char		    *path, 
									char		 	*pack, 
									S_completions	*c,
									void 			*idp,
									int				*pstorage
								) {
	char			cfname[MAX_FILE_NAME_SIZE];
	S_cline 		compLine;
	char			*cname,*suff;
	struct stat		stt;
	S_idIdentList	ttl,*id;
	int				len,storage,complType;
	S_symbol		*memb;
	memb = NULL;
	id = (S_idIdentList *) idp;
	if (pstorage == NULL) storage = StorageDefault;
	else storage = *pstorage;
	len = strlen(fname);
	complType = TypePackage;
/*&fprintf(dumpOut,":mapping %s\n",fname);fflush(dumpOut);&*/
	/*& // orig
	FILLF_idIdentList(&ttl, fname, NULL, -1, 0, 0, fname, TypePackage, id);
	cname=javaCreateComposedName(path,&ttl,SLASH,NULL,tmpMemory,SIZE_TMP_MEM);
	if (statb(cname,&stt)!=0 || (stt.st_mode & S_IFMT) != S_IFDIR) { //& }
	&*/
	if (strchr(fname,'.') != NULL) { 		/* not very proper */
		if (strchr(fname,'$') != NULL) return;
		suff = strchr(fname,'.');
		if (suff == NULL) return;
		if (strcmp(suff,".java")==0) len -= 5;
		else if (strcmp(suff,".class")==0) len -= 6;
		else return;
		complType = TypeStruct;
		sprintf(cfname,"%s/%s", pack, fname);
		InternalCheck(strlen(cfname)+1 < MAX_FILE_NAME_SIZE);
		memb = javaGetFieldClass(cfname,NULL);
		if (storage == StorageConstructor) {
			sprintf(cfname,"%s/%s/%s", path, pack, fname);
			InternalCheck(strlen(cfname)+1 < MAX_FILE_NAME_SIZE);
			memb = javaGetFileNameClass(cfname);
			completeConstructorsFromFile(c, cfname);
		}
	}
	XX_ALLOCC(cname, len+1, char);
	strncpy(cname, fname, len);
	cname[len]=0;
 	FILL_cline(&compLine, cname, memb, complType,0, 0 , NULL,NULL);
	processName(cname, &compLine, 1, (void*) c);
}

static void javaCompleteNestedClasses(	S_completions *c,
										S_symbol *cclas,
										int storage
										) {
	S_cline 		compLine;
	int				i;
	S_symbol		*memb;
	S_symbol 		*str;
	str = cclas;
	// TODO count inheritance and virtual levels (at least for nested classes)
	for(str=cclas; 
		str!=NULL && str->u.s->super!=NULL; 
		str=str->u.s->super->d) {
		assert(str && str->u.s);
		for(i=0; i<str->u.s->nnested; i++) {
			if (str->u.s->nest[i].membFlag) {
				memb = str->u.s->nest[i].cl;
				assert(memb);
				memb->b.accessFlags |= str->u.s->nest[i].accFlags;	// hack!!!
				FILL_cline(&compLine, memb->name, memb, TypeStruct,0, 0 , NULL,NULL);
				processName(memb->name, &compLine, 1, (void*) c);
				if (storage == StorageConstructor) {
					javaLoadClassSymbolsFromFile(memb);
					completeRecordsNames(c, memb, ACC_ALL,CLASS_TO_ANY, StorageConstructor,TypeDefault,0);
				}
			}
		}
	}
}

static void javaCompleteNestedClSingleName(S_completions *cc) {
	S_cline 	compLine;
	S_javaStat	*cs;
	S_symbol	*nest,*mc;
	int			i;
	for(cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
		javaCompleteNestedClasses(cc, cs->thisClass, StorageDefault);
	}
}


static void javaCompleteComposedName(
								S_completions*c,
								int classif,
								int storage,
								int innerConstruct
								) {
	S_symbol 		*str;
	S_typeModifiers *expr;
	int 			i,nameType;
	char			*p,*lp,*fullName;
	char			packName[MAX_FILE_NAME_SIZE];
	unsigned		accs;
	S_reference		*orr;
	nameType = javaClassifyAmbiguousName(s_javaStat->lastParsedName,NULL,&str,
										 &expr,&orr,NULL, USELESS_FQT_REFS_ALLOWED,classif,UsageUsed);
/*&
fprintf(dumpOut,"compl %s %s\n",s_javaStat->lastParsedName->idi.name,
typesName[nameType]);fflush(dumpOut);
&*/
	if (innerConstruct && nameType != TypeExpression) return;
	if (nameType == TypeExpression) {
		if (expr->m == TypeArray) expr = &s_javaArrayObjectSymbol.u.s->stype;
		if (expr->m != TypeStruct) return;
		str = expr->u.t;
		accs = ACC_DEFAULT;
	} else {
		accs = ACC_STATIC;
	}
	/* complete packages and classes from file system */
	if (nameType==TypePackage){
		javaCreateComposedName(NULL,s_javaStat->lastParsedName,'/',NULL,packName,MAX_FILE_NAME_SIZE);
		javaMapDirectoryFiles1(packName, javaTypeNameCompletion, 
							c,s_javaStat->lastParsedName, &storage);
	}
	/* complete inner classes */
	if (nameType==TypeStruct || innerConstruct){
		javaLoadClassSymbolsFromFile(str);
		javaCompleteNestedClasses(c, str, storage);
	}
	/* complete fields and methods */
	if (classif==CLASS_TO_EXPR && str!=NULL && storage!=StorageConstructor
			&& (nameType==TypeStruct || nameType==TypeExpression)) {
		javaLoadClassSymbolsFromFile(str);
		completeRecordsNames(c, str, accs,CLASS_TO_ANY, storage,TypeDefault,0);
	}
}

void javaHintCompleteMethodParameters(S_completions *c) {
	S_cline 				compLine;
	S_symbol 				*r, *vFunCl;
	S_recFindStr			*rfs;
	S_typeModifiersList		*aaa;
	int						visibCheck, accCheck, vlevel, rr, actArgi;
	char					*mname, *mess;
	char					actArg[MAX_PROFILE_SIZE];
	if (c->idToProcessLen != 0) return;
	if (s_cp.erfsForParamsComplet==NULL) return;
	r = s_cp.erfsForParamsComplet->memb;
	rfs = &s_cp.erfsForParamsComplet->s;
	mname = r->name;
	visibCheck = VISIB_CHECK_NO;
	accCheck = getAccCheckOption();
	// partiall actual parameters

	*actArg = 0; actArgi = 0; 
	for(aaa=s_cp.erfsForParamsComplet->params; aaa!=NULL; aaa=aaa->next) {
		actArgi += javaTypeToString(aaa->d,actArg+actArgi,MAX_PROFILE_SIZE-actArgi);
	}
	do {
		assert(r != NULL);
		if (*actArg==0 || javaMethodApplicability(r,actArg)==PROFILE_PARTIALLY_APPLICABLE){
			vFunCl = rfs->currClass;
			if (vFunCl->u.s->classFile == -1) {
				vFunCl = NULL;
			}
			vlevel = rfs->sti;
			FILL_cline(&compLine, r->name, r, TypeDefault,
					   vlevel,0,NULL,vFunCl);
			processName(r->name, &compLine, 0, (void*) c);
		}		
		rr = findStrRecordSym(rfs, mname, &r, CLASS_TO_METHOD, accCheck, visibCheck);
	} while (rr == RETURN_OK);
	if (c->ai != 0) {
		c->comPrefix[0]=0;
		c->fullMatchFlag = 1;
		c->noFocusOnCompletions = 1;
	}
	if (s_opt.cxrefs != OLO_SEARCH) s_completions.abortFurtherCompletions = 1;
}


void javaCompletePackageSingleName(S_completions*c) {
	javaMapDirectoryFiles2(NULL, javaPackageNameCompletion, c, NULL, NULL);
}

void javaCompleteThisPackageName(S_completions *c) {
	S_cline		compLine;
	static char	cname[TMP_STRING_SIZE];
	char 		*cc, *ss, *dd;
	if (c->idToProcessLen != 0) return;
	ss = javaCutSourcePathFromFileName(getRealFileNameStatic(s_fileTab.tab[s_olOriginalFileNumber]->name));
	strcpy(cname, ss);
	dd = lastOccurenceInString(cname, '.');
	if (dd!=NULL) *dd=0;
	javaDotifyFileName(cname);
	cc = lastOccurenceInString(cname, '.');
	if (cc==NULL) return;
	*cc++ = ';'; *cc = 0;
	FILL_cline(&compLine,cname,NULL,TypeSpecialComplet,0,0,NULL,NULL);
	completeName(cname, &compLine, 0, c);
}

static void javaCompleteThisClassDefinitionName(S_completions*c) {
	S_cline		compLine;
	static char cname[TMP_STRING_SIZE];
	char 		*cc;
	javaGetClassNameFromFileNum(s_olOriginalFileNumber, cname, DOTIFY_NAME);
	cc = strchr(cname,0);
	assert(cc!=NULL);
	*cc++ = ' '; *cc=0;
	cc = lastOccurenceInString(cname, '.');
	if (cc==NULL) return;
	cc++;
	FILL_cline(&compLine,cc,NULL,TypeSpecialComplet,0,0,NULL,NULL);
	completeName(cc, &compLine, 0, c);
}

void javaCompleteClassDefinitionNameSpecial(S_completions*c) {
	if (c->idToProcessLen != 0) return;
	javaCompleteThisClassDefinitionName(c);
}

void javaCompleteClassDefinitionName(S_completions*c) {
	S_completionSymInfo ii;
	javaCompleteThisClassDefinitionName(c);
	// order is important because of hack in nestedcl Access modifs
	javaCompleteNestedClSingleName(c);
	FILL_completionSymInfo(&ii, c, TypeStruct);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
}

void javaCompletePackageCompName(S_completions*c) {
	javaClassifyToPackageNameAndAddRefs(s_javaStat->lastParsedName, UsageUsed);
	javaMapDirectoryFiles2(s_javaStat->lastParsedName,
			javaPackageNameCompletion, c, s_javaStat->lastParsedName, NULL);
}

void javaCompleteTypeSingleName(S_completions*c) {
	S_completionSymInfo ii;
	// order is important because of hack in nestedcl Access modifs
	javaCompleteNestedClSingleName(c);
	javaMapDirectoryFiles2(NULL, javaTypeNameCompletion, c, NULL, NULL);
	FILL_completionSymInfo(&ii, c, TypeStruct);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
}

static void completeFqtFromFileName(char *file, void *cfmpi) {
	char 					ttt[MAX_FILE_NAME_SIZE];
	char 					sss[MAX_FILE_NAME_SIZE];
	char 					*tt,*suff, *sname, *ss;
	S_cline					compLine;
	S_completionFqtMapInfo	*fmi;
	S_completions			*c;
	S_symbol 				*memb;
	int						i;
	fmi = (S_completionFqtMapInfo *) cfmpi;
	c = fmi->res;
	suff = getFileSuffix(file);
	if (fnCmp(suff, ".class")==0 || fnCmp(suff, ".java")==0) {
		sprintf(ttt, "%s", file);
		InternalCheck(strlen(ttt) < MAX_FILE_NAME_SIZE-1);
		suff = lastOccurenceInString(ttt, '.');
		assert(suff);
		*suff = 0;
		sname = lastOccurenceOfSlashOrAntiSlash(ttt);
		if (sname == NULL) sname = ttt;
		else sname ++;
		if (pathncmp(c->idToProcess, sname, c->idToProcessLen, s_opt.completionCaseSensitive)==0
			|| (fmi->completionType == FQT_COMPLETE_ALSO_ON_PACKAGE
				&& pathncmp(c->idToProcess, ttt, c->idToProcessLen, s_opt.completionCaseSensitive)==0)) {
			memb = javaGetFieldClass(ttt, &sname);
			linkNamePrettyPrint(sss, ttt, TMP_STRING_SIZE, LONG_NAME);
			XX_ALLOCC(ss, strlen(sss)+1, char);
			strcpy(ss, sss);
			sname = lastOccurenceInString(ss,'.');
			// do not complete names not containing dot (== not fqt)
			if (sname!=NULL) {
				sname++;
				FILL_cline(&compLine,ss,memb,TypeNonImportedClass,0,0,NULL,NULL);
				if (fmi->completionType == FQT_COMPLETE_ALSO_ON_PACKAGE) {
					processName(ss, &compLine, 1, c);
				} else {
					processName(sname, &compLine, 1, c);
				}
				// reset common prefix and full match in order to always show window
				c->comPrefix[0] = 0;
				c->fullMatchFlag = 1;
			}
		}
	}
}

static void completeFqtClassFileFromZipArchiv(char *zip, char *file, void *cfmpi) {
	completeFqtFromFileName(file, cfmpi);
}

static int isItCurrentPackageName(char *fn) {
	int plen;
	if (s_javaThisPackageName!=NULL && s_javaThisPackageName[0]!=0) {
		plen = strlen(s_javaThisPackageName);
		if (fnnCmp(fn, s_javaThisPackageName, plen)==0) return(1);
	}
	return(0);
}

static void completeFqtClassFileFromFileTab(S_fileItem *fi, void *cfmpi) {
	char	*fn;
	assert(fi!=NULL);
	if (fi->name[0] == ZIP_SEPARATOR_CHAR) {
		// remove current package
		fn = fi->name+1;
		if (! isItCurrentPackageName(fn)) {
			completeFqtFromFileName(fn, cfmpi);
		}
	}
}

static void comleteRecursivelyFqtNamesFromDirectory(MAP_FUN_PROFILE) {
	char 			*fname, *dir, *path;
	char			fn[MAX_FILE_NAME_SIZE];
	struct stat		st;
	int				stt, plen;
	fname = file;
	dir = a1;
	path = a2;
	if (strcmp(fname,".")==0) return;
	if (strcmp(fname,"..")==0) return;
	// do not descent too deep
	if (strlen(dir)+strlen(fname) >= MAX_FILE_NAME_SIZE-50) return;
	sprintf(fn,"%s%c%s",dir,SLASH,fname);
	stt = statb(fn, &st);
	if (stt==0  && (st.st_mode & S_IFMT)==S_IFDIR) {
		mapDirectoryFiles(fn, comleteRecursivelyFqtNamesFromDirectory, DO_NOT_ALLOW_EDITOR_FILES, 
						  fn, path, NULL, a4, NULL);
	} else if (stt==0) {
		// O.K. cut the path
		assert(path!=NULL);
		plen = strlen(path);
		assert(fnnCmp(fn, path, plen)==0);
		if (fn[plen]!=0 && ! isItCurrentPackageName(fn)) {
			completeFqtFromFileName(fn+plen+1, a4);
		}
	}
}

static void javaFqtCompletions(S_completions *c, int completionType) {
	S_completionFqtMapInfo	cfmi;
	int 					i;
	S_stringList			*pp;

	FILL_completionFqtMapInfo(&cfmi, c, completionType);
	if (s_opt.fqtNameToCompletions == 0) return;
	// fqt from .jars
	for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchivTab[i].fn[0]!=0; i++) {
		fsRecMapOnFiles(s_zipArchivTab[i].dir, s_zipArchivTab[i].fn, 
						"", completeFqtClassFileFromZipArchiv, &cfmi);
	}
	if (s_opt.fqtNameToCompletions <= 1) return;
	// fqt from filetab
	for(i=0; i<s_fileTab.size; i++) {
		if (s_fileTab.tab[i]!=NULL) completeFqtClassFileFromFileTab(s_fileTab.tab[i], &cfmi);
	}
	if (s_opt.fqtNameToCompletions <= 2) return;
	// fqt from classpath
	for(pp=s_javaClassPaths; pp!=NULL; pp=pp->next) {
		mapDirectoryFiles(pp->d, comleteRecursivelyFqtNamesFromDirectory,DO_NOT_ALLOW_EDITOR_FILES,
						  pp->d,pp->d,NULL,&cfmi,NULL);
	}
	if (s_opt.fqtNameToCompletions <= 3) return;
	// fqt from sourcepath
	JavaMapOnPaths(s_javaSourcePaths, {
		mapDirectoryFiles(currentPath, comleteRecursivelyFqtNamesFromDirectory,DO_NOT_ALLOW_EDITOR_FILES,
						  currentPath,currentPath,NULL,&cfmi,NULL);		
	});
}

void javaHintCompleteNonImportedTypes(S_completions*c) {
	javaFqtCompletions(c, DEFAULT_VALUE);
}

void javaHintImportFqt(S_completions*c) {
	javaFqtCompletions(c, FQT_COMPLETE_ALSO_ON_PACKAGE);
}

void javaHintVariableName(S_completions*c) {
	S_cline compLine;
	char	ss[TMP_STRING_SIZE];
	char	*name, *affect1, *affect2;

	if (! LANGUAGE(LAN_JAVA)) return;
	if (c->idToProcessLen != 0) return;
	if (s_lastReturnedLexem != IDENTIFIER) return;

	sprintf(ss, "%s", uniyylval->bbidIdent.d->name);
	//&sprintf(ss, "%s", yytext);
	if (ss[0]!=0) ss[0] = tolower(ss[0]);
	XX_ALLOCC(name, strlen(ss)+1, char);
	strcpy(name, ss);
	sprintf(ss, "%s = new %s", name, uniyylval->bbidIdent.d->name);
	//&sprintf(ss, "%s = new %s", name, yytext);
	XX_ALLOCC(affect1, strlen(ss)+1, char);
	strcpy(affect1, ss);
	sprintf(ss, "%s = null;", name);
	XX_ALLOCC(affect2, strlen(ss)+1, char);
	strcpy(affect2, ss);
	FILL_cline(&compLine, affect1, NULL, TypeSpecialComplet,0,0,NULL,NULL);
	processName(affect1, &compLine, 0, c);
	FILL_cline(&compLine, affect2, NULL, TypeSpecialComplet,0,0,NULL,NULL);
	processName(affect2, &compLine, 0, c);
	FILL_cline(&compLine, name, NULL, TypeSpecialComplet,0,0,NULL,NULL);
	processName(name, &compLine, 0, c);
	c->comPrefix[0] = 0;

}

void javaCompleteTypeCompName(S_completions *c) {
	javaCompleteComposedName(c, CLASS_TO_TYPE, StorageDefault,0);
}

void javaCompleteConstructSingleName(S_completions *c) {
	S_completionSymFunInfo ii;
	symTabMap2(s_symTab, completeJavaConstructors, c);
// commented, because do not understand why it is here
//&	FILL_completionSymFunInfo(&ii, c, StorageConstructor);
//&	symTabMap2(s_symTab, completeSymFun, (void*) &ii);
}

void javaCompleteHintForConstructSingleName(S_completions *c) {
	S_cline 	compLine;
	char		*name;
	if (c->idToProcessLen == 0 && s_opt.cxrefs == OLO_COMPLETION) {
		// O.K. wizard completion
		if (s_cps.lastAssignementStruct!=NULL) {
			name = s_cps.lastAssignementStruct->name;
			FILL_cline(&compLine, name, NULL, TypeSpecialComplet,0,0,NULL,NULL);
			processName(name, &compLine, 0, c);					}
	}
}

void javaCompleteConstructCompName(S_completions*c) {
	javaCompleteComposedName(c, CLASS_TO_TYPE, StorageConstructor,0);
}

void javaCompleteConstructNestNameName(S_completions*c) {
	javaCompleteComposedName(c, CLASS_TO_EXPR, StorageConstructor,1);
}

void javaCompleteConstructNestPrimName(S_completions*c) {
	S_symbol *memb;
	if (s_javaCompletionLastPrimary == NULL) return;
	if (s_javaCompletionLastPrimary->m == TypeStruct) {
		memb = s_javaCompletionLastPrimary->u.t;
	} else return;
	assert(memb);
	javaCompleteNestedClasses(c, memb, StorageConstructor);
}

void javaCompleteExprSingleName(S_completions*c) {
	S_completionSymInfo ii;
	javaMapDirectoryFiles1(NULL, javaTypeNameCompletion, c, NULL, NULL);
	FILL_completionSymInfo(&ii, c, TypeStruct);
	symTabMap2(s_symTab, completeFun, (void*) &ii);
	completeFromSymTab(c, StorageDefault);
}

void javaCompleteThisConstructor (S_completions *c) {
	S_symbol *memb;
	if (strcmp(c->idToProcess,"this")!=0) return;
	if (s_opt.cxrefs == OLO_SEARCH) return;
	memb = s_javaStat->thisClass;
	javaLoadClassSymbolsFromFile(memb);
	completeRecordsNames(c, memb, ACC_ALL,CLASS_TO_ANY, StorageConstructor,
						 TypeSpecialConstructorCompletion,0);
}

void javaCompleteSuperConstructor (S_completions *c) {
	S_symbol *memb;
	if (strcmp(c->idToProcess,"super")!=0) return;
	if (s_opt.cxrefs == OLO_SEARCH) return;
	memb = javaCurrentSuperClass();
	javaLoadClassSymbolsFromFile(memb);
	completeRecordsNames(c, memb, ACC_ALL,CLASS_TO_ANY, StorageConstructor,
						 TypeSpecialConstructorCompletion,0);
}

void javaCompleteSuperNestedConstructor (S_completions *c) {
	if (strcmp(c->idToProcess,"super")!=0) return;
	//TODO!!!
}

void javaCompleteExprCompName(S_completions*c) {
	javaCompleteComposedName(c, CLASS_TO_EXPR, StorageDefault,0);
}

void javaCompleteStrRecordPrimary(S_completions*c) {
	S_symbol *memb;
	if (s_javaCompletionLastPrimary == NULL) return;
	if (s_javaCompletionLastPrimary->m == TypeStruct) {
		memb = s_javaCompletionLastPrimary->u.t;
	} else if (s_javaCompletionLastPrimary->m == TypeArray) {
		memb = &s_javaArrayObjectSymbol;
	} else return;
	assert(memb);
	javaLoadClassSymbolsFromFile(memb);
/*fprintf(dumpOut,": completing %s\n",memb->linkName);fflush(dumpOut);*/
	completeRecordsNames(c, memb, ACC_DEFAULT,CLASS_TO_ANY, StorageDefault,TypeDefault,0);
}

void javaCompleteStrRecordSuper(S_completions*c) {
	S_symbol *memb;
	memb = javaCurrentSuperClass();
	if (memb == &s_errorSymbol || memb->b.symType==TypeError) return;
	assert(memb);
	javaLoadClassSymbolsFromFile(memb);
	completeRecordsNames(c, memb, s_javaStat->cpMethodMods,CLASS_TO_ANY, StorageDefault,TypeDefault,0);
}

void javaCompleteStrRecordQualifiedSuper(S_completions*c) {
	S_symbol			*str;
	S_typeModifiers		*expr;
	S_idIdentList		*ii;
	S_reference			*rr, *lastUselessRef;
	int					ttype;
	lastUselessRef = NULL;
	ttype = javaClassifyAmbiguousName(s_javaStat->lastParsedName, NULL,&str,&expr,&rr, 
									  &lastUselessRef, USELESS_FQT_REFS_ALLOWED,CLASS_TO_TYPE,UsageUsed);
	if (ttype != TypeStruct) return;
	javaLoadClassSymbolsFromFile(str);
	str = javaGetSuperClass(str);
	if (str == &s_errorSymbol || str->b.symType==TypeError) return;
	assert(str);
	completeRecordsNames(c, str, s_javaStat->cpMethodMods,CLASS_TO_ANY, StorageDefault,TypeDefault,0);
}

void javaCompleteUpMethodSingleName(S_completions*c) {
	S_completionSymFunInfo 	ii;
	if (s_javaStat!=NULL) {
		completeRecordsNames(c, s_javaStat->thisClass, ACC_ALL,CLASS_TO_ANY,
							 StorageDefault,TypeDefault,0);
	}	
}

void javaCompleteFullInheritedMethodHeader(S_completions*c) {
	S_completionSymFunInfo 	ii;
	S_cline 				compLine;
	char 					*maindecl;
	if (c->idToProcessLen != 0) return;
	if (s_javaStat!=NULL) {
		completeRecordsNames(c,s_javaStat->thisClass,ACC_ALL,CLASS_TO_METHOD,
							 StorageDefault,TypeInheritedFullMethod,0);
	}
#if ZERO
	// completing main is sometimes very strange, especially
	// in case there is no inherited method from direct superclass
	maindecl = "public static void main(String args[])";
	FILL_cline(&compLine, maindecl, NULL, TypeInheritedFullMethod,
			   0,0,NULL,NULL);
	processName(maindecl, &compLine, 0, (void*) c);
#endif
}

/* this is unused */
void javaCompleteMethodCompName(S_completions*c) {
	javaCompleteExprCompName(c);
}


/* ************************** Yacc stuff ************************ */

static void completeFromXrefFun(S_symbolRefItem *s, void *c) {
	S_completionSymInfo *cc;
	S_cline compLine;
	cc = (S_completionSymInfo *) c;
	assert(s && cc);
	if (s->b.symType != cc->symType) return;
/*&fprintf(dumpOut,"testing %s\n",s->name);fflush(dumpOut);&*/
	FILL_cline(&compLine, s->name, NULL, s->b.symType,0, 0, NULL,NULL);
	processName(s->name, &compLine, 1, cc->res);
}

void completeYaccLexem(S_completions*c) {
	S_completionSymInfo ii;
	FILL_completionSymInfo(&ii, c, TypeYaccSymbol);
	refTabMap2(&s_cxrefTab, completeFromXrefFun, (void*) &ii);
}


/*
	$Revision: 1.12 $
	$Date: 2002/08/03 21:43:56 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "bitmaps.h"

#include "protocol.h"
//

static bitArray tmpChRelevant[BIT_ARR_DIM(MAX_FILES)];
static bitArray tmpChProcessed[BIT_ARR_DIM(MAX_FILES)];
static bitArray tmpChMarkProcessed[BIT_ARR_DIM(MAX_FILES)];

static S_olSymbolsMenu *tmpVApplClassBackPointersToMenu[MAX_FILES];

static int s_symbolListOutputCurrentLine =0;


static void clearTmpChRelevant() {
	memset(tmpChRelevant, 0, sizeof(tmpChRelevant));
}
static void clearTmpChProcessed() {
	memset(tmpChProcessed, 0, sizeof(tmpChProcessed));
}
static void clearTmpChMarkProcessed() {
	memset(tmpChMarkProcessed, 0, sizeof(tmpChMarkProcessed));
}
static void clearTmpClassBackPointersToMenu() {
	// this should be rather cycle affecting NULLs
	memset(tmpVApplClassBackPointersToMenu, 0, sizeof(tmpVApplClassBackPointersToMenu));
}

int classHierarchyClassNameLess(int c1, int c2) {
	char *n2, *nn;
	S_fileItem *fi1, *fi2;
	int ccc;
	char ttt[MAX_FILE_NAME_SIZE];
	fi1 = s_fileTab.tab[c1];
	fi2 = s_fileTab.tab[c2];
	assert(fi1 && fi2);
	// SMART, put interface largest, so they will be at the end
	if (fi2->b.isInterface && ! fi1->b.isInterface) return(1);
	if (fi1->b.isInterface && ! fi2->b.isInterface) return(0);
	nn = fi1->name;
	nn = javaGetNudePreTypeName_st(getRealFileNameStatic(nn),s_opt.nestedClassDisplaying);
	strcpy(ttt,nn);
	nn = fi2->name;
    nn = javaGetNudePreTypeName_st(getRealFileNameStatic(nn),s_opt.nestedClassDisplaying);
	ccc = strcmp(ttt,nn);
	if (ccc!=0) return(ccc<0);
	ccc = strcmp(fi1->name, fi2->name);
	return(ccc<0);
}

int classHierarchySupClassNameLess(S_chReference *c1, S_chReference *c2) {
	return(classHierarchyClassNameLess(c1->clas, c2->clas)); 
}

static int markTransitiveRelevantSubsRec(int cind, int pass) {
	S_fileItem 		*fi, *tt;
	S_chReference 	*s;
	int				res;
	fi = s_fileTab.tab[cind];
	if (THEBIT(tmpChMarkProcessed,cind)) return(THEBIT(tmpChRelevant,cind));
 	SETBIT(tmpChMarkProcessed, cind);
	res = 0;
	for(s=fi->infs; s!=NULL; s=s->next) {
		tt = s_fileTab.tab[s->clas];
		assert(tt);
		// do not descend from class to an
		// interface, because of Object -> interface lapsus ?
		// if ((! fi->b.isInterface) && tt->b.isInterface ) continue;
		// do not mix interfaces in first pass
//&		if (pass==FIRST_PASS && tt->b.isInterface) continue;
		if (markTransitiveRelevantSubsRec(s->clas, pass)) {
//&fprintf(dumpOut,"setting %s relevant\n",fi->name);
			SETBIT(tmpChRelevant, cind);
		}
	}
	return(THEBIT(tmpChRelevant,cind));
}

static void markTransitiveRelevantSubs(int cind, int pass) {
	S_fileItem 		*fi;
//&fprintf(dumpOut,"\n PRE checking %s relevant\n",fi->name);
	fi = s_fileTab.tab[cind];
	if (THEBIT(tmpChRelevant,cind)==0) return;
	markTransitiveRelevantSubsRec(cind, pass);
}

void classHierarchyGenInit() {
	clearTmpChRelevant();
	clearTmpChProcessed();
}

void setTmpClassBackPointersToMenu(S_olSymbolsMenu *menu) {
	S_olSymbolsMenu *ss;
	clearTmpClassBackPointersToMenu();
	for(ss=menu; ss!=NULL; ss=ss->next) {
		tmpVApplClassBackPointersToMenu[ss->s.vApplClass] = ss;
	}
}

static void genClassHierarchyVerticalBars( FILE *ff, S_intlist *nextbars, 
										   int secondpass) {
	S_intlist *nn;
	if (s_opt.xref2 && s_opt.taskRegime!=RegimeHtmlGenerate) {
		fprintf(ff," %s=\"", PPCA_TREE_DEPS);
	}
	if (nextbars!=NULL) {
		LIST_REVERSE(S_intlist, nextbars);
		for(nn = nextbars; nn!=NULL; nn=nn->next) {
			if (nn->next==NULL) {
				if (secondpass) fprintf(ff,"  +- ");
				else fprintf(ff,"  |  ");
			}
			else if (nn->i) fprintf(ff,"  | ");
			else fprintf(ff,"    ");
		}
		LIST_REVERSE(S_intlist, nextbars);
	}
	if (s_opt.xref2 && s_opt.taskRegime!=RegimeHtmlGenerate) {
		fprintf(ff,"\"");
	}
}


/* ******************************************************************* */
/* ***************************** HTML ******************************** */
/* ******************************************************************* */


static S_olSymbolsMenu *htmlItemInOriginalList(S_olSymbolsMenu *orr, int fInd) {
	S_olSymbolsMenu *rr;
	if (fInd == -1) return(NULL);
	assert(fInd>=0 && fInd<MAX_FILES);
	rr = tmpVApplClassBackPointersToMenu[fInd];
	//& LIST_FIND(S_olSymbolsMenu, fInd, s.vApplClass, orr, rr);
	return(rr);
}

static void htmlCHEmptyIndent( FILE *ff ) {
  fprintf(ff,"    ");
}


static void htmlPrintClassHierarchyLine( FILE *ff, int fInd, 
										 S_intlist *nextbars, 
										 int virtFlag, 
										 S_olSymbolsMenu *itt ) {
	char *cname, *pref1, *pref2, *suf1, *suf2;
	S_fileItem *fi;
	int cnt;

	fi = s_fileTab.tab[fInd];
	htmlCHEmptyIndent( ff);
	genClassHierarchyVerticalBars( ff, nextbars, 0);
	fprintf(ff,"\n");

	cname = javaGetNudePreTypeName_st(getRealFileNameStatic(fi->name),
									  s_opt.nestedClassDisplaying);
	pref1 = pref2 = suf1 = suf2 = "";
	if (fi->b.isInterface) {
		pref2 = "<em>"; suf2 = "</em>";
	}			
	if (itt!=NULL && THEBIT(tmpChProcessed,fInd)==0) {
		assert(itt);
		genClassHierarchyItemLinks( ff, itt, virtFlag);
		genClassHierarchyVerticalBars( ff, nextbars, 1);
		if (itt->s.vApplClass == itt->s.vFunClass) {
			pref1 = "<B>"; suf1 = "</B>";
		}
		fprintf(ff,"%s%s%s%s%s", pref1, pref2, cname, suf2, suf1);
		cnt = itt->defRefn+itt->refn;
		if (cnt > 0) {
			fprintf(ff,"\t\t(%d)", cnt);
		}
	} else {
	    htmlCHEmptyIndent( ff);
	    genClassHierarchyVerticalBars( ff, nextbars, 1);
		fprintf(ff,"%s%s%s%s%s", pref1, pref2, cname, suf2, suf1);
		//&fprintf(ff,"\t\t(0)");
	}
	if (THEBIT(tmpChProcessed,fInd)==1) {
		fprintf(ff, " ---> up");
	}
	fprintf(ff,"\n");
}

static void olcxPrintMenuItemPrefix(FILE *ff, S_olSymbolsMenu *itt, 
								   int selectable) {
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	if (s_opt.cxrefs==OLO_CLASS_TREE || s_opt.cxrefs==OLO_SHOW_CLASS_TREE) {
		fprintf(ff,"");
	} else if (! selectable) {
		if (s_opt.xref2) fprintf(ff, " %s=2", PPCA_SELECTED);
		else fprintf(ff,"- ");
	} else if (itt!=NULL && itt->selected) {
		if (s_opt.xref2) fprintf(ff, " %s=1", PPCA_SELECTED);
		else fprintf(ff,"+ ");
	} else {
		if (s_opt.xref2) fprintf(ff, " %s=0", PPCA_SELECTED);
		else fprintf(ff,"  ");
	}
	if (itt!=NULL 
		&& itt->vlevel==1 
		&& ooBitsGreaterOrEqual(itt->ooBits, OOC_PROFILE_APPLICABLE)) {
		if (s_opt.xref2) fprintf(ff, " %s=1", PPCA_BASE);
		else fprintf(ff,">>");
	} else {
		if (s_opt.xref2) fprintf(ff, " %s=0", PPCA_BASE);
		else fprintf(ff,"  ");
	}
	if (! s_opt.xref2) fprintf(ff," "); 
	if (s_opt.cxrefs==OLO_CLASS_TREE || s_opt.cxrefs==OLO_SHOW_CLASS_TREE) {
		fprintf(ff, "");
	} else if (itt==NULL || (itt->defRefn==0 && itt->refn==0) || !selectable) {
		if (s_opt.xref2) fprintf(ff, " %s=0 %s=0", PPCA_DEF_REFN, PPCA_REFN);
		else fprintf(ff, "  -/-  ");
	} else if (itt->defRefn==0) {
		if (s_opt.xref2) fprintf(ff, " %s=0 %s=%d", PPCA_DEF_REFN, PPCA_REFN, itt->refn);
		else fprintf(ff, "  -/%-3d", itt->refn);
	} else if (itt->refn==0) {
		if (s_opt.xref2) fprintf(ff, " %s=%d %s=0", PPCA_DEF_REFN, itt->defRefn, PPCA_REFN);
		else fprintf(ff, "%3d/-  ", itt->defRefn);
	} else {
		if (s_opt.xref2) fprintf(ff, " %s=%d %s=%d", PPCA_DEF_REFN, itt->defRefn, PPCA_REFN, itt->refn);
		else fprintf(ff, "%3d/%-3d", itt->defRefn, itt->refn);
	}
	if (! s_opt.xref2) fprintf(ff,"    ");
}

static void olcxMenuGenNonVirtualGlobSymList( FILE *ff, S_olSymbolsMenu *ss) {
	int ii;
	char ttt[MAX_CX_SYMBOL_SIZE];
	if (s_symbolListOutputCurrentLine == 1) s_symbolListOutputCurrentLine++; // first line irregularity
	ss->outOnLine = s_symbolListOutputCurrentLine;
	s_symbolListOutputCurrentLine ++ ;
	if (s_opt.xref2) {
		ppcIndentOffset();
		fprintf(ff,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, ss->outOnLine+SYMBOL_MENU_FIRST_LINE);
		if (ss->s.b.symType!=TypeDefault) {
			fprintf(ff," %s=%s", PPCA_TYPE, typesName[ss->s.b.symType]);
		}
		olcxPrintMenuItemPrefix(ff, ss, 1);
		sprintfSymbolLinkName(ttt, ss);
		fprintf(ff," %s=%d>%s</%s>\n", PPCA_LEN, strlen(ttt), ttt, PPC_SYMBOL);
	} else {
		fprintf(ff,"\n");
		olcxPrintMenuItemPrefix(ff, ss, 1);
		printSymbolLinkName(ff, ss);
		if (ss->s.b.symType != TypeDefault) {
			fprintf(ff,"\t(%s)", typesName[ss->s.b.symType]);
		}
//&fprintf(ff," ==%s %o (%s) at %x", ss->s.name, ss->ooBits, refCategoriesName[ss->s.b.category], ss);
	}
}

static void olcxMenuPrintClassHierarchyLine( FILE *ff, int fInd, 
										 S_intlist *nextbars, 
										 int virtFlag, 
										 S_olSymbolsMenu *itt ) {
	char 		*cname;
	S_fileItem 	*fi;
	int 		yetProcesed, indent;

	yetProcesed = THEBIT(tmpChProcessed,fInd);
	if (s_opt.xref2) {
		ppcIndentOffset();
		fprintf(ff, "<%s %s=%d", PPC_CLASS, PPCA_LINE,
				itt->outOnLine+SYMBOL_MENU_FIRST_LINE);
	}
	olcxPrintMenuItemPrefix(ff, itt, ! yetProcesed);
	if (s_opt.xref2) {
		LIST_LEN(indent, S_intlist, nextbars);
		fprintf(ff, " %s=%d", PPCA_INDENT, indent);
	} 
	genClassHierarchyVerticalBars( ff, nextbars, 1);
	fi = s_fileTab.tab[fInd];
	if (itt!=NULL) {
		assert(itt);
		if (itt->s.vApplClass == itt->s.vFunClass && s_opt.cxrefs!=OLO_CLASS_TREE) {
			if (s_opt.xref2) fprintf(ff, " %s=1", PPCA_DEFINITION);
			else fprintf(ff,"*");
		}
		itt->visible = 1;
	}
	if (fi->b.isInterface) {
		if (s_opt.xref2) fprintf(ff, " %s=1", PPCA_INTERFACE);
		else fprintf(ff,"~");
	}
	cname = javaGetNudePreTypeName_st(getRealFileNameStatic(fi->name), 
									  s_opt.nestedClassDisplaying);
	if (s_opt.xref2) {
		if (THEBIT(tmpChProcessed,fInd)) fprintf(ff," %s=1", PPCA_TREE_UP);
		fprintf(ff, " %s=%d>%s</%s>\n", PPCA_LEN, strlen(cname), cname, PPC_CLASS);
	} else {
		if (THEBIT(tmpChProcessed,fInd)) fprintf(ff,"(%s) -> up", cname);
		else fprintf(ff,"%s", cname);
//&if(itt!=NULL)fprintf(ff," %o %s", itt->ooBits, storagesName[itt->s.b.storage]);
		fprintf(ff,"\n");
	}
}


static void descendTheClassHierarchy(	FILE *ff, 
										int vApplCl, int oldvFunCl,
										S_olSymbolsMenu *rrr, 
										int level,
										S_intlist *nextbars,
										int virtFlag,
										int pass
	) {
	S_intlist		snextbar,*nn;
	S_fileItem 		*tt, *fi;
	S_chReference 	*s,*ol,*snext;
	S_olSymbolsMenu 	*itt;
	S_symbolRefItem		*ri;
	char 			*cname,*pref1,*pref2,*suf1,*suf2;
	int				i, vFunCl;
	fi = s_fileTab.tab[vApplCl];
	assert(fi!=NULL);
	if (THEBIT(tmpChRelevant,vApplCl)==0) return;
	itt = htmlItemInOriginalList(rrr, vApplCl);

	if (itt == NULL) {
		assert(rrr);
		vFunCl = oldvFunCl;
		// O.K. create new item, so that browse class action will work
		itt = olCreateNewMenuItem(&rrr->s, vApplCl, vFunCl, &s_noPos, UsageNone, 
								  0, 1, 0, UsageNone, 0);
		// insert it into the list, no matter where?
		itt->next = rrr->next;
		rrr->next = itt;
		tmpVApplClassBackPointersToMenu[vApplCl] = itt;
		//&sprintf(tmpBuff, "adding %s (%s)", itt->s.name, s_fileTab.tab[itt->s.vApplClass]);
		//&ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
	} else {
		vFunCl = itt->s.vFunClass;
	}

	if (s_symbolListOutputCurrentLine == 1) s_symbolListOutputCurrentLine++; // first line irregularity
	if (itt!=NULL && itt->outOnLine==0) itt->outOnLine = s_symbolListOutputCurrentLine;
	s_symbolListOutputCurrentLine ++;
	if (s_opt.taskRegime==RegimeHtmlGenerate) {
		htmlPrintClassHierarchyLine( ff, vApplCl, nextbars, virtFlag, itt);
	} else {
		olcxMenuPrintClassHierarchyLine( ff, vApplCl, nextbars, virtFlag, itt);
	}

	if (THEBIT(tmpChProcessed,vApplCl)==1) return;
	SETBIT(tmpChProcessed, vApplCl);

	// putting the following in commentary makes that for -refnum==1
    // subclasses will not be sorted !
	// also subclasses for on-line resolution would not be sorted!
	LIST_MERGE_SORT(S_chReference, fi->infs, classHierarchySupClassNameLess);
	s=fi->infs; 
	while (s!=NULL) {
		assert(s_fileTab.tab[s->clas]);
		snext = s->next;
		while (snext!=NULL && THEBIT(tmpChRelevant,snext->clas)==0) {
			snext = snext->next;
		}
		FILL_intlist(&snextbar, (snext!=NULL), nextbars)
		descendTheClassHierarchy(ff, s->clas, vFunCl, rrr, level+1, 
									 &snextbar, virtFlag, pass);
		s = snext;
	}
}

static int genThisClassHierarchy(int vApplCl, int oldvFunCl,
								 FILE *ff,
								 S_olSymbolsMenu   *rrr,
								 int virtFlag,
								 int pass) {
	S_fileItem 		*top,*tt,*fi;
	S_chReference 	*s;
	fi = s_fileTab.tab[vApplCl];
	if (fi==NULL) return(0);
	if (THEBIT(tmpChProcessed,vApplCl)) return(0);
	if (THEBIT(tmpChRelevant,vApplCl)==0) return(0);
	// check if you are at the top of a sub-hierarchy
	for(s=fi->sups; s!=NULL; s=s->next) {
		tt = s_fileTab.tab[s->clas];
		assert(tt);
		if (THEBIT(tmpChRelevant,s->clas) && THEBIT(tmpChProcessed,s->clas)==0) return(0);
	}
//&fprintf(dumpOut,"getting %s %s\n",top->b.isInterface?"interface":"class",top->name);fflush(dumpOut);
	// yes I am on the top, recursively descent and print all subclasses
	if (pass==FIRST_PASS && fi->b.isInterface) return(0);
	descendTheClassHierarchy(ff, vApplCl, oldvFunCl, rrr, 0, NULL, virtFlag, pass);
	return(1);
}

void genClassHierarchies( FILE *ff, S_olSymbolsMenu *rrr, 
										int virtFlag, int pass ) {
	S_olSymbolsMenu *ss;
	int i,rr;
	S_fileItem *fi;

	// mark the classes where the method is defined and used
	clearTmpChRelevant();
	for(ss=rrr; ss!=NULL; ss=ss->next) {
		assert(s_fileTab.tab[ss->s.vApplClass]);
		if (ss->visible) {
			SETBIT(tmpChRelevant, ss->s.vApplClass);
			SETBIT(tmpChRelevant, ss->s.vFunClass);
		}
	}
	// now, mark the relevant subtree of class tree
	clearTmpChMarkProcessed();
	for(ss=rrr; ss!=NULL; ss=ss->next) {
		markTransitiveRelevantSubs(ss->s.vFunClass, pass);
		markTransitiveRelevantSubs(ss->s.vApplClass, pass);
	}
	// and gen the class subhierarchy
	for(ss=rrr; ss!=NULL; ss=ss->next) {
		genThisClassHierarchy(ss->s.vFunClass, s_noneFileIndex,ff,rrr,virtFlag,pass);
		genThisClassHierarchy(ss->s.vApplClass, s_noneFileIndex,ff,rrr,virtFlag,pass);
	}
}

static int isInterface(int fnum) {
	S_fileItem *fi;
	fi = s_fileTab.tab[fnum];
	assert(fi!=NULL);
	return(fi->b.isInterface);
}

static int htmlRefItemsClassNameLess(S_olSymbolsMenu *ss1, 
									 S_olSymbolsMenu *ss2) {
	return(classHierarchyClassNameLess(ss1->s.vApplClass,ss2->s.vApplClass));
}

int chLineOrderLess(S_olSymbolsMenu *r1, S_olSymbolsMenu *r2) {
	return(r1->outOnLine < r2->outOnLine);
}

void olcxMenuGenGlobRefsForVirtMethod(FILE *ff, S_olSymbolsMenu *rrr) {
	char 			ln[MAX_HTML_REF_LEN];
	int 			virtFlag;
	linkNamePrettyPrint(ln,rrr->s.name,MAX_HTML_REF_LEN,SHORT_NAME);
	if (strcmp(rrr->s.name, LINK_NAME_CLASS_TREE_ITEM)==0) {
		/*&
		fprintf(ff, "\n");
		s_symbolListOutputCurrentLine += 1 ;
		&*/
	} else {
		if (s_opt.xref2) ppcGenRecord(PPC_VIRTUAL_SYMBOL, ln, "\n");
		else fprintf(ff, "\n== %s\n", ln);
		s_symbolListOutputCurrentLine += 2 ;
	}
	classHierarchyGenInit();
	setTmpClassBackPointersToMenu(rrr);
	genClassHierarchies( ff, rrr, SINGLE_VIRT_ITEM, FIRST_PASS);
//&fprintf(ff,"interfaces:\n");
	setTmpClassBackPointersToMenu(rrr);
	genClassHierarchies( ff, rrr, SINGLE_VIRT_ITEM, SECOND_PASS);
	//& if (! s_opt.xref2) fprintf(ff, "\n");
	//& s_symbolListOutputCurrentLine ++ ;
}

static void htmlMarkVisibleAllClassesHavingDefinition( S_olSymbolsMenu *rrr ) {
	S_olSymbolsMenu *rr;
	// put all classes having definition visible
	for(rr=rrr; rr!=NULL; rr=rr->next) {
		if (rr->s.vApplClass==rr->s.vFunClass) rr->visible = 1;
	}
}

static int isVirtualMenuItem(S_symbolRefItem *p) {
	return (p->b.storage == StorageField 
			|| p->b.storage == StorageMethod
			|| p->b.storage == StorageConstructor);
}

static void genVirtualsGlobRefLists(	S_olSymbolsMenu *rrr, 
										FILE *ff, 
										char *fn
	) {
	S_olSymbolsMenu 	*rr,*ss;
	int				i,olen,nlen,count,virtFlag;
	S_symbolRefItem *p;
	S_fileItem      *fi;
	// first count if there are some references at all
	count = 0;
	for(ss=rrr; ss!=NULL && ss->visible==0; ss=ss->next) ;
	if (ss == NULL) return;
	assert(rrr!=NULL);
	p = &rrr->s;
	assert(p!=NULL);
//&fprintf(dumpOut,"storage of %s == %s\n",p->name,storagesName[p->b.storage]);
	if (isVirtualMenuItem(p)) {
		if (s_opt.taskRegime==RegimeHtmlGenerate) {
			htmlMarkVisibleAllClassesHavingDefinition( rrr);
			htmlGenGlobRefsForVirtMethod( ff, fn, rrr);
		} else {
			olcxMenuGenGlobRefsForVirtMethod( ff, rrr);
		}
	}
}

static void genNonVirtualsGlobRefLists(	S_olSymbolsMenu *rrr, 
										FILE *ff, 
										char *fn
	) {
	S_olSymbolsMenu 	*rr,*ss;
	int				i,olen,nlen,count,virtFlag;
	S_symbolRefItem *p;
	S_fileItem      *fi;
	// first count if there are some references at all
	count = 0;
	for(ss=rrr; ss!=NULL && ss->visible==0; ss=ss->next) ;
	if (ss == NULL) return;
	assert(rrr!=NULL);
	p = &rrr->s;
	assert(p!=NULL);
//&fprintf(dumpOut,"storage of %s == %s\n",p->name,storagesName[p->b.storage]);
	if (! isVirtualMenuItem(p)) {
		for(ss=rrr; ss!=NULL; ss=ss->next) {
			p = &ss->s;
			if (s_opt.taskRegime==RegimeHtmlGenerate) {
				htmlGenNonVirtualGlobSymList( ff, fn, p);
			} else {
				olcxMenuGenNonVirtualGlobSymList( ff, ss);
			}
		}
	}
}

#if ZERO
void splitMenuPerSymbolsAndMap(S_olSymbolsMenu *rrr, 
							 void (*fun)(S_olSymbolsMenu *, void *, void *),
							 void *p1,
							 char *p2
	) {	
	S_olSymbolsMenu *rr, *ss, *nextl;
	rr = rrr;
	while (rr!=NULL) {
		for(ss= rr; 
			ss!=NULL&&ss->next!=NULL && itIsSameCxSymbol(&ss->next->s,&rr->s);
			ss = ss->next
			) ;
		assert(ss!=NULL);
		nextl = ss->next;
		ss->next = NULL;
		(*fun)(rr, p1, p2);
		LIST_APPEND(S_olSymbolsMenu, rr, nextl);
		rr = nextl;
	}
}
#else
void splitMenuPerSymbolsAndMap(S_olSymbolsMenu *rrr, 
							 void (*fun)(S_olSymbolsMenu *, void *, void *),
							 void *p1,
							 char *p2
	) {	
	S_olSymbolsMenu *rr, *mp, **ss, *cc, *all;
	S_symbolRefItem *cs;
	all = NULL;
	rr = rrr;
	while (rr!=NULL) {
		mp = NULL;
		ss= &rr; cs= &rr->s;
		while (*ss!=NULL) {
			cc = *ss;
			if (itIsSameCxSymbol(&cc->s, cs)) {
				// move cc it into map list
				*ss = (*ss)->next;
				cc->next = mp;
				mp = cc;
				goto contlab;
			}
			ss= &(*ss)->next;
		contlab:;
		}
		(*fun)(mp, p1, p2);
		// reconstruct the list in all
		LIST_APPEND(S_olSymbolsMenu, mp, all);
		all = mp;
	}
	// now find the original head and make it head, 
	// berk, TODO do this by passing pointer to pointer to rrr
	// as parameter
	if (all!=rrr) {
		ss = &all;
		while (*ss!=rrr && *ss!=NULL) ss = &(*ss)->next;
		assert(*ss!=NULL);
		assert (*ss != all);
		*ss = rrr->next;
		rrr->next = all;
	}
}
#endif

void htmlGenGlobRefLists(S_olSymbolsMenu *rrr, FILE *ff, char *fn) {
	S_olSymbolsMenu 	*rr,*ss,*nextl;
	int				i,n;
	static int 		counter;
	S_symbolRefItem *p;
	for(rr=rrr; rr!=NULL; rr=rr->next) rr->outOnLine = 0;
	s_symbolListOutputCurrentLine = 1;
	splitMenuPerSymbolsAndMap(
		rrr, (void (*)(S_olSymbolsMenu *, void *, void *))genNonVirtualsGlobRefLists, ff, fn
		);
	splitMenuPerSymbolsAndMap(
		rrr, (void (*)(S_olSymbolsMenu *, void *, void *))genVirtualsGlobRefLists, ff, fn
		);
}



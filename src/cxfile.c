/*
	$Revision: 1.16 $
	$Date: 2002/08/22 14:22:19 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "protocol.h"
//


/* *********************** INPUT/OUTPUT ************************** */

#define CXFI_FILE_FUMTIME 	'm'		/* last full update mtime for file item */
#define CXFI_FILE_UMTIME 	'p'		/* last update mtime for file item */
#define CXFI_FILE_INDEX 	'f'
#define CXFI_SOURCE_INDEX 	'o'		/* source index for java classes */
#define CXFI_SYM_INDEX 		's'
#define CXFI_USAGE			'u'							
#define CXFI_LINE_INDEX 	'l'
#define CXFI_COLL_INDEX 	'c'
#define CXFI_REFERENCE		'r'		/* using 'fsulc' */
#define	CXFI_INPUT_FROM_CL	'i'		/* file was introduced from comand line */
#define	CXFI_ACCESS_BITS	'a'		/* java access bit */
#define	CXFI_REQ_ACCESS		'A'		/* java reference required accessibilite index */
#define	CXFI_STORAGE		'g'		/* storaGe field */

#define CXFI_SUPER_CLASS	'h'							/* hore */
#define CXFI_INFER_CLASS	'd'							/* dole */
#define CXFI_CLASS_EXT		'e'		/* using 'fhd' */

#define CXFI_MACRO_BASE_FILE 'b'	/* ref to a file invoking macro */

#define CXFI_SYM_TYPE 		't'

#define CXFI_SYM_NAME 		'/'		/* using 'atdhg' -> 's' 			*/
#define CXFI_CLASS_NAME		'+'		/* 				 -> 'h' info 	*/
#define CXFI_FILE_NAME		':'		/* 				 -> 'ifm' info 	*/

#define CXFI_CHECK_NUMBER	'k'
#define CXFI_REFNUM			'n'
#define CXFI_VERSION		'v'
#define CXFI_SINGLE_RECORDS '@'
#define CXFI_REMARK			'#'

static int s_cxGeneratedSingleRecords[] = {
	CXFI_FILE_FUMTIME,
	CXFI_FILE_UMTIME,
	CXFI_FILE_INDEX,
	CXFI_SOURCE_INDEX,
	CXFI_SYM_TYPE,
	CXFI_USAGE,
	CXFI_LINE_INDEX,
	CXFI_COLL_INDEX,
	CXFI_SYM_INDEX,
	CXFI_REFERENCE,
	CXFI_SUPER_CLASS,
	CXFI_INFER_CLASS,
	CXFI_CLASS_EXT,
	CXFI_INPUT_FROM_CL,
	CXFI_MACRO_BASE_FILE,
	CXFI_REFNUM,
	CXFI_ACCESS_BITS,
	CXFI_REQ_ACCESS,
	CXFI_STORAGE,
	CXFI_CHECK_NUMBER,
	-1
};

#define MAX_CX_SYMBOL_TAB 2

struct lastCxFileInfos {
	int					onLineReferencedSym;
	S_olSymbolsMenu		*onLineRefMenuItem;
	int					onLineRefIsBestMatchFlag; // vyhodit ?
	S_symbolRefItem		*symbolTab[MAX_CX_SYMBOL_TAB];
	char				symbolIsWritten[MAX_CX_SYMBOL_TAB];
	char				*symbolBestMatchFlag[MAX_CX_SYMBOL_TAB];
	int					macroBaseFileGeneratedForSym[MAX_CX_SYMBOL_TAB];
	char				singleRecord[MAX_CHARS];
	int					counter[MAX_CHARS];
	void 				(*fun[MAX_CHARS])(int size,int ri,char **ccc,char **ffin,S_charBuf *bbb, int additional);
	int					additional[MAX_CHARS];

	// dead code detection vars
	int					symbolToCheckedForDeadness;
	char				deadSymbolIsDefined;

	// following item can be used only via symbolTab, 
	// it is just to smplifie memoru handling !!!!!!!!!!!!!!!!
	S_symbolRefItem		_symbolTab[MAX_CX_SYMBOL_TAB];
	char				_symbolTabNames[MAX_CX_SYMBOL_TAB][MAX_CX_SYMBOL_SIZE];
};

static struct lastCxFileInfos s_inLastInfos;
static struct lastCxFileInfos s_outLastInfos;


static S_charBuf cxfBuf;

static unsigned s_decodeFilesNum[MAX_FILES];

static char tmpFileName[MAX_FILE_NAME_SIZE];



/* *********************** INPUT/OUTPUT ************************** */

int cxFileHashNumber(char *sym) {
	register unsigned	res,r;
	register char		*ss,*bb;
	register int		c;
	if (s_opt.refnum <= 1) return(0);
	if (s_opt.xfileHashingMethod == XFILE_HASH_DEFAULT) {
	  res = 0;
	  ss = sym; 
	  while ((c = *ss)) {
		if (c == '(') break;
		SYM_TAB_HASH_FUN_INC(res, c);
		if (LINK_NAME_MAYBE_START(c)) res = 0;
		ss++;
	  }
	  SYM_TAB_HASH_FUN_FINAL(res);
	  res %= s_opt.refnum;
	  return(res);
	} else if (s_opt.xfileHashingMethod == XFILE_HASH_ALPHA1) {
	  assert(s_opt.refnum == XFILE_HASH_ALPHA1_REFNUM);
	  for(ss = bb = sym; *ss && *ss!='('; ss++) {
		c = *ss;
		if (LINK_NAME_MAYBE_START(c)) bb = ss+1;
	  }
	  c = *bb;
	  c = tolower(c);
	  if (c>='a' && c<='z') res = c-'a';
	  else res = ('z'-'a')+1;
	  assert(res>=0 && res<s_opt.refnum);
	  return(res);
	} else if (s_opt.xfileHashingMethod == XFILE_HASH_ALPHA2) {
	  for(ss = bb = sym; *ss && *ss!='('; ss++) {
		c = *ss;
		if (LINK_NAME_MAYBE_START(c)) bb = ss+1;
	  }
	  c = *bb;
	  c = tolower(c);
	  if (c>='a' && c<='z') r = c-'a';
	  else r = ('z'-'a')+1;
	  if (c==0) res=0;
	  else {
		c = *(bb+1);
		c = tolower(c);
		if (c>='a' && c<='z') res = c-'a';
		else res = ('z'-'a')+1;
	  }
	  res = r*XFILE_HASH_ALPHA1_REFNUM + res;
	  assert(res>=0 && res<s_opt.refnum);
	  return(res);
	} else {
	  assert(0);
	  return(0);
	}
}

static int searchSingleStringEqual(char *s, char *c) {
	while (*s!=0 && *s!=' ' && *s!='\t' && tolower(*s)==tolower(*c)) {
		c++; s++;
	}
	if (*s==0 || *s==' ' || *s=='\t') return(1);
	return(0);
}

static int searchSingleStringFitness(char *cxtag, char *searchedStr, int len) {
	char *cc,*s,*c,*tt;
	int i,pilotc;
	assert(searchedStr);
	pilotc = tolower(*searchedStr);
	if (pilotc == '^') {
		// check for exact prefix
		return(searchSingleStringEqual(searchedStr+1, cxtag));
	} else {
		cc = cxtag;
		for(cc=cxtag, i=0; *cc && i<len; cc++,i++) {
			if (searchSingleStringEqual(searchedStr, cc)) return(1);
		}
	}
 fini0:
	return(0);
}

int searchStringNonWildCharactersFitness(char *cxtag, int len) {
	char 	*ss;
	int		r;
	ss = s_opt.olcxSearchString;
	while (*ss) {
		while (*ss==' ' || *ss=='\t') ss++;
		if (*ss == 0) goto fini1;
		r = searchSingleStringFitness(cxtag, ss, len);
		if (r==0) return(0);
		while (*ss!=0 && *ss!=' ' && *ss!='\t') ss++;
	}
 fini1:
	return(1);
}

int searchStringFitness(char *cxtag, int len) {
	if (s_wildCharSearch) return(shellMatch(cxtag, len, s_opt.olcxSearchString, 0));
	else return(searchStringNonWildCharactersFitness(cxtag, len));
}


#define SET_MAX(max,val) {\
	if (val > max) max = val;\
}

char *crTagSearchLineStatic(char *name, S_position *p, 
							int *len1, int *len2, int *len3) {
	static char res[COMPLETION_STRING_SIZE];
	char type[TMP_STRING_SIZE];
	char file[TMP_STRING_SIZE];
	char dir[TMP_STRING_SIZE];
	char *ffname;
	int l1,l2,l3,fl, dl;
	l1 = l2 = l3 = 0;
	l1 = strlen(name);

	/*&
	type[0]=0;
	if (symType != TypeDefault) {
		l2 = strmcpy(type,typesName[symType]+4) - type;
	}
	&*/

	ffname = s_fileTab.tab[p->file]->name;
	assert(ffname);
	ffname = getRealFileNameStatic(ffname);
	fl = strlen(ffname);
	l3 = strmcpy(file,simpleFileName(ffname)) - file;

	dl = fl /*& - l3 &*/ ;
	strncpy(dir, ffname, dl);
	dir[dl]=0;

	SET_MAX(*len1, l1);
	SET_MAX(*len2, l2);
	SET_MAX(*len3, l3);

	if (s_opt.tagSearchSpecif == TSS_SEARCH_DEFS_ONLY_SHORT
		|| s_opt.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
		sprintf(res, "%s", name);
	} else {
		sprintf(res, "%-*s :%-*s :%s", *len1, name, *len3, file, dir);
	}
	return(res);
}

// filter out symbols which polluating search reports
int symbolNameShouldBeHiddenFromReports(char *name) {
	int 		nlen;
	char 		*s;
	nlen = strlen(name);
	// commons symbols
	if (name[0] == ' ') return(1);	// internal xref symbol

	// only java specific pollution
	if (! LANGUAGE(LAN_JAVA)) return(0);

	// class$ fields
	if (strncmp(name, "class$", 6)==0) return(1);

	// filter anonymous classes 
	//&if (s_opt.tagSearchSpecif==TSS_FULL_SEARCH || s_opt.tagSearchSpecif==TSS_SEARCH_DEFS_ONLY_SHORT) {
		// anonymous classes
		if (isdigit(name[0])) return(1);
		s = name;
		while ((s=strchr(s, '$'))!=NULL) {
			s++;
			while (isdigit(*s)) s++;
			if (*s == '.' || *s=='(' || *s=='$' || *s==0) return(1);
		}
	//&}

	return(0);
}

void searchSymbolCheckReference(S_symbolRefItem  *ss, S_reference *rr) {
	char ssname[MAX_CX_SYMBOL_SIZE];
	char *s, *sname;
	int i, slen;

	if (ss->b.symType == TypeCppInclude) return;   // no %%i symbols
	if (symbolNameShouldBeHiddenFromReports(ss->name)) return;

	linkNamePrettyPrint(ssname, ss->name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
	sname = ssname;
	slen = strlen(sname);
	// if completing without profile, cut profile
	if (s_opt.tagSearchSpecif==TSS_SEARCH_DEFS_ONLY_SHORT
		|| s_opt.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
		s = strchr(sname, '(');
		if (s!=NULL) *s = 0;
	}
	// cut package name for checking
	do {
		s = strchr(sname, '.');
		if (s!=NULL) sname = s+1;
	} while (s!=NULL);
	slen = strlen(sname);
	if (searchStringFitness(sname, slen)) {
		static int count = 0;
		//& olCompletionListPrepend(sname, NULL, NULL, 0, NULL, NULL, rr, ss->vFunClass, s_olcxCurrentUser->retrieverStack.top);
//&sprintf(tmpBuff,"adding %s of %s(%d) matched %s %d", sname, s_fileTab.tab[rr->p.file]->name, rr->p.file, s_opt.olcxSearchString, s_wildCharSearch);ppcGenTmpBuff();
		olCompletionListPrepend(sname, NULL, NULL, 0, NULL, ss, rr, ss->b.symType, ss->vFunClass, s_olcxCurrentUser->retrieverStack.top);
		// this is a hack for memory reduction
		// compact completions from time to time
		count ++;
		if (count > COMPACT_TAG_SEARCH_AFTER) {
			tagSearchCompactShortResults();
			count = 0;
		}
	}
}

/* ************************* WRITE **************************** */

/* Those two are macros for efficiency of small updates */

#if ZERO
#define GenDecimal(info) {\
	static char _ttt_[TMP_STRING_SIZE]= {0,};\
	char *_d_;\
	int _n_;\
	_n_ = info;\
	assert(_n_>0);\
	_d_ = _ttt_+TMP_STRING_SIZE-1;\
	while (_n_>=10) {\
		*(--_d_) = _n_%10 + '0';\
		_n_ = _n_/10;\
	}\
	*(--_d_) = _n_ + '0';\
	assert(_d_>=_ttt_);\
	fputs(_d_, cxOut);\
}
#endif

#define GenCompactRecord(recNum, info, blankPrefix) {\
	assert(recNum >= 0 && recNum < MAX_CHARS);\
	assert(info >= 0);\
	if (*blankPrefix!=0) fputs(blankPrefix, cxOut);\
	if (info != 0) fPutDecimal(cxOut, info);\
	fputc(recNum, cxOut);\
	s_outLastInfos.counter[recNum] = info;\
}

#define GenOptCompactRecord(recNum, info, blankPrefix) {\
	assert(recNum >= 0 && recNum < MAX_CHARS);\
	if (*blankPrefix!=0) fputs(blankPrefix, cxOut);\
	if (s_outLastInfos.counter[recNum] != info) {\
		if (info != 0) fPutDecimal(cxOut, info);\
		fputc(recNum, cxOut);\
		s_outLastInfos.counter[recNum] = info;\
	}\
}

static void genStringRecord(int recNum, char *s, char *blankPrefix) {
	int rsize;
	rsize = strlen(s)+1;
	if (*blankPrefix!=0) fputs(blankPrefix, cxOut);
	fPutDecimal(cxOut, rsize);
	fputc(recNum, cxOut);
	fputs(s, cxOut);
}


static void genSymbolItem(int symIndex) {
	char ttt[TMP_STRING_SIZE];
	S_symbolRefItem *d;
	GenOptCompactRecord(CXFI_SYM_INDEX, symIndex, "");
	d = s_outLastInfos.symbolTab[symIndex];
	GenOptCompactRecord(CXFI_SYM_TYPE, d->b.symType, "\n");
	GenOptCompactRecord(CXFI_INFER_CLASS, d->vApplClass, "");
	GenOptCompactRecord(CXFI_SUPER_CLASS, d->vFunClass, "");
	GenOptCompactRecord(CXFI_ACCESS_BITS, d->b.accessFlags, "");
	GenOptCompactRecord(CXFI_STORAGE, d->b.storage, "");
	s_outLastInfos.macroBaseFileGeneratedForSym[symIndex] = 0;
	s_outLastInfos.symbolIsWritten[symIndex] = 1;
	if (s_opt.long_cxref) {
		sprintf(ttt,"\t%s",typesName[d->b.symType]);
		genStringRecord(CXFI_REMARK,ttt,"");
	}
	genStringRecord(CXFI_SYM_NAME, d->name, "\t");
	if (s_opt.long_cxref) {
		if (d->vApplClass != s_noneFileIndex) {
			sprintf(ttt,"\ton %s",s_fileTab.tab[d->vApplClass]->name);
			genStringRecord(CXFI_REMARK,ttt,"\n");
		}
	}
	if (s_opt.long_cxref) {
		if (d->vApplClass != s_noneFileIndex) {
			sprintf(ttt,"\tfun %s",s_fileTab.tab[d->vFunClass]->name);
			genStringRecord(CXFI_REMARK,ttt,"\n");
		}
	}
	//& fprintf(cxOut,"\n\n");
	//fprintf(cxOut,"\t");
	fputc('\t', cxOut);
}

#define GenOptSymbolItem(symIndex) {\
	if (! s_outLastInfos.symbolIsWritten[symIndex]) {\
		genSymbolItem(symIndex);\
	}\
}

#define GenCxReferenceBase(symbolNum, usage, requiredAccess, file, line, coll) {\
	char ttt[TMP_STRING_SIZE];\
	GenOptSymbolItem(symbolNum);\
	if (usage == UsageMacroBaseFileUsage) {\
		/* optimize the number of those references to 1*/\
		assert(symbolNum>=0 && symbolNum<MAX_CX_SYMBOL_TAB);\
		if (s_outLastInfos.macroBaseFileGeneratedForSym[symbolNum]) return;\
		s_outLastInfos.macroBaseFileGeneratedForSym[symbolNum] = 1;\
	}\
	GenOptCompactRecord(CXFI_USAGE, usage, "");\
	GenOptCompactRecord(CXFI_REQ_ACCESS, requiredAccess, "");\
	GenOptCompactRecord(CXFI_SYM_INDEX, symbolNum, "");\
	GenOptCompactRecord(CXFI_FILE_INDEX, file, "");\
	GenOptCompactRecord(CXFI_LINE_INDEX, line, "");\
	GenOptCompactRecord(CXFI_COLL_INDEX, coll, "");\
	GenCompactRecord(CXFI_REFERENCE, 0, "");\
	if (s_opt.long_cxref) {\
		sprintf(ttt,"\t%-7s in %30s:%u:%d\n",usagesName[usage]+5,\
			s_fileTab.tab[file]->name,line,coll);\
		genStringRecord(CXFI_REMARK,ttt,"");\
	}\
}

static void genCxReference(S_reference *rr, int symbolNum) {
	GenCxReferenceBase(symbolNum, rr->usg.base, rr->usg.requiredAccess, rr->p.file, rr->p.line, rr->p.coll);
}

static void genSubClassInfo(int sup, int inf, int origin) {
	char ttt[TMP_STRING_SIZE];
	GenOptCompactRecord(CXFI_FILE_INDEX, origin, "\n");
	GenOptCompactRecord(CXFI_SUPER_CLASS, sup, "");
	GenOptCompactRecord(CXFI_INFER_CLASS, inf, "");
	GenCompactRecord(CXFI_CLASS_EXT, 0, "");
	if (s_opt.long_cxref) {
		sprintf(ttt,"\t\t%s",s_fileTab.tab[inf]->name);
		genStringRecord(CXFI_REMARK,ttt,"\n");
		sprintf(ttt,"  extends\t%s",s_fileTab.tab[sup]->name);
		genStringRecord(CXFI_REMARK,ttt,"\n");
	}
}

static void genFileIndexItem(struct fileItem *fi, int ii) {
	GenOptCompactRecord(CXFI_FILE_INDEX, ii, "\n");
	GenOptCompactRecord(CXFI_FILE_UMTIME, fi->lastUpdateMtime, " ");
	GenOptCompactRecord(CXFI_FILE_FUMTIME, fi->lastFullUpdateMtime, " ");
	GenOptCompactRecord(CXFI_INPUT_FROM_CL, fi->b.commandLineEntered, "");
	if (fi->b.isInterface) {
		GenOptCompactRecord(CXFI_ACCESS_BITS, ACC_INTERFACE, "");
	} else {
		GenOptCompactRecord(CXFI_ACCESS_BITS, ACC_DEFAULT, "");
	}
	genStringRecord(CXFI_FILE_NAME, fi->name, " ");
}

static void genFileSourceIndexItem(struct fileItem *fi, int ii) {
	if (fi->b.sourceFile != -1 && fi->b.sourceFile != s_noneFileIndex) {
		GenOptCompactRecord(CXFI_FILE_INDEX, ii, "\n");
		GenCompactRecord(CXFI_SOURCE_INDEX, fi->b.sourceFile, " ");
	}
}

static void genClassHierarchyItems(struct fileItem *fi, int ii) {
	S_chReference *p;
	for(p=fi->sups; p!=NULL; p=p->next) {
		genSubClassInfo(p->clas, ii, p->ofile);
	}
}

static void crSubClassInfo(int sup, int inf, int origin, int genfl) {
	S_fileItem		*ii,*jj;
	S_chReference	*p,*pp;
	int				mm;
	ii = s_fileTab.tab[inf];
	jj = s_fileTab.tab[sup];
	assert(ii && jj);
	for(p=ii->sups; p!=NULL && p->clas!=sup; p=p->next) ;
	if (p==NULL) {
		CX_ALLOC(p, S_chReference);
		FILL_chReference(p, origin, sup, ii->sups);
		ii->sups = p;
		assert(s_opt.taskRegime);
		if (s_opt.taskRegime == RegimeXref) {
			if (genfl == CX_FILE_ITEM_GEN) genSubClassInfo(sup, inf, origin);
		}
		CX_ALLOC(pp, S_chReference);
		FILL_chReference(pp, origin, inf, jj->infs);
		jj->infs = pp;
	}
}

void addSubClassItemToFileTab( int sup, int inf, int origin) {
	if (sup >= 0 && inf >= 0) {
		crSubClassInfo(sup, inf, origin, NO_CX_FILE_ITEM_GEN);
	}
}


void addSubClassesItemsToFileTab(S_symbol *ss, int origin) {
	int i,cf1;
	S_symbolList *sups;
	if (ss->b.symType != TypeStruct) return;
/*fprintf(dumpOut,"testing %s\n",ss->name);*/
	assert(ss->b.javaFileLoaded);
	if (ss->b.javaFileLoaded == 0) return;
	cf1 = ss->u.s->classFile;
	assert(cf1 >= 0 &&  cf1 < MAX_FILES);
/*fprintf(dumpOut,"loaded: #sups == %d\n",ns);*/
	for(sups=ss->u.s->super; sups!=NULL; sups=sups->next) {
		assert(sups->d && sups->d->b.symType == TypeStruct);
		addSubClassItemToFileTab( sups->d->u.s->classFile, cf1, origin);
	}
}

/* *************************************************************** */

static void genRefItem0(S_symbolRefItem *d, int forceGen) {
	S_reference *rr;
	int symIndex;
//&fprintf(dumpOut,"function %s\n", d->name);
	symIndex = 0;
	assert(strlen(d->name)+1 < MAX_CX_SYMBOL_SIZE);
	strcpy(s_outLastInfos._symbolTabNames[symIndex], d->name);
	FILL_symbolRefItemBits(&s_outLastInfos._symbolTab[symIndex].b,
						   d->b.symType, d->b.storage,
						   d->b.scope,d->b.accessFlags, d->b.category,0);
	FILL_symbolRefItem(&s_outLastInfos._symbolTab[symIndex],
					   s_outLastInfos._symbolTabNames[symIndex], 
					   d->fileHash, // useless put 0
					   d->vApplClass, d->vFunClass,
					   s_outLastInfos._symbolTab[symIndex].b,NULL,NULL);
	s_outLastInfos.symbolTab[symIndex] = &s_outLastInfos._symbolTab[symIndex];
	s_outLastInfos.symbolIsWritten[symIndex] = 0;
	if ( d->b.category == CatLocal) return;
	if ( d->refs == NULL && ! forceGen) return; 
	for(rr = d->refs; rr!=NULL; rr=rr->next) {
//&fprintf(ccOut,"checking ref %d --< %s:%d\n", s_fileTab.tab[rr->p.file]->b.cxLoading, s_fileTab.tab[rr->p.file]->name, rr->p.line);
		if (s_opt.update==UP_NO_UPDATE || s_fileTab.tab[rr->p.file]->b.cxLoading) {
			/* ?? s_opt.update==UP_NO_UPDATE, why it is there */
			genCxReference(rr, symIndex);
		}	
	}
	//&fflush(cxOut);
}

static void genRefItem(S_symbolRefItem *dd) {
	genRefItem0(dd,0);
#if ZERO // original
	S_symbolRefItem *d;
	/* why this is made like this ???, meaning it recollects all */
	/* from the list, however it makes complexity O(n^2)         */
	for(d=dd; d!=NULL; d=d->next) {
		if (REF_ELEM_EQUAL(d,dd)) genRefItem0(d,0);
	}
#endif
}

#define GET_VERSION_STRING(ttt) {\
	sprintf(ttt," file format: C-xrefactory %s ",C_XREF_FILE_VERSION_NUMBER);\
}

#define COMPOSE_CXFI_CHECK_NUM(filen,hashMethod,exactPositionLinkFlag) (\
	((filen)*XFILE_HASH_MAX+hashMethod)*2+exactPositionLinkFlag\
)
#define DECOMPOSE_CXFI_CHECK_NUM(num,filen,hashMethod,exactPositionLinkFlag){\
	unsigned tmp;\
    tmp = num;\
	exactPositionLinkFlag = tmp % 2;\
	tmp = tmp / 2;\
	hashMethod = tmp % XFILE_HASH_MAX;\
	tmp = tmp / XFILE_HASH_MAX;\
	filen = tmp;\
}

static void genCxFileHead() {
	char sr[MAX_CHARS];
	char ttt[TMP_STRING_SIZE];
	int i, magicnum;
	memset(&s_outLastInfos, 0, sizeof(s_outLastInfos));
	for(i=0; i<MAX_CHARS; i++) {
		s_outLastInfos.counter[i] = -1;
	}
	GET_VERSION_STRING(ttt);
	genStringRecord(CXFI_VERSION, ttt, "\n\n");
	fprintf(cxOut,"\n\n\n");
	for(i=0; i<MAX_CHARS && s_cxGeneratedSingleRecords[i] != -1; i++) {
		sr[i] = s_cxGeneratedSingleRecords[i];
	}
	assert(i < MAX_CHARS);
	sr[i]=0;
	genStringRecord(CXFI_SINGLE_RECORDS, sr, "");
	GenCompactRecord(CXFI_REFNUM, s_opt.refnum, " ");
	GenCompactRecord(CXFI_CHECK_NUMBER, COMPOSE_CXFI_CHECK_NUM(
		MAX_FILES, 
		s_opt.xfileHashingMethod,
		s_opt.exactPositionResolve
		), " ");
}

static void openInOutReferenceFiles(int updateFlag, char *fname) {
	char *ttt;
	tmpFileName[0] = 0;
	if (updateFlag) {
		ttt = crTmpFileName_st();
		assert(strlen(ttt)+1 < MAX_FILE_NAME_SIZE);
		strcpy(tmpFileName, ttt);
		copyFile(fname, tmpFileName);
	}
	assert(fname);
	cxOut = fopen(fname,"w");
	if (cxOut == NULL) fatalError(ERR_CANT_OPEN, fname, XREF_EXIT_ERR);
	if (updateFlag) {
		fIn = fopen(tmpFileName,"r");
		if (fIn==NULL) warning(ERR_CANT_OPEN,tmpFileName);
	} else {
		fIn = NULL;
	}
}

static void referenceFileEnd(int updateFlag, char *fname) {
	if (fIn != NULL) {
		fclose(fIn);
		fIn = NULL;
		removeFile(tmpFileName);
	}
	fclose(cxOut);
	cxOut = stdout;
}

static void createDirIfNotExists(char *dirname) {
	struct stat st;
	int createFlag;
	createFlag = 1;
	if (stat(dirname, &st)==0) {
		if ((st.st_mode & S_IFMT) == S_IFDIR) return;
		removeFile(dirname);
	}
	createDir(dirname);
}

/* fnamesuff contains '/' at the beginning */

static void genPartialFileTabRefFile(	int updateFlag, 
										char *dirname, 
										char *fnamesuff,
										void mapfun(S_fileItem *, int),
										void mapfun2(S_fileItem *, int)
							) {
	char fn[MAX_FILE_NAME_SIZE];
	sprintf(fn, "%s%s", dirname, fnamesuff);
	InternalCheck(strlen(fn) < MAX_FILE_NAME_SIZE-1);
	openInOutReferenceFiles(updateFlag, fn);
	genCxFileHead();
	idTabMap3(&s_fileTab, mapfun);
	if (mapfun2!=NULL) idTabMap3(&s_fileTab, mapfun2);
	scanCxFile(s_cxFullScanFunTab);
	referenceFileEnd(updateFlag, fn);		
}

#if ZERO
static void genPartialRefItem(S_symbolRefItem *dd, int fileOrder) {
	if (dd->b.category == CatLocal) return;
	if (dd->refs == NULL) return; 
//&	assert(cxFileHashNumber(dd->name) == dd->fileHash);
	if (dd->fileHash == fileOrder) genRefItem0(dd,0);
#if ZERO // original
	S_symbolRefItem *d;
	for(d=dd; d!=NULL; d=d->next) {
		if (cxFileHashNumber(d->name) == fileOrder) {
			if (REF_ELEM_EQUAL(d,dd)) {		/* what does this test mean ? */
				genRefItem0(d,0);
			}
		}
	}
#endif
}
#endif

static void generateRefsFromMemory(int fileOrder) {
	int 				i;
	int 				tsize;
	S_symbolRefItem	*pp;
	tsize = s_cxrefTab.size;
	for(i=0; i<tsize; i++) {
		for(pp=s_cxrefTab.tab[i]; pp!=NULL; pp=pp->next) {
			if (pp->b.category == CatLocal) goto nextitem;
			if (pp->refs == NULL) goto nextitem;
			if (pp->fileHash == fileOrder) genRefItem0(pp,0);
		nextitem:;
		}
	}
}

void genReferenceFile(int updateFlag, char *fname) {
	char 	fn[MAX_FILE_NAME_SIZE];
	char 	*dirname;
	int		i;
	if (updateFlag==0) removeFile(fname);
	recursivelyCreateFileDirIfNotExists(fname);
	if (s_opt.refnum <= 1) {
		/* single reference file */
		openInOutReferenceFiles(updateFlag, fname);
/*&		idTabMap(&s_fileTab, javaInitSubClassInfo);&*/
		genCxFileHead();
		idTabMap3(&s_fileTab, genFileIndexItem);
		idTabMap3(&s_fileTab, genFileSourceIndexItem);
		idTabMap3(&s_fileTab, genClassHierarchyItems);
		scanCxFile(s_cxFullScanFunTab);
		refTabMap(&s_cxrefTab, genRefItem);
		referenceFileEnd(updateFlag, fname);
	} else {
		/* several reference files */
		dirname = fname;
		createDirIfNotExists(dirname);
		genPartialFileTabRefFile(updateFlag,dirname,PRF_FILES,
						  genFileIndexItem, genFileSourceIndexItem);
		genPartialFileTabRefFile(updateFlag,dirname,PRF_CLASS,
						  genClassHierarchyItems, NULL);
		for (i=0; i<s_opt.refnum; i++) {
			sprintf(fn, "%s%s%04d", dirname, PRF_REF_PREFIX, i);
			InternalCheck(strlen(fn) < MAX_FILE_NAME_SIZE-1);
			openInOutReferenceFiles(updateFlag, fn);
			genCxFileHead();
			scanCxFile(s_cxFullScanFunTab);
			//&refTabMap4(&s_cxrefTab, genPartialRefItem, i);
			generateRefsFromMemory(i);
			referenceFileEnd(updateFlag, fn);
		}
	}
}

/* ************************* READ **************************** */

#define GetChar(cch, ccc, ffin, bbb) {\
	if (ccc >= ffin) {\
		(bbb)->cc = ccc;\
		if ((bbb)->isAtEOF || getCharBuf(bbb) == 0) {\
			cch = -1;\
			(bbb)->isAtEOF = 1;\
		} else {\
			ccc = (bbb)->cc; ffin = (bbb)->fin;\
			cch = * ((unsigned char*)ccc); ccc ++;\
		}\
	} else {\
		cch = * ((unsigned char*)ccc); ccc++;\
	}\
/*fprintf(dumpOut,"getting char *%x < %x == '0x%x'\n",ccc,ffin,cch);fflush(dumpOut);*/\
}

#define ScanInt(cch, ccc, ffin, bbb, res) {\
	while (cch==' ' || cch=='\n' || cch=='\t') GetChar(cch,ccc,ffin,bbb);\
	res = 0;\
	while (isdigit(cch)) {\
		res = res*10 + cch-'0';\
		GetChar(cch,ccc,ffin,bbb);\
	}\
}

#define SkipNChars(count, ccc, ffin, iBuf) {\
	register int ccount,i,ch;\
	ccount = count;\
	while (ccc + ccount > ffin) {\
		ccount -= ffin - ccc;\
		ccc = ffin;\
		GetChar(ch, ccc, ffin, iBuf);\
		ccount --;\
	}\
	ccc += ccount;\
}

static void cxrfSetSingleRecords(	int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int additionalArg
							) {
	int i,cch;
	char *cc, *fin;
	cc = *ccc; fin = *ffin;
	assert(ri == CXFI_SINGLE_RECORDS);
	for(i=0; i<size-1; i++) {
		GetChar(cch, cc, fin, bbb);
		s_inLastInfos.singleRecord[cch] = 1;
	}
	*ccc = cc; *ffin = fin;
}

static void writeCxFileCompatibilityError(char *message) {
	static time_t lastMessageTime;
	if (s_opt.taskRegime == RegimeEditServer) {
		if (lastMessageTime < s_fileProcessStartTime) {
			error(ERR_ST, message);
			lastMessageTime = time(NULL);
		}
	} else {
		fatalError(ERR_ST, message, XREF_EXIT_ERR);
	}
}


static void cxrfVersionCheck(	int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int additionalArg
							) {
	char versionString[TMP_STRING_SIZE];
	char thisVersionString[TMP_STRING_SIZE];
	int i,cch;
	char *cc, *fin;
	cc = *ccc; fin = *ffin;
	assert(ri == CXFI_VERSION);
	for(i=0; i<size-1; i++) {
		GetChar(cch, cc, fin, bbb);
		versionString[i]=cch;
	}
	versionString[i]=0;
	GET_VERSION_STRING(thisVersionString);
	if (strcmp(versionString, thisVersionString)) {
		writeCxFileCompatibilityError("The Tag file was not generated by the current version of C-xrefactory, recreate it");
	}
	*ccc = cc; *ffin = fin;
}

static void cxrfCheckNumber(	int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int additionalArg
	) {
	int i,cch;
	int magicn, filen, hashMethod, exactPositionLinkFlag;
	char *cc, *fin;
	cc = *ccc; fin = *ffin;
	assert(ri == CXFI_CHECK_NUMBER);
	if (s_opt.create) return; // no check when creating new file
	magicn = s_inLastInfos.counter[CXFI_CHECK_NUMBER];
	DECOMPOSE_CXFI_CHECK_NUM(magicn,filen,hashMethod,exactPositionLinkFlag);
	if (filen != MAX_FILES) {
		sprintf(tmpBuff,"The Tag file was generated with different MAX_FILES, recreate it");
		writeCxFileCompatibilityError(tmpBuff);
	}
	if (hashMethod != s_opt.xfileHashingMethod) {
		sprintf(tmpBuff,"The Tag file was generated with different hash method, recreate it");
		writeCxFileCompatibilityError(tmpBuff);
	}
//&fprintf(dumpOut,"checking %d <-> %d\n", exactPositionLinkFlag, s_opt.exactPositionResolve);
	if (exactPositionLinkFlag != s_opt.exactPositionResolve) {
		if (exactPositionLinkFlag) {
			sprintf(tmpBuff,"The Tag file was generated with '-exactpositionresolve' flag, recreate it");
		} else {
			sprintf(tmpBuff,"The Tag file was generated without '-exactpositionresolve' flag, recreate it");
		}
		writeCxFileCompatibilityError(tmpBuff);
	}
	*ccc = cc; *ffin = fin;
}

static int cxrfFileItemShouldBeUpdatedFromCxFile(S_fileItem *ffi) {
	int updateFromCxFile = 1;
//fprintf(dumpOut, "!testing cx retake finfo from %s for %s\n", s_cxref_file_name, ffi->name);
	if (s_opt.taskRegime == RegimeXref) {
		if (ffi->b.cxLoading && ! ffi->b.cxSaved) {
			updateFromCxFile = 0;
		} else {
			updateFromCxFile = 1;
		}
	}
	if (s_opt.taskRegime == RegimeEditServer) {
//&fprintf(dumpOut, "!lit st == %d %d\n", ffi->lastInspect, s_fileProcessStartTime);
		// Hmm, should be there testing on both intervals
		// s_fileProcessStartTime < ffi->lastInspect < time(NULL) ????
		if (ffi->lastInspect < s_fileProcessStartTime) {
			updateFromCxFile = 1;
		} else {
			updateFromCxFile = 0;
		}
	}
//&if (updateFromCxFile) fprintf(dumpOut, "!cx retake finfo from %s for %s\n", s_opt.cxrefFileName, ffi->name);
	return(updateFromCxFile);
}

static void cxrfFileName(		int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int genFl
							) {
	char id[MAX_FILE_NAME_SIZE];
	char *fn;
	struct fileItem *ffi,tffi;
	int i,ii,dii,len,commandLineFlag,isInterface;
	time_t fumtime, umtime;
	char *cc, *fin, cch;
	assert(ri == CXFI_FILE_NAME);
	cc = *ccc; fin = *ffin;
	fumtime = (time_t) s_inLastInfos.counter[CXFI_FILE_FUMTIME];
	umtime = (time_t) s_inLastInfos.counter[CXFI_FILE_UMTIME];
	commandLineFlag = s_inLastInfos.counter[CXFI_INPUT_FROM_CL];
	isInterface=((s_inLastInfos.counter[CXFI_ACCESS_BITS] & ACC_INTERFACE)!=0);
	ii = s_inLastInfos.counter[CXFI_FILE_INDEX];
	for (i=0; i<size-1; i++) {
		GetChar(cch, cc, fin, bbb);
		id[i] = cch;
	}
	id[i] = 0;
	len = i;
	InternalCheck(len+1 < MAX_FILE_NAME_SIZE);
	InternalCheck(ii>=0 && ii<MAX_FILES);
 	FILLF_fileItem(&tffi, id, 0, 0,0, 0, 
				   0,0,0,commandLineFlag,0,0,0,0,0,s_noneFileIndex, 
				   NULL,NULL,s_noneFileIndex,NULL);
	if (! idTabIsMember(&s_fileTab,&tffi,&dii)) {
		addFileTabItem(id, &dii);
		ffi = s_fileTab.tab[dii];
		ffi->b.commandLineEntered = commandLineFlag;
		ffi->b.isInterface = isInterface;
		if (ffi->lastFullUpdateMtime == 0) ffi->lastFullUpdateMtime=fumtime;
		if (ffi->lastUpdateMtime == 0) ffi->lastUpdateMtime=umtime;
		//&if (fumtime>ffi->lastFullUpdateMtime) ffi->lastFullUpdateMtime=fumtime;
		//&if (umtime>ffi->lastUpdateMtime) ffi->lastUpdateMtime=umtime;
		assert(s_opt.taskRegime);
		if (s_opt.taskRegime == RegimeXref) {
			if (genFl == CX_GENERATE_OUTPUT) {
				genFileIndexItem(ffi, dii);
			}
		}
	} else {
		ffi = s_fileTab.tab[dii];
		if (cxrfFileItemShouldBeUpdatedFromCxFile(ffi)) {
			ffi->b.isInterface = isInterface;
			// put it to none, it will be updated by source item
			ffi->b.sourceFile = s_noneFileIndex;
		}
		if (s_opt.taskRegime == RegimeEditServer) {
			ffi->b.commandLineEntered = commandLineFlag;
		}
		if (ffi->lastFullUpdateMtime == 0) ffi->lastFullUpdateMtime=fumtime;
		if (ffi->lastUpdateMtime == 0) ffi->lastUpdateMtime=umtime;
		//&if (fumtime>ffi->lastFullUpdateMtime) ffi->lastFullUpdateMtime=fumtime;
		//&if (umtime>ffi->lastUpdateMtime) ffi->lastUpdateMtime=umtime;
	}
	ffi->b.isFromCxfile = 1;
	s_decodeFilesNum[ii]=dii;
/*&fprintf(dumpOut,"%d: '%s'\t scanned: added as %d\n",ii,id,dii);fflush(dumpOut);&*/
	*ccc = cc; *ffin = fin;
}

static void cxrfSourceIndex(	int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int genFl
							) {
	char *cc, *fin, cch;
	int file, sfile;
	assert(ri == CXFI_SOURCE_INDEX);
	cc = *ccc; fin = *ffin;
	file = s_inLastInfos.counter[CXFI_FILE_INDEX];
	file = s_decodeFilesNum[file];
	sfile = s_inLastInfos.counter[CXFI_SOURCE_INDEX];
	sfile = s_decodeFilesNum[sfile];
	assert(file>=0 && file<MAX_FILES && s_fileTab.tab[file]);
	// hmmm. here be more generous in getting corrct source info
	if (s_fileTab.tab[file]->b.sourceFile == s_noneFileIndex
		|| s_fileTab.tab[file]->b.sourceFile == -1) {
//&fprintf(dumpOut,"setting %d source to %d\n", file, sfile);fflush(dumpOut);
//&fprintf(dumpOut,"setting %s source to %s\n", s_fileTab.tab[file]->name, s_fileTab.tab[sfile]->name);fflush(dumpOut);	
		// first check that it is not set directly from source
		if (! s_fileTab.tab[file]->b.cxLoading) {
			s_fileTab.tab[file]->b.sourceFile = sfile;
		}
// I think that following was a bug
#if ZERO
		if (s_opt.taskRegime==RegimeXref && genFl==CX_GENERATE_OUTPUT) {
			genFileSourceIndexItem(s_fileTab.tab[file], file);
		}
#endif
	}
	*ccc = cc; *ffin = fin;
}

static int scanSymNameString(int size,char **ccc,char **ffin,
								 S_charBuf *bbb,char *id) {
	int i, len;
	char *cc, *fin, cch;
	cc = *ccc; fin = *ffin;
	for (i=0; i<size-1; i++) {
		GetChar(cch, cc, fin, bbb);
		id[i] = cch;
	}
	id[i] = 0;
	len = i;
	InternalCheck(len+1 < MAX_CX_SYMBOL_SIZE);
	*ccc = cc; *ffin = fin;
	return(len);
}


static void getSymTypeAndClasses(int *_symType, int *_vApplClass,
								  int *_vFunClass) {
	int symType, vApplClass, vFunClass;
	symType = s_inLastInfos.counter[CXFI_SYM_TYPE];
	vApplClass = s_inLastInfos.counter[CXFI_INFER_CLASS];
	vApplClass = s_decodeFilesNum[vApplClass];
	assert(s_fileTab.tab[vApplClass] != NULL);
	vFunClass = s_inLastInfos.counter[CXFI_SUPER_CLASS];
	vFunClass = s_decodeFilesNum[vFunClass];
	assert(s_fileTab.tab[vFunClass] != NULL);
	*_symType = symType;
	*_vApplClass = vApplClass;
	*_vFunClass = vFunClass;
}



static void cxrfSymbolNameForFullUpdateSchedule(	int size,
													int ri,
													char **ccc,
													char **ffin,
													S_charBuf *bbb,
													int additionalArg
	) {
	S_symbol *d;
	S_symbolRefItem *ddd,dd,*memb;
	int i,ii,si,symType,len,rr,vApplClass,vFunClass,accessFlags;
	int storage;
	char *id;
	char *fn,*ss;
	char *cc, *fin, cch;
	assert(ri == CXFI_SYM_NAME);
	accessFlags = s_inLastInfos.counter[CXFI_ACCESS_BITS];
	storage = s_inLastInfos.counter[CXFI_STORAGE];
	si = s_inLastInfos.counter[CXFI_SYM_INDEX];
	InternalCheck(si>=0 && si<MAX_CX_SYMBOL_TAB);
	id = s_inLastInfos._symbolTabNames[si];
	len = scanSymNameString( size, ccc, ffin, bbb, id);
	cc = *ccc; fin = *ffin;
	getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
//&fprintf(dumpOut,":scanning ref of %s %d %d: \n",id,symType,vFunClass);fflush(dumpOut);
	if (symType!=TypeCppInclude || strcmp(id, LINK_NAME_INCLUDE_REFS)!=0) {
		s_inLastInfos.onLineReferencedSym = -1;
		return;
	}
	ddd = &s_inLastInfos._symbolTab[si];
	s_inLastInfos.symbolTab[si] = ddd;
	FILL_symbolRefItemBits(&ddd->b,symType, storage,
						   ScopeGlobal,accessFlags,CatGlobal,0);
	FILL_symbolRefItem(ddd,id,
					   cxFileHashNumber(id), //useless, put 0
					   vApplClass,vFunClass,ddd->b,NULL,NULL);
	rr = refTabIsMember(&s_cxrefTab, ddd, &ii,&memb);
	if (rr == 0) {
		CX_ALLOCC(ss, len+1, char);
		strcpy(ss,id);
		CX_ALLOC(memb, S_symbolRefItem);
		FILL_symbolRefItemBits(&memb->b,symType, storage,
							   ScopeGlobal,accessFlags,CatGlobal,0);
		FILL_symbolRefItem(memb,ss, cxFileHashNumber(ss),
						   vApplClass,vFunClass,memb->b,NULL,NULL);
		refTabAdd(&s_cxrefTab, memb, &ii);
	}
	s_inLastInfos.symbolTab[si] = memb;
	s_inLastInfos.onLineReferencedSym = si;
	*ccc = cc; *ffin = fin;
}

static void cxfileCheckLastSymbolDeadness() {
	if (s_inLastInfos.symbolToCheckedForDeadness != -1
		&& s_inLastInfos.deadSymbolIsDefined) {
//&sprintf(tmpBuff,"adding %s storage==%s", s_inLastInfos.symbolTab[s_inLastInfos.symbolToCheckedForDeadness]->name, storagesName[s_inLastInfos.symbolTab[s_inLastInfos.symbolToCheckedForDeadness]->b.storage]);ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
		olAddBrowsedSymbol(s_inLastInfos.symbolTab[s_inLastInfos.symbolToCheckedForDeadness],
						   &s_olcxCurrentUser->browserStack.top->hkSelectedSym,
						   1,1,0,UsageDefined,0, &s_noPos, UsageDefined);
	}
}


static int symbolIsReportableAsDead(S_symbolRefItem *ss) {
	if (ss==NULL) return(0);
	if (ss->name[0]==' ') return(0);
	// you need to be strong here, in fact struct record can be used
	// without using struct explicitly
	if (ss->b.symType == TypeStruct) return(0);
	// maybe I should collect also all toString() referebces?
	if (ss->b.storage==StorageMethod && strcmp(ss->name,"toString()")==0) return(0);

	// in this first approach restrict this to variables and functions
	if (ss->b.symType == TypeMacro) return(0);
	return(1);
}

static void cxrfSymbolName(		int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int additionalArg
	) {
	S_symbol *d;
	S_symbolRefItem *ddd,dd,*memb;
	S_olSymbolsMenu *cms;
	int i,ii,si,symType,len,rr,vApplClass,vFunClass,ols,accessFlags,storage;
	char *id;
	char *fn,*ss;
	char *cc, *fin, cch;
	assert(ri == CXFI_SYM_NAME);
	if (s_opt.taskRegime==RegimeEditServer && additionalArg==DEAD_CODE_DETECTION) {
		// check if previous symbol was dead
		cxfileCheckLastSymbolDeadness();
	}
	accessFlags = s_inLastInfos.counter[CXFI_ACCESS_BITS];
	storage = s_inLastInfos.counter[CXFI_STORAGE];
	si = s_inLastInfos.counter[CXFI_SYM_INDEX];
	InternalCheck(si>=0 && si<MAX_CX_SYMBOL_TAB);
	id = s_inLastInfos._symbolTabNames[si];
	len = scanSymNameString( size, ccc, ffin, bbb, id);
	cc = *ccc; fin = *ffin;
	getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
/*fprintf(dumpOut,":scanning ref of %s %d %d: \n",id,symType,virtClass);fflush(dumpOut);*/
	ddd = &s_inLastInfos._symbolTab[si];
	s_inLastInfos.symbolTab[si] = ddd;
	FILL_symbolRefItemBits(&ddd->b,symType, storage,
						   ScopeGlobal,accessFlags, CatGlobal,0);
	FILL_symbolRefItem(ddd,id,
					   cxFileHashNumber(id), // useless put 0
					   vApplClass,vFunClass,ddd->b,NULL,NULL);
	rr = refTabIsMember(&s_cxrefTab, ddd, &ii,&memb);
	while (rr && memb->b.category!=CatGlobal) rr=refTabNextMember(ddd, &memb);
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime == RegimeHtmlGenerate) {
		if (memb==NULL) {
			CX_ALLOCC(ss, len+1, char);
			strcpy(ss,id);
			CX_ALLOC(memb, S_symbolRefItem);
			FILL_symbolRefItemBits(&memb->b,symType, storage,
								   ScopeGlobal, accessFlags, CatGlobal,0);
			FILL_symbolRefItem(memb,ss,cxFileHashNumber(ss),
							   vApplClass,vFunClass,memb->b,NULL,NULL);
			refTabAdd(&s_cxrefTab,memb, &ii);
		}
		s_inLastInfos.symbolTab[si] = memb;
	}
	if (s_opt.taskRegime == RegimeXref) {
		if (memb==NULL) memb=ddd;
		genRefItem0(memb,1);
		ddd->refs = memb->refs; // note references to not generate multiple
		memb->refs = NULL;  	// HACK, remove them, to not be regenerated
	}
	if (s_opt.taskRegime == RegimeEditServer) {
		if (additionalArg == DEAD_CODE_DETECTION) {
			if (symbolIsReportableAsDead(s_inLastInfos.symbolTab[si])) {
				s_inLastInfos.symbolToCheckedForDeadness = si;
				s_inLastInfos.deadSymbolIsDefined = 0;
			} else {
				s_inLastInfos.symbolToCheckedForDeadness = -1;
			}
		} else if (s_opt.cxrefs!=OLO_TAG_SEARCH) {
			S_olSymbolsMenu		*ss;
			cms = NULL; ols = 0;
			if (additionalArg == CX_MENU_CREATION) {
				cms = createSelectionMenu(ddd);
				if (cms == NULL) {
					ols = 0;
				} else {
					if (IS_BEST_FIT_MATCH(cms)) ols = 2;
					else ols = 1;
				}
				//& ols=itIsSymbolToPushOlRefences(ddd,s_olcxCurrentUser->browserStack.top,&cms,DO_NOT_CHECK_IF_SELECTED);
			} else if (additionalArg!=CX_BY_PASS) {
				ols=itIsSymbolToPushOlRefences(ddd,s_olcxCurrentUser->browserStack.top,&cms,DEFAULT_VALUE);
			}
			s_inLastInfos.onLineRefMenuItem = cms;
//&fprintf(dumpOut,"check (%s) %s ols==%d\n", miscellaneousName[additionalArg], ddd->name, ols);
			if (ols || (additionalArg==CX_BY_PASS&&byPassAcceptableSymbol(ddd))
				) {
				s_inLastInfos.onLineReferencedSym = si;
				s_inLastInfos.onLineRefIsBestMatchFlag = (ols == 2);
//&sprintf(tmpBuff,"symbol %s is O.K.\n", ddd->name);ppcGenRecord(PPC_INFORMATION,tmpBuff, "\n");
//&fprintf(dumpOut,"symbol %s is O.K. for %s (ols==%d)\n", ddd->name, s_opt.browsedSymName, ols);
			} else {
				if (s_inLastInfos.onLineReferencedSym == si) {
					s_inLastInfos.onLineReferencedSym = -1;
				}
			}
		}
	}
	*ccc = cc; *ffin = fin;
}

static void cxrfReferenceForFullUpdateSchedule(		int size,
													int ri,
													char **ccc,
													char **ffin,
													S_charBuf *bbb,
													int additionalArg
	) {
	S_position 			pos;
	S_reference 		rr,rro;
	S_usageBits			usageBits;
	S_symbolRefItem 	*memb;
	int 				ii,file,line,coll,usage,sym,vApplClass,vFunClass,addfl,symType,reqAcc;
	int 				copyrefFl;
	assert(ri == CXFI_REFERENCE);
	usage = s_inLastInfos.counter[CXFI_USAGE];
	reqAcc = s_inLastInfos.counter[CXFI_REQ_ACCESS];
	FILL_usageBits(&usageBits, usage, reqAcc, 0);
	sym = s_inLastInfos.counter[CXFI_SYM_INDEX];
	file = s_inLastInfos.counter[CXFI_FILE_INDEX];
	file = s_decodeFilesNum[file];
	assert(s_fileTab.tab[file]!=NULL);
	line = s_inLastInfos.counter[CXFI_LINE_INDEX];
	coll = s_inLastInfos.counter[CXFI_COLL_INDEX];
	getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
//&fprintf(dumpOut,"%d %d->%d %d  ", usage,file,s_decodeFilesNum[file],line);fflush(dumpOut);
	FILL_position(&pos,file,line,coll);
	if (s_inLastInfos.onLineReferencedSym == 
		s_inLastInfos.counter[CXFI_SYM_INDEX]) {
			addToRefList(&s_inLastInfos.symbolTab[sym]->refs,
						 &usageBits,&pos,CatGlobal);
	}
}

static void cxrfReference(		int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int additionalArg
	) {
	S_position 			pos;
	S_reference 		rr,rro;
	S_symbolRefItem 	*memb;
	S_usageBits			usageBits;
	int 				ii,file,line,coll,usage,sym,addfl,reqAcc;
	int 				copyrefFl;
	assert(ri == CXFI_REFERENCE);
	usage = s_inLastInfos.counter[CXFI_USAGE];
	reqAcc = s_inLastInfos.counter[CXFI_REQ_ACCESS];
	sym = s_inLastInfos.counter[CXFI_SYM_INDEX];
	file = s_inLastInfos.counter[CXFI_FILE_INDEX];
	file = s_decodeFilesNum[file];
	assert(s_fileTab.tab[file]!=NULL);
	line = s_inLastInfos.counter[CXFI_LINE_INDEX];
	coll = s_inLastInfos.counter[CXFI_COLL_INDEX];
/*&fprintf(dumpOut,"%d %d %d  ", usage,file,line);fflush(dumpOut);&*/
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime == RegimeXref) {
		if (s_opt.keep_old ||
			(s_fileTab.tab[file]->b.cxLoading&&s_fileTab.tab[file]->b.cxSaved)
			) {
			/* if keep_old or if we repass refs after overflow */
			FILL_position(&pos,file,line,coll);
			FILL_usageBits(&usageBits, usage, reqAcc, 0);
			copyrefFl = ! isInRefList(s_inLastInfos.symbolTab[sym]->refs,
									  &usageBits, &pos, CatGlobal);
		} else {
			copyrefFl = ! s_fileTab.tab[file]->b.cxLoading;
		}
		if (copyrefFl) GenCxReferenceBase(sym, usage, reqAcc, file, line, coll);
	} else 	if (s_opt.taskRegime == RegimeHtmlGenerate) {
		FILL_position(&pos,file,line,coll);
		FILL_usageBits(&usageBits, usage, reqAcc, 0);
		FILL_reference(&rr, usageBits, pos, NULL);
		InternalCheck(sym>=0 && sym<MAX_CX_SYMBOL_TAB);
		if (additionalArg==CX_HTML_SECOND_PASS) {
			if (rr.usg.base<UsageMaxOLUsages || rr.usg.base==UsageClassTreeDefinition) {
				addToRefList(&s_inLastInfos.symbolTab[sym]->refs,
							 &rr.usg,&rr.p,CatGlobal);
			}
		} else if (rr.usg.base==UsageDefined || rr.usg.base==UsageDeclared
				   ||  s_fileTab.tab[rr.p.file]->b.commandLineEntered==0 ) {
/*&fprintf(dumpOut,"htmladdref %s on %s:%d\n",s_inLastInfos.symbolTab[sym]->name,s_fileTab.tab[rr.p.file]->name,rr.p.line);fflush(dumpOut);&*/
			addToRefList(&s_inLastInfos.symbolTab[sym]->refs,
						 &rr.usg,&rr.p,CatGlobal);
		}
	} else if (s_opt.taskRegime == RegimeEditServer) {
		FILL_position(&pos,file,line,coll);
		FILL_usageBits(&usageBits, usage, reqAcc, 0);
		FILL_reference(&rr, usageBits, pos, NULL);
		if (additionalArg == DEAD_CODE_DETECTION) {
			if (OL_VIEWABLE_REFS(&rr)) {
				// restrict reported symbols to those defined in project
				// input file
				if (IS_DEFINITION_USAGE(rr.usg.base) 
					&& s_fileTab.tab[rr.p.file]->b.commandLineEntered
					) {
					s_inLastInfos.deadSymbolIsDefined = 1;
				} else if (! IS_DEFINITION_OR_DECL_USAGE(rr.usg.base)) {
					s_inLastInfos.symbolToCheckedForDeadness = -1;
				}
			}
		} else if (additionalArg == OL_LOOKING_2_PASS_MACRO_USAGE) {
			if (	s_inLastInfos.onLineReferencedSym == 
					s_inLastInfos.counter[CXFI_SYM_INDEX]
					&&	rr.usg.base == UsageMacroBaseFileUsage) {
				s_olMacro2PassFile = rr.p.file;
			}
		} else {
			if (s_opt.cxrefs == OLO_TAG_SEARCH) {
				if (rr.usg.base==UsageDefined 
					|| ((s_opt.tagSearchSpecif==TSS_FULL_SEARCH
						 || s_opt.tagSearchSpecif==TSS_FULL_SEARCH_SHORT)
						&& 	(rr.usg.base==UsageDeclared
							 || rr.usg.base==UsageClassFileDefinition))) {
					searchSymbolCheckReference(s_inLastInfos.symbolTab[sym],&rr);
				}
			} else if (s_opt.cxrefs == OLO_SAFETY_CHECK1) {
				if (	s_inLastInfos.onLineReferencedSym != 
						s_inLastInfos.counter[CXFI_SYM_INDEX]) {
					olcxCheck1CxFileReference(s_inLastInfos.symbolTab[sym],
											  &rr); 
				}
			} else {
				if (	s_inLastInfos.onLineReferencedSym == 
						s_inLastInfos.counter[CXFI_SYM_INDEX]) {
					if (additionalArg == CX_MENU_CREATION) {
						assert(s_inLastInfos.onLineRefMenuItem);
						if (s_opt.keep_old 
							|| file!=s_olOriginalFileNumber
							|| s_fileTab.tab[file]->b.commandLineEntered==0
							|| s_opt.cxrefs==OLO_GOTO 
							|| s_opt.cxrefs==OLO_CGOTO
							|| s_opt.cxrefs==OLO_PUSH_NAME
							|| s_opt.cxrefs==OLO_PUSH_SPECIAL_NAME
							) {
//&fprintf(dumpOut,":adding reference %s:%d\n", s_fileTab.tab[rr.p.file]->name, rr.p.line);
							olcxAddReferenceToOlSymbolsMenu(s_inLastInfos.onLineRefMenuItem, &rr, s_inLastInfos.onLineRefIsBestMatchFlag);
						}
					} else if (additionalArg == CX_BY_PASS) {
						if (POSITION_EQ(s_olcxByPassPos,rr.p)) {
							// got the bypass reference 
//&fprintf(dumpOut,":adding bypass selected symbol %s\n", s_inLastInfos.symbolTab[sym]->name);
							olAddBrowsedSymbol(s_inLastInfos.symbolTab[sym],
											   &s_olcxCurrentUser->browserStack.top->hkSelectedSym,
											   1, 1, 0, usage,0,&s_noPos, UsageNone);
						}
					} else if (1 
							   /*& 
								 s_opt.keep_old 
								 || file!=s_olOriginalFileNumber
								 || s_fileTab.tab[file]->b.commandLineEntered==0
								 || s_opt.cxrefs==OLO_GOTO || s_opt.cxrefs==OLO_CGOTO
								 &*/
						) {
						// this is only goto definition from completion menu?
//&sprintf(tmpBuff,"%s %s:%d\n", usagesName[usage],s_fileTab.tab[file]->name,line);ppcGenRecord(PPC_INFORMATION,tmpBuff,"\n");
//&fprintf(dumpOut,"%s %s:%d\n", usagesName[usage],s_fileTab.tab[file]->name,line);fflush(dumpOut);
						olcxAddReference(&s_olcxCurrentUser->browserStack.top->r, &rr,
										 s_inLastInfos.onLineRefIsBestMatchFlag);
					}
				}
			}
		}
	}
}


static void cxrfRefNum(		int fileRefNum,
							int ri,
							char **ccc,
							char **ffin,
							S_charBuf *bbb,
							int additionalArg
						) {
	int check;
	check = changeRefNumOption(fileRefNum);
	if (check == 0)	{
		assert(s_opt.taskRegime);
		fatalError(ERR_ST,"Tag file was generated with different '-refnum' options, recreate it!", XREF_EXIT_ERR);
	}
}

static void cxrfSubClass(		int size,
								int ri,
								char **ccc,
								char **ffin,
								S_charBuf *bbb,
								int additionalArg
							) {
	S_position pos;
	S_reference rr;
	int of, file, sup, inf;
	assert(ri == CXFI_CLASS_EXT);
	file = of = s_inLastInfos.counter[CXFI_FILE_INDEX];
	sup = s_inLastInfos.counter[CXFI_SUPER_CLASS];
	inf = s_inLastInfos.counter[CXFI_INFER_CLASS];
/*fprintf(dumpOut,"%d %d->%d %d  ", usage,file,s_decodeFilesNum[file],line);*/
/*fflush(dumpOut);*/
	file = s_decodeFilesNum[file];
//&if (s_fileTab.tab[file]==NULL) {fprintf(dumpOut,"!%d->%d %d %d ",of,file,sup,inf);}
	assert(s_fileTab.tab[file]!=NULL);
	sup = s_decodeFilesNum[sup];
	assert(s_fileTab.tab[sup]!=NULL);
	inf = s_decodeFilesNum[inf];
	assert(s_fileTab.tab[inf]!=NULL);
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime == RegimeHtmlGenerate) {
		crSubClassInfo(sup, inf, file, NO_CX_FILE_ITEM_GEN);
	}
	if (s_opt.taskRegime == RegimeXref) {
		if (s_fileTab.tab[file]->b.cxLoading==0 &&
			additionalArg==CX_GENERATE_OUTPUT) {
			genSubClassInfo(sup, inf, file);  // updating refs
//&			crSubClassInfo(sup, inf, file, CX_FILE_ITEM_GEN);
		}
	}
	if (s_opt.taskRegime == RegimeEditServer) {
		if (file!=s_input_file_number) {
//&fprintf(dumpOut,":reading %s < %s\n", simpleFileName(s_fileTab.tab[inf]->name),simpleFileName(s_fileTab.tab[sup]->name));
			crSubClassInfo(sup, inf, file, NO_CX_FILE_ITEM_GEN);
		}
	}
}

void scanCxFile(S_cxScanFileFunctionLink *scanFuns) {
	int recInfo;
	int ch,i;
	char *cc, *cfin;
/*fprintf(dumpOut,"scanning ref. file start\n"); fflush(dumpOut);*/
	if (fIn == NULL) return;
/*fprintf(dumpOut,"scanning ref. file start 2\n"); fflush(dumpOut);*/
	memset(&s_inLastInfos, 0, sizeof(s_inLastInfos));
	s_inLastInfos.onLineReferencedSym = -1;
	s_inLastInfos.symbolToCheckedForDeadness = -1;
	s_inLastInfos.onLineRefMenuItem = NULL;
	s_inLastInfos.singleRecord[CXFI_INFER_CLASS] = s_noneFileIndex;
	s_inLastInfos.singleRecord[CXFI_SUPER_CLASS] = s_noneFileIndex;
	s_decodeFilesNum[s_noneFileIndex] = s_noneFileIndex;
	for(i=0; scanFuns[i].recordCode>0; i++) {
		assert(scanFuns[i].recordCode < MAX_CHARS);
		ch = scanFuns[i].recordCode;
		s_inLastInfos.fun[ch] = scanFuns[i].handleFun;
		s_inLastInfos.additional[ch] = scanFuns[i].additionalArg;
	}
	FILL_charBuf(&cxfBuf, cxfBuf.a, cxfBuf.a, fIn, 0, -1, 0, 0, 0, 0,INPUT_DIRECT,s_defaultZStream);
	ch = ' '; cc = cxfBuf.a; cfin = cxfBuf.fin;
	while(! cxfBuf.isAtEOF) {
		ScanInt(ch, cc, cfin, &cxfBuf, recInfo);
/*fprintf(stdout,"number %d scaned\n",recInfo);*/
/*fprintf(stdout,"record %d == %c scaned\n",ch,ch);*/
/*fflush(stdout);*/
		if (cxfBuf.isAtEOF) break;
		assert(ch >= 0 && ch<MAX_CHARS);
		if (s_inLastInfos.singleRecord[ch]) {
			s_inLastInfos.counter[ch] = recInfo;
		}
		if (s_inLastInfos.fun[ch] != NULL) {
			(*s_inLastInfos.fun[ch])(recInfo, ch, &cc, &cfin, &cxfBuf,
									 s_inLastInfos.additional[ch]);
		} else if (! s_inLastInfos.singleRecord[ch]) {
			assert(recInfo>0);
			SkipNChars(recInfo-1, cc, cfin, &cxfBuf);
		}
		GetChar(ch,cc,cfin,&cxfBuf);
	}
	if (s_opt.taskRegime==RegimeEditServer 
		&& (s_opt.cxrefs==OLO_LOCAL_UNUSED
			|| s_opt.cxrefs==OLO_GLOBAL_UNUSED)) {
		// check if last symbol was dead
		cxfileCheckLastSymbolDeadness();
	}
#ifdef LINEAR_ADD_REFERENCE
	purgeCxReferenceTable();
#endif
/*fprintf(dumpOut,"ref. file scanned\n"); fflush(dumpOut);*/
}


/* fnamesuff contains '/' at the beginning !!! */

int scanReferenceFile(	char *fname, char *fns1, char *fns2,
						S_cxScanFileFunctionLink *scanFunTab
	) {
	char fn[MAX_FILE_NAME_SIZE];
	sprintf(fn, "%s%s%s", fname, fns1, fns2);
	InternalCheck(strlen(fn) < MAX_FILE_NAME_SIZE-1);
//&fprintf(dumpOut,":scanning file %s\n",fn);
	fIn = fopen(fn,"r");
	if (fIn==NULL) {
	  //&		sprintf(tmpBuff,
	  //&			"can't open TAG file %s\n\tcreate the TAG file first",fn);
	  //& error(ERR_ST,tmpBuff);
		return(0);
	} else {
		scanCxFile(scanFunTab);
		fclose(fIn);
		fIn = NULL;
		return(1);
	}
}

void scanReferenceFiles(char *fname, S_cxScanFileFunctionLink *scanFunTab) {
	char 	nn[MAX_FILE_NAME_SIZE];
	char 	*dirname;
	int		i;
	if (s_opt.refnum <= 1) {
		scanReferenceFile(fname,"","",scanFunTab);
	} else {
		scanReferenceFile(fname,PRF_FILES,"",scanFunTab);
		scanReferenceFile(fname,PRF_CLASS,"",scanFunTab);
		for (i=0; i<s_opt.refnum; i++) {
			sprintf(nn,"%04d",i);
			scanReferenceFile(fname,PRF_REF_PREFIX,nn,scanFunTab);
		}
	}
}

int smartReadFileTabFile() {
	static time_t 	readedFileModTime = 0;
	static off_t	readedFileSize = 0;
	static char 	readedFileFile[MAX_FILE_NAME_SIZE] = {0};
	char			tt[MAX_FILE_NAME_SIZE];
	struct stat 	st;
	int 			tmp;
	if (s_opt.refnum > 1) {
		sprintf(tt, "%s%s", s_opt.cxrefFileName, PRF_FILES);
	} else {
		sprintf(tt, "%s", s_opt.cxrefFileName);
	}
	if (statb(tt,&st)==0) {
		if (	readedFileModTime != st.st_mtime
			||	readedFileSize != st.st_size
			||	strcmp(readedFileFile, tt) != 0) {
//&fprintf(dumpOut,":(re)reading file tab\n");
			tmp = scanReferenceFile(tt,"","",s_cxScanFileTab);
			if (tmp != 0) {
				readedFileModTime = st.st_mtime;
				readedFileSize = st.st_size;
				strcpy(readedFileFile, tt);
			} 
		} else {
//&fprintf(dumpOut,":saving the (re)reading of file tab\n");
		}
		return(1);
	}
	return(0);
}

// symbolName can be NULL !!!!!!
void readOneAppropReferenceFile(char *symbolName,
								S_cxScanFileFunctionLink  *scanFunTab
	) {
	static char fns[MAX_FILE_NAME_SIZE];
	int i,tmp;
//&	if (s_opt.cxrefs!=OLO_CGOTO && ! creatingOlcxRefs()) return;
	if (s_opt.cxrefFileName == NULL) return;
	cxOut = stdout;
	if (s_opt.refnum <= 1) {
		tmp = scanReferenceFile(s_opt.cxrefFileName,"","",scanFunTab);
		if (tmp == 0) return;
	} else {
		tmp = smartReadFileTabFile();
		if (tmp == 0) return;
		tmp = scanReferenceFile(s_opt.cxrefFileName,PRF_CLASS,"",
								scanFunTab);
		if (tmp == 0) return;
		if (symbolName == NULL) return;
        /* following must be after reading XFiles*/
		i = cxFileHashNumber(symbolName);
//&fprintf(dumpOut,"reading X%04d\n",i);fflush(dumpOut);
		sprintf(fns, "%04d", i);
		InternalCheck(strlen(fns) < MAX_FILE_NAME_SIZE-1);
		tmp = scanReferenceFile(s_opt.cxrefFileName,PRF_REF_PREFIX,fns,
								scanFunTab);
		if (tmp == 0) return;
	}
}


/* ************************************************************ */


S_cxScanFileFunctionLink s_cxScanFileTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxFullScanFunTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_VERSION, cxrfVersionCheck, 0},
	{CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_GENERATE_OUTPUT},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_GENERATE_OUTPUT},
	{CXFI_SYM_NAME,cxrfSymbolName, DEFAULT_VALUE},
	{CXFI_REFERENCE,cxrfReference, CX_HTML_FIRST_PASS},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_GENERATE_OUTPUT},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxByPassFunTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_VERSION, cxrfVersionCheck, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, CX_BY_PASS},
	{CXFI_REFERENCE,cxrfReference, CX_BY_PASS},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_JUST_READ},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxSymbolLoadMenuRefs[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_VERSION, cxrfVersionCheck, 0},
	{CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, DEFAULT_VALUE},
	{CXFI_REFERENCE,cxrfReference, CX_MENU_CREATION},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_JUST_READ},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxSymbolMenuCreationTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_VERSION, cxrfVersionCheck, 0},
	{CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, CX_MENU_CREATION},
	{CXFI_REFERENCE,cxrfReference, CX_MENU_CREATION},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_JUST_READ},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxFullUpdateScanFunTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_VERSION, cxrfVersionCheck, 0},
	{CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolNameForFullUpdateSchedule, DEFAULT_VALUE},
	{CXFI_REFERENCE,cxrfReferenceForFullUpdateSchedule, DEFAULT_VALUE},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxScanFunTabFor2PassMacroUsage[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, DEFAULT_VALUE},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, OL_LOOKING_2_PASS_MACRO_USAGE},
	{CXFI_REFERENCE,cxrfReference, OL_LOOKING_2_PASS_MACRO_USAGE},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_JUST_READ},
	{CXFI_REFNUM,cxrfRefNum, DEFAULT_VALUE},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxScanFunTabForClassHierarchy[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, DEFAULT_VALUE},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_JUST_READ},
	{CXFI_REFNUM,cxrfRefNum, DEFAULT_VALUE},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxHtmlGlobRefListScanFunTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_CLASS_EXT,cxrfSubClass, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, CX_HTML_SECOND_PASS},
	{CXFI_REFERENCE,cxrfReference, CX_HTML_SECOND_PASS},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxSymbolSearchScanFunTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, SEARCH_SYMBOL},
	{CXFI_REFERENCE,cxrfReference, CX_HTML_FIRST_PASS},
	{-1,NULL, 0},
};

S_cxScanFileFunctionLink s_cxDeadCodeDetectionScanFunTab[]={
	{CXFI_SINGLE_RECORDS,cxrfSetSingleRecords, 0},
	{CXFI_REFNUM,cxrfRefNum, 0},
	{CXFI_FILE_NAME,cxrfFileName, CX_JUST_READ},
	{CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
	{CXFI_SYM_NAME,cxrfSymbolName, DEAD_CODE_DETECTION},
	{CXFI_REFERENCE,cxrfReference, DEAD_CODE_DETECTION},
	{-1,NULL, 0},
};




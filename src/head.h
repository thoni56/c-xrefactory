#ifndef _HEAD__H
#define _HEAD__H

#include "strFill.h"

#include "listmacr.h"
#include "memmac.h"
#include "head2.h"		/*SBD*/

/* ************************** VERSION NUMBER ********************* */

// version numbers were moved to 'protocol.tc'

/* *********************************************************************** */

#define JAVA_VERSION_1_3 "1.3"
#define JAVA_VERSION_1_4 "1.4"
#define JAVA_VERSION_AUTO "auto"

/* **************************** EXPIRATIONS ********************** */

#define EXPIRATION0 0

#define BIN_REL_LICENSE_STRING "1/1/2000/1365:x:dadde11113b13b11"

// any-for-02-2004.tar.gz  i.e. until Thu Apr  1 00:00:00 2004
//#define EXPIRATION 1080770400
// any-for-03-2004.tar.gz  i.e. until Sat May  1 00:00:00 2004
//#define EXPIRATION 1083362400
// any-for-04-2004.tar.gz  i.e. until Tue Jun  1 00:00:00 2004
//#define EXPIRATION 1086040800
// any-for-05-2004.tar.gz  i.e. until Thu Jul  1 00:00:00 2004
//#define EXPIRATION 1088632800
// any-for-06-2004.tar.gz  i.e. until Sun Aug  1 00:00:00 2004
//#define EXPIRATION 1091311200
// any-for-07-2004.tar.gz  i.e. until Wed Sep  1 00:00:00 2004
#define EXPIRATION 1093989600
// any-for-08-2004.tar.gz  i.e. until Fri Oct  1 00:00:00 2004
//#define EXPIRATION 1096581600
// any-for-09-2004.tar.gz  i.e. until Mon Nov  1 00:00:00 2004
//#define EXPIRATION 1099263600
// any-for-10-2004.tar.gz  i.e. until Wed Dec  1 00:00:00 2004
//#define EXPIRATION 1101855600
// any-for-11-2004.tar.gz  i.e. until Sat Jan  1 00:00:00 2005
//#define EXPIRATION 1104534000
// any-for-12-2004.tar.gz  i.e. until Tue Feb  1 00:00:00 2005
//#define EXPIRATION 1107212400
// any-for-01-2005.tar.gz  i.e. until Tue Mar  1 00:00:00 2005
//#define EXPIRATION 1109631600

/* ******************** assertion checks ****************** */


#define InternalCheck(expr) {\
	if (!(expr)) internalCheckFail(#expr, __FILE__, __LINE__);\
}

#undef assert
#define assert(expr) InternalCheck(expr)

/* ******************** language identification ****************** */

/* bitwise different */
#define LAN_C		2
#define LAN_JAVA	4
#define LAN_YACC	8
#define LAN_CCC		16	/* ccc - standing for C++ */
#define LAN_JAR		32
#define LAN_CLASS	64

/* ****************** end of line conversions ***************************** */

#define NO_EOL_CONVERSION		0
#define CR_LF_EOL_CONVERSION	1
#define CR_EOL_CONVERSION		2

/* ****************** X-files hashing method****************************** */

#define XFILE_HASH_DEFAULT 0
#define XFILE_HASH_ALPHA1 1
#define XFILE_HASH_ALPHA2 2
#define XFILE_HASH_MAX 3

#define XFILE_HASH_ALPHA1_REFNUM ('z'-'a'+2)
#define XFILE_HASH_ALPHA2_REFNUM (XFILE_HASH_ALPHA1_REFNUM*XFILE_HASH_ALPHA1_REFNUM)

/* *********************************************************************** */

#define STORAGES_LN 5		/* logarithm of MAX_STORAGE */
#define SYMTYPES_LN 7
#define SCOPES_LN 3

/* *********************************************************************** */

#define XREF_EXIT_BASE 			64	// base for exit status
#define XREF_EXIT_ERR 			65
#define XREF_EXIT_NO_PROJECT 	66
#define XREF_EXIT_LICENSE		67

/* *********************************************************************** */

#define ANY_FILE (-1)		// must be different from any file number
#define ANY_CPP_PASS (-1) 	// must be different from any cpp pass number

/* *********************************************************************** */

#define CONTINUE_LABEL_NAME " %cntl%"
#define BREAK_LABEL_NAME " %brkl%"
#define SWITCH_LABEL_NAME " %swtl%"

// special link names starts by one space character
// special local link names starts by two spaces
#define LINK_NAME_CLASS_TREE_ITEM 				" [classTree]"
#define LINK_NAME_IMPORTED_QUALIFIED_ITEM 		" redundant long names of the project"
#define LINK_NAME_IMPORT_STATEMENT		 		"  import statement"
#define LINK_NAME_UNIMPORTED_QUALIFIED_ITEM 	"  unimported type"
#define LINK_NAME_MAYBE_THIS_ITEM 				"  method's \"maybe this\" dependencies"
#define LINK_NAME_SUPER_METHOD_ITEM 			"  \"super\" dot method references"
#define LINK_NAME_NOT_FQT_ITEM 					"  not fully qualified names"
#define LINK_NAME_FUNCTION_SEPARATOR 			"  fun separator"
#define LINK_NAME_SAFETY_CHECK_MISSED   		"  Conflicting References"
#define LINK_NAME_SAFETY_CHECK_LOST   			"  References lost by refactoring"
#define LINK_NAME_SAFETY_CHECK_FOUND   			"  Unexpected new references"
#define LINK_NAME_INDUCED_ERROR         		"  References Misinterpreted due to previous Errors"
#define LINK_NAME_MOVE_CLASS_MISSED     		"  Symbols Inaccessible After Class Moving"

#define LINK_NAME_CUT_SYMBOL '!'
#define LINK_NAME_COLLATE_SYMBOL '#'
#define LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG '*'
#define LINK_NAME_EXTRACT_DEFAULT_FLAG ' '

#define LINK_NAME_INCLUDE_REFS "%%%i"		// no #, netscape does not like them

/* *********************************************************************** */

#define LANGUAGE(lan) ((s_language & (lan)) != 0)
#define ABS(xxx) ((xxx>0)?(xxx):(-(xxx)))
#define CX_REGIME() (s_opt.taskRegime!=RegimeGenerate)
#define WORK_NEST_LEVEL0() 	(s_topBlock->previousTopBlock == NULL)
#define CLASS_NAME_FROM_NUM(cnum) (s_fileTab.tab[cnum]->name+1)

/* *************************************************************** */

#define MAX_CHARS 127	/* maximal value storable in char encoded types 	*/
						/* just for strings encoding fun-profiles for link  */

// !!!!!!! do not move the following into head2.h header, must be here
// because if not, YACC will put there defaults !!!!!!
#define YYSTACKSIZE 5000

/* **************  edit communication constants  ***************** */

#define CC_COMPLETION 	'c'
#define CC_CXREF 		'x'
#define CC_EOF 			'\n'

/* ***************** OLCX COMMUNICATION CHARS ******************** */

#define COLCX_GOTO_REFERENCE 		"#"
#define COLCX_GOTO_REFERENCE_LIST 	"@"
#define COLCX_LIST					";"

/* ************ char to separate archive from file name ********** */

#define ZIP_SEPARATOR_CHAR ';'
#define HTML_GXANY -1

/* *************************************************************** */

#define TOKEN_SIZE 2
#define IDENT_TOKEN_SIZE 2

/* ********************************************************************** */

#define ERROR_MESSAGE_STARTING_OFFSET 10

/* ********************************************************************** */

#define CCT_TREE_INDEX		 4	/* number of subtrees on each cct node    */

#define TYPE_STR_RESERVE   100  /* reserve when sprinting type name */
                                /* is it still actual ????????????? */
#define NEST_VIRT_COMPL_OFFSET 1000

#define MAXIMAL_INT 			((int) (((unsigned) -2)>>1))

#define MAX_AVAILABLE_REFACTORINGS 500
#define OLCX_CHECK_ARRAY_SIZE 1000000

/* ********************************************************************** */

/* stack memory synchronized with program block structure */
#define XX_ALLOC(p,t) 			{p = (t*) stackMemoryAlloc(sizeof(t)); }
#define XX_ALLOCC(p,n,t) 		{p = (t*) stackMemoryAlloc((n)*sizeof(t)); }
#define XX_FREE(p) 				{ }

/* pre-processor macro definitions allocations */
#define PP_ALLOC(p,t) 			{SM_ALLOC(ppmMemory,p,t);}
#define PP_ALLOCC(p,n,t) 		{SM_ALLOCC(ppmMemory,p,n,t);}
#define PP_REALLOCC(p,n,t,on)	{SM_REALLOCC(ppmMemory,p,n,t,on);}
#define PP_FREE_UNTIL(p) 		{SM_FREE_UNTIL(ppmMemory,p);}
#define PP_FREE(p) 				{ }

/* java class-file read allocations ( same memory as cpp !!!!!!!! ) */
#define CF_ALLOC(p,t) 			{SM_ALLOC(ppmMemory,p,t);}
#define CF_ALLOCC(p,n,t) 		{SM_ALLOCC(ppmMemory,p,n,t);}
#define CF_REALLOCC(p,n,t,on)	{SM_REALLOCC(ppmMemory,p,n,t,on);}
#define CF_FREE_UNTIL(p) 		{SM_FREE_UNTIL(ppmMemory,p);}
#define CF_FREE(p) 				{ }

/* cross - references global symbols allocations */
#define CX_ALLOC(p,t) 			{DM_ALLOC(cxMemory,p,t);}
#define CX_ALLOCC(p,n,t) 		{DM_ALLOCC(cxMemory,p,n,t);}
#define CX_FREE_UNTIL(p) 		{DM_FREE_UNTIL(cxMemory,p);}

/* file table allocations */
#define FT_ALLOC(p,t) 			{SM_ALLOC(ftMemory,p,t);}
#define FT_ALLOCC(p,n,t) 		{SM_ALLOCC(ftMemory,p,n,t);}
#define FT_FREE_UNTIL(p) 		{SM_FREE_UNTIL(ftMemory,p);}

/* options allocations */
#define OPT_ALLOC(p,t) 			{DM_ALLOC(((S_memory*)&s_opt.pendingMemory),p,t);}
#define OPT_ALLOCC(p,n,t) 		{DM_ALLOCC(((S_memory*)&s_opt.pendingMemory),p,n,t);}

/* on-line dialogs allocation */
#define OLCX_ALLOCC(p,n,t) {\
	RLM_SOFT_ALLOCC(olcxMemory, p, n, t);\
	while (p==NULL) {\
		freeOldestOlcx();\
		RLM_SOFT_ALLOCC(olcxMemory, p, n, t);\
	}\
}
#define OLCX_ALLOC(p,t) OLCX_ALLOCC(p,1,t)
#define OLCX_FREE(p,size) RLM_FREE(olcxMemory, p, size)

/* editor allocations, for now, store it in olcxmemory */
#define ED_ALLOCC(p,n,t) OLCX_ALLOCC(p,n,t)
#define ED_ALLOC(p,t) ED_ALLOCC(p,1,t)
#define ED_FREE(p,size) OLCX_FREE(p,size) 

/* *********************************************************************** */

#ifdef DEBUG
#define DPRINTF(Format) {\
	if (s_opt.debug) {fprintf(dumpOut,Format);fflush(dumpOut);}\
}
#define DPRINTF1(Format) {\
	if (s_opt.debug) {fprintf(dumpOut,Format);fflush(dumpOut);}\
}
#define DPRINTF2(Format,Arg1) {\
	if (s_opt.debug) {fprintf(dumpOut,Format,Arg1);fflush(dumpOut);}\
}
#define DPRINTF3(Format,Arg1,Arg2) {\
	if (s_opt.debug) {fprintf(dumpOut,Format,Arg1,Arg2);fflush(dumpOut);}\
}
#define DPRINTF4(Format,Arg1,Arg2,Arg3) {\
	if (s_opt.debug) {\
		fprintf(dumpOut,Format,Arg1,Arg2,Arg3);\
		fflush(dumpOut);\
	}\
}
#define DPRINTF5(Format,Arg1,Arg2,Arg3,Arg4) {\
	if (s_opt.debug) {\
		fprintf(dumpOut,Format,Arg1,Arg2,Arg3,Arg4);\
		fflush(dumpOut);\
	}\
}
#define DPRINTF6(Format,Arg1,Arg2,Arg3,Arg4,Arg5) {\
	if (s_opt.debug) {\
		fprintf(dumpOut,Format,Arg1,Arg2,Arg3,Arg4,Arg5);\
		fflush(dumpOut);\
	}\
}
#define DPRINTF7(Format,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6) {\
	if (s_opt.debug) {\
		fprintf(dumpOut,Format,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6);\
		fflush(dumpOut);\
	}\
}
#else
#define DPRINTF(Format) {}
#define DPRINTF1(Format) {}
#define DPRINTF2(Format,Arg1) {}
#define DPRINTF3(Format,Arg1,Arg2) {}
#define DPRINTF4(Format,Arg1,Arg2,Arg3) {}
#define DPRINTF5(Format,Arg1,Arg2,Arg3,Arg4) {}
#define DPRINTF6(Format,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6) {}
#endif

/* ********************************************************************** */
/*                      common macros for C arguments                     */

#if defined(PRESERVE_C_ARGS) || defined(__cplusplus)
#define C_ARG(xxx) xxx
#else
#define C_ARG(xxx) ()
#endif

/* ********************************************************************** */
/*            common integer return values for cplex funs                 */

#define RETURN_OK 			0
#define RETURN_NOT_FOUND	1
#define RETURN_ERROR		2

/* ******************************************************************** */

#define USAGE_TOP_LEVEL_USED UsageAddrUsed	/* type name at top-level */
#define USAGE_EXTEND_USAGE UsageLvalUsed	/* type name in extend context */

/* ******************************************************************** */

#define FILL_TM(mtt,mden,mmesiac,mrok,mhod,mmin,msek) {\
  (mtt)->tm_mday = mden;\
  (mtt)->tm_mon = mmesiac-1;\
  (mtt)->tm_year = mrok-1900;\
  (mtt)->tm_hour = mhod;\
  (mtt)->tm_min = mmin;\
  (mtt)->tm_sec = msek;\
  (mtt)->tm_isdst = -1;\
}

#define UNFILL_TM(mtt,mden,mmesiac,mrok,mhod,mmin,msek) {\
  mden = (mtt)->tm_mday;\
  mmesiac = (mtt)->tm_mon+1;\
  mrok = (mtt)->tm_year+1900;\
  mhod = (mtt)->tm_hour;\
  mmin = (mtt)->tm_min;\
  msek = (mtt)->tm_sec;\
}

/* ********************      PROTECTION    ************************** */

#define EXP_COM_SIZE 32

#ifdef BIN_RELEASE									// BIN_RELEASE

#define FILL_EXP_COMMAND() {}
#define SET_EXPIRATION() {}
#define BIN_LICENSE_CHECK(lc) {lc=0;}
#define LICENSE_CHECK() {}
#define LICENSE_CHECK2() {}

#else												// SOURCE_RELEASE 

#define BIN_LICENSE_CHECK(lc) {lc=0;}
#define FILL_EXP_COMMAND() {}

#ifdef TIME_LIMITED				// SOURCE RELEASE && TIME_LIMITED
#define SET_EXPIRATION() {}
#define LICENSE_CHECK() {}
#define LICENSE_CHECK2() {}
#else							// SOURCE RELEASE && TIME_UNLIMITED
#define SET_EXPIRATION() {}
#define LICENSE_CHECK() {}
#define LICENSE_CHECK2() {}
#endif
#endif

#define DEFINITION_NOT_FOUND_MESSAGE "Definition not found"

/* *********************************************************************** */

#define GenInternalLabelReference(count,usage) {\
	if (s_opt.cxrefs == OLO_EXTRACT) genInternalLabelReference(count, usage);\
}

#define AddContinueBreakLabelSymbol(count, name, res) {\
	if (s_opt.cxrefs == OLO_EXTRACT) res = addContinueBreakLabelSymbol(count, name);\
}

#define DeleteContinueBreakLabelSymbol(name) {\
	if (s_opt.cxrefs == OLO_EXTRACT) deleteContinueBreakLabelSymbol(name);\
}

#define GenContBreakReference(name) {\
	if (s_opt.cxrefs == OLO_EXTRACT) genContinueBreakReference(name);\
}

#define GenSwitchCaseFork(lastFlag) {\
	if (s_opt.cxrefs == OLO_EXTRACT) genSwitchCaseFork(lastFlag);\
}

#define ExtrDeleteContBreakSym(sym) {\
	if (s_opt.cxrefs == OLO_EXTRACT) deleteSymDef(sym);\
}


#define EXTRACT_COUNTER_SEMACT(rescount) {\
		rescount = s_count.localSym;\
		s_count.localSym++;\
}

#define EXTRACT_LABEL_SEMACT(rescount) {\
		rescount = s_count.localSym;\
		GenInternalLabelReference(s_count.localSym, UsageDefined);\
		s_count.localSym++;\
}

#define EXTRACT_GOTO_SEMACT(rescount) {\
		rescount = s_count.localSym;\
		GenInternalLabelReference(s_count.localSym, UsageUsed);\
		s_count.localSym++;\
}

#define EXTRACT_FORK_SEMACT(rescount) {\
		rescount = s_count.localSym;\
		GenInternalLabelReference(s_count.localSym, UsageFork);\
		s_count.localSym++;\
}

#define RESET_REFERENCE_USAGE(rrr,uuu) {\
	if (rrr!=NULL && rrr->usg.base > uuu) {\
		rrr->usg.base = uuu;\
	}\
}

#define POSITION_NEQ(p1,p2) (\
	((p1).file != (p2).file) || \
	((p1).line != (p2).line) || \
	((p1).coll != (p2).coll) \
)

#define POSITION_EQ(p1,p2) (! POSITION_NEQ(p1,p2))

#define POSITION_LESS(p1,p2) (\
	 ((p1).file < (p2).file) ||\
	 ((p1).file==(p2).file && (p1).line < (p2).line)  || \
	 ((p1).file==(p2).file && (p1).line==(p2).line && (p1).coll < (p2).coll) \
)
#define POSITION_LESS_EQ(p1,p2) (\
	 ((p1).file < (p2).file) ||\
	 ((p1).file==(p2).file && (p1).line < (p2).line)  || \
	 ((p1).file==(p2).file && (p1).line==(p2).line && (p1).coll <= (p2).coll) \
)
#define POSITION_IS_BETWEEN_IN_THE_SAME_FILE(p1,p,p2) (\
	(p1).file == (p).file\
	&& (p).file == (p2).file\
	&& POSITION_LESS_EQ(p1,p)\
	&& POSITION_LESS_EQ(p,p2)\
)

#define MARKER_EQ(mm1, mm2) (mm1->buffer==mm2->buffer && mm1->offset==mm2->offset)

#define REF_ELEM_EQUAL(e1,e2) (\
	e1->b.symType==e2->b.symType && \
	e1->b.storage==e2->b.storage && \
	e1->b.category==e2->b.category && \
	e1->vApplClass==e2->vApplClass && \
	strcmp(e1->name,e2->name)==0\
)

#define IS_DEFINITION_USAGE(usage) (\
  (usage)==UsageDefined \
  || (usage)==UsageOLBestFitDefined\
)

#define IS_DEFINITION_OR_DECL_USAGE(usage) (\
  IS_DEFINITION_USAGE(usage) \
  || (usage)==UsageDeclared \
)

#define OL_VIEWABLE_REFS(rrr) ((rrr)->usg.base < UsageMaxOLUsages)

#define SET_IDENTIFIER_YYLVAL(name, symb, pos) {\
	uniyylval->bbidIdent.d = &s_yyIdentBuf[s_yyIdentBufi];\
	s_yyIdentBufi ++; s_yyIdentBufi %= (YYBUFFERED_ID_INDEX);\
	FILL_idIdent(uniyylval->bbidIdent.d, name, symb, pos);\
	yytext = name;\
	uniyylval->bbidIdent.b = pos;\
	uniyylval->bbidIdent.e = pos;\
	uniyylval->bbidIdent.e.coll += strlen(yytext);\
}

#define SHOW_COMPLETION_WINDOW(ccc) (\
   (ccc)->comPrefix[0]==0\
)

#define IS_BEST_FIT_MATCH(ss) (\
	(ss->ooBits&OOC_VIRTUAL_MASK)==OOC_VIRT_SAME_APPL_FUN_CLASS\
)

#define AddSymbolNoTrail(pp,symtab) {\
	int i;\
	S_symbol *memb;\
	symTabIsMember(symtab,pp,&i,&memb);\
	if (	LANGUAGE(LAN_CCC) && pp->b.symType==TypeDefault \
			&& pp->u.type->m == TypeFunction) { \
		pp->u.type->u.f.thisFunList = &(symtab->tab[i]); \
	} \
	symTabSet(symtab,pp,i);\
}

#define LINK_NAME_MAYBE_START(ccc) (\
	ccc=='.' || ccc=='/' || ccc==LINK_NAME_CUT_SYMBOL \
	|| ccc==LINK_NAME_COLLATE_SYMBOL \
	|| ccc=='$'\
)

#define GET_NUDE_NAME(name, start, len) {\
	register int _c_;\
	register char *_ss_;\
	_ss_ = start = name; \
	while ((_c_= *_ss_)) {\
		if (_c_ == '(') break;\
		if (LINK_NAME_MAYBE_START(_c_)) start = _ss_+1;\
		_ss_++ ;\
	}\
	len = _ss_ - start;\
}

#define JAVA_STATICALLY_LINKED(storage, accessFlags) (\
	(storage==StorageField\
	|| ((storage==StorageMethod || storage==StorageConstructor)\
		&& (accessFlags & ACC_STATIC)))\
)

#define JavaMapOnPaths(thePaths, COMMAND ) {\
	char *currentPath, *jmop_pp, *jmop_ecp;\
	int jmop_i, jmop_ind;\
	/* following was static, but I need to call this recursively */\
	char sourcepathes[MAX_SOURCE_PATH_SIZE];\
	assert(thePaths!=NULL);\
	jmop_pp = thePaths;\
	strcpy(sourcepathes, jmop_pp);\
	currentPath = sourcepathes;\
	jmop_ecp = currentPath+strlen(currentPath);\
	while (currentPath<jmop_ecp) {\
		for(jmop_ind=0; \
			currentPath[jmop_ind]!=0 && currentPath[jmop_ind]!=CLASS_PATH_SEPARATOR; \
			jmop_ind++) ;\
		currentPath[jmop_ind] = 0;\
		jmop_i = jmop_ind;\
		if (jmop_i>0 && currentPath[jmop_i-1]==SLASH) currentPath[--jmop_i] = 0;\
		COMMAND;\
		currentPath += jmop_ind;\
		currentPath++;\
	}\
}

#define SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs,pbflag) {\
	assert(s_olcxCurrentUser);\
/*&fprintf(ccOut,";safetyCheck2!!!\n");fflush(dumpOut);&*/\
	refs = s_olcxCurrentUser->browserStack.top;\
	if (refs==NULL || refs->previous==NULL || refs->previous->previous==NULL){\
		error(ERR_INTERNAL, "something goes wrong at safety check");\
		pbflag = 1;\
	} else {\
		newrefs = refs;\
		diffrefs = refs->previous;\
		origrefs = refs->previous->previous;\
	}\
}

#define MOVE_CLASS_MAP_FUN_RETURN_ON_UNINTERESTING_SYMBOLS(ri,dd) {\
	if (! isPushAllMethodsValidRefItem(ri)) return;\
	/* this is too strong, but check only fields and methods */\
	if (ri->b.storage!=StorageField\
		&& ri->b.storage!=StorageMethod\
		&& ri->b.storage!=StorageConstructor) return;\
	/* check that it has default accessibility*/\
	if (ri->b.accessFlags & ACC_PUBLIC) return;\
	if (ri->b.accessFlags & ACC_PROTECTED) return;\
	if (! (ri->b.accessFlags & ACC_PRIVATE)) {\
		/* default accessibility, check only if transpackage move*/\
		if (! dd->transPackageMove) return;\
	}\
}

#define IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, dd) ( \
  DM_IS_BETWEEN(cxMemory, rr, (dd)->minMemi, (dd)->maxMemi) \
  && OL_VIEWABLE_REFS(rr)\
  && rr->p.file != s_noneFileIndex \
  && rr->p.file == s_input_file_number /* fixing bug with references comming from jsl */\
)

#define ABS_FILE_POS(cb,cfin,ccc) (cb->filePos - (cfin-ccc) - 1)
#define COLUMN_POS(ccc,clb,clo) (ccc-clb+clo-1)

#define JAVA2HTML() (s_opt.java2html)

#define SPRINT_FILE_TAB_CLASS_NAME(ftname, linkName) {\
	sprintf(ftname, "%c%s.class", ZIP_SEPARATOR_CHAR, linkName);\
	assert(strlen(ftname)+1 < MAX_FILE_NAME_SIZE);\
}

/*#define LARGE_FILE_POSITION() XREF_HUGE */
#define LARGE_FILE_POSITION() (MAX_FILES >= 65535)

#define SYMBOL_MENU_FIRST_LINE 0
#define MAX_ASCII_CHAR 256


#define MARKER_TO_POINTER(marker) (marker->buffer->a.text+marker->offset)
#define POINTER_AFTER_MARKER(marker) (marker->buffer->a.text+marker->offset+1)
#define POINTER_BEFORE_MARKER(marker) (marker->buffer->a.text+marker->offset-1)
#define CHAR_BEFORE_MARKER(marker) (*POINTER_BEFORE_MARKER(marker))
#define CHAR_ON_MARKER(marker) (*MARKER_TO_POINTER(marker))
#define CHAR_AFTER_MARKER(marker) (*POINTER_AFTER_MARKER(marker))

/* *********************************************************************** */

#define StackMemPush(x,t) ((t*) stackMemoryPush(x,sizeof(t)))
#define StackMemAlloc(t) ((t*) stackMemoryAlloc(sizeof(t)))

/* ******************* symbol table hash function unit ****************** */

#if ZERO   // OLD_HASH
#define SYM_TAB_HASH_FUN_INC(oldval, charcode) {\
	oldval = oldval+oldval+charcode;\
}
#define SYM_TAB_HASH_FUN_FINAL(oldval) {}
#else
#define SYM_TAB_HASH_FUN_INC(oldval, charcode) {\
	oldval+=charcode; oldval+=(oldval<<10); oldval^=(oldval>>6);\
}
#define SYM_TAB_HASH_FUN_FINAL(oldval) {\
	oldval+=(oldval<<3); oldval^=(oldval>>11); oldval+=(oldval<<15);\
}
#endif

/* *********************************************************************** */
/*                    JAVA Constant Pool Item Tags                         */

#define CONSTANT_Asciz				1
#define CONSTANT_Unicode			2			/* ?????????????? */
#define CONSTANT_Integer			3
#define CONSTANT_Float				4
#define CONSTANT_Long				5
#define CONSTANT_Double				6
#define CONSTANT_Class				7
#define CONSTANT_String				8
#define CONSTANT_Fieldref			9
#define CONSTANT_Methodref			10
#define CONSTANT_InterfaceMethodref	11
#define CONSTANT_NameandType		12

/* *********************************************************************** */
/*                       JAVA Attribute Item                               */

#define ACC_PUBLIC				0x001
#define ACC_PRIVATE				0x002
#define ACC_PROTECTED			0x004
#define ACC_STATIC				0x008
#define ACC_FINAL				0x010
#define ACC_SYNCHRONIZED		0x020
#define ACC_THREADSAFE			0x040
#define ACC_TRANSIENT			0x080
#define ACC_NATIVE				0x100
#define ACC_INTERFACE			0x200
#define ACC_ABSTRACT			0x400

#define ACC_DEFAULT				0x000
#define ACC_ALL					0x800

#define ACC_PPP_MODIFER_MASK 	0x007

/* *********************************************************************** */
// this is maximal value of required access field in usg bits, it is index
// into the table s_requiredeAccessTable

#define MIN_REQUIRED_ACCESS 0
#define MAX_REQUIRED_ACCESS 3
#define MAX_REQUIRED_ACCESS_LN 2

/* *********************************************************************** */

#define JAVA_NULL_CODE 'N'
#define JAVA_CONSTRUCTOR_NAME1 "<init>"
#define JAVA_CONSTRUCTOR_NAME2 "<clinit>"

/* ********************** code inspection state bits ********************* */

#define INSP_VISITED	 		1
#define INSP_INSIDE_BLOCK	  	2
#define INSP_OUTSIDE_BLOCK  	4
#define INSP_INSIDE_REENTER  	8		/* value reenters the block 			*/
#define INSP_INSIDE_PASSING  	16		/* a non-modified values pass via block */

/* *******************   Object-Oriented Resolutions   ******************* */

/* visibilities for completion */

#define OOC_LINKAGE_CHECK			00001 /* static / nostatic    */
#define OOC_ACCESS_CHECK			00002 /* private/public/auto  */
#define OOC_VISIBILITY_CHECK		00004 /* private inheritance  */

#define OOC_ALL_CHECKS				00010 /* accessibility in fqt names  */


/* virtual resolution */

#define OOC_VIRTUAL_MASK				00700 /* mask for virtual resolution */
#define OOC_VIRT_ANY					00000 /* no matter the class */
#define OOC_VIRT_SUBCLASS_OF_RELATED	00100 /* sub. of a related class */
#define OOC_VIRT_RELATED				00200 /* + related to browsed applic.*/
#define OOC_VIRT_SAME_FUN_CLASS			00300 /* + having the same fun class */
#define OOC_VIRT_APPLICABLE				00400 /* + virtually applicable */
#define OOC_VIRT_SAME_APPL_FUN_CLASS	00500 /* + having the same class */

/* profile resolution */

#define OOC_PROFILE_MASK			07000 /* mask for profile resolution */
#define OOC_PROFILE_ANY				00000 /* no matter the profile */
#define OOC_PROFILE_ARITY			01000 /* same number of arguments */
#define OOC_PROFILE_APPLICABLE		02000 /* + applicable profile */
#define OOC_PROFILE_EQUAL			03000 /* + no conversion meeded */

/* when symbol is selected */

//#define DEFAULT_SELECTION_OO_BITS (OOC_VIRT_APPLICABLE | OOC_PROFILE_APPLICABLE)
#define DEFAULT_SELECTION_OO_BITS (OOC_VIRT_SAME_FUN_CLASS | OOC_PROFILE_APPLICABLE)
#define RENAME_SELECTION_OO_BITS (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE)
#define METHOD_VIA_SUPER_SELECTION_OO_BITS (OOC_VIRT_SAME_APPL_FUN_CLASS | OOC_PROFILE_EQUAL)

/* *********************************************************************** */

#define MAP_FUN_PROFILE char *file, char *a1,char *a2,S_completions *a3,void *a4,int *a5

/* *********************************************************************** */

#endif	/* ifndef _HEAD__H*/



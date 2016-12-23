/*
	$Revision: 1.101 $
	$Date: 2002/09/07 17:21:50 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "unigram.h"
#include "recyacc.h"

#include "protocol.h"
//
#define NON_FILE_NAME "___None___"

static char olExtractAddrParPrefixStatChar[TMP_STRING_SIZE];
static char oldStdopFile[MAX_FILE_NAME_SIZE];
static char oldStdopSection[MAX_FILE_NAME_SIZE];
static char oldOnLineClassPath[MAX_OPTION_LEN];
static time_t oldStdopTime;
static int oldLanguage;
static int oldCppPass;
static char qnxMsgBuff[QNX_MSG_BUF_SIZE];
static S_options s_tmpOptions;

static void usage(char *s) {
	fprintf(stdout,"usage: \t\t%s <option>+ ",s);
	fprintf(stdout,"<input files>");
	fprintf(stdout,"\n");
	fprintf(stdout,"options:\n");
	fprintf(stdout,"\t-r                    - recursively descent directories\n");
	fprintf(stdout,"\t-html                 - convert sources to html format\n");
	fprintf(stdout,"\t-htmlroot=<dir>       - specifies root dir. for html output\n");
	fprintf(stdout,"\t-htmltab=<n>          - set tabulator to <n> in htmls\n");
	fprintf(stdout,"\t-htmlgxlist           - generate xref-lists for global symbols\n");
	fprintf(stdout,"\t-htmllxlist           - generate xref-lists for local symbols\n");
	fprintf(stdout,"\t-htmldirectx          - link first character to cross references\n");
	fprintf(stdout,"\t-htmlfunseparate      - separate functions by a bar\n");
	fprintf(stdout,"\t-htmlzip=<command>    - zip command, ! stands for the file name\n");
	fprintf(stdout,"\t-htmllinksuffix=<suf> - add this suffix to file links\n");
	fprintf(stdout,"\t-htmllinenums         - generate line numbers in HTML\n");
	fprintf(stdout,"\t-htmlnocolors         - do not generate colors in HTML\n");
	fprintf(stdout,"\t-htmlnounderline      - not underlined HTML links\n");
	fprintf(stdout,"\t-htmlcutpath=<path>   - cut path in generated html\n");
	fprintf(stdout,"\t-htmlcutcwd           - cut current dir in html\n");
	fprintf(stdout,"\t-htmlcutsourcepaths   - cut source paths in html\n");
	fprintf(stdout,"\t-htmlcutsuffix        - cut language suffix in html\n");
	fprintf(stdout,"\t-htmllinkcolor=<color>- color of HTML links\n");
	fprintf(stdout,"\t-htmllinenumcolor=<color>- color of line numbers in HTML links\n");
	fprintf(stdout,"\t-htmlgenjavadoclinks  - gen links to javadoc if no definition\n");
	fprintf(stdout,"\t                      - only for packages from -htmljavadocavailable\n");
	fprintf(stdout,"\t-javadocurl=<http>    - url to existing java docs\n");
	fprintf(stdout,"\t-javadocpath=<path>   - paths to existing java docs\n");
	fprintf(stdout,"\t-javadocavailable=<packs> - packages for which javadoc is available\n");
	fprintf(stdout,"\t-p <prj>              - read options from <prj> section\n");
	fprintf(stdout,"\t-I <dir>              - search for includes in <dir>\n");
	fprintf(stdout,"\t-D<mac>[=<body>]      - define macro <mac> with body <body>\n");
	fprintf(stdout,"\t-packages             - allow packages as input files\n");
	fprintf(stdout,"\t-sourcepath <path>    - set java sources paths\n");
	fprintf(stdout,"\t-classpath <path>     - set java class path\n");
	fprintf(stdout,"\t-filescasesensitive   - file names are case sensitive\n");
	fprintf(stdout,"\t-filescaseunsensitive - file names are case unsensitive\n");
	fprintf(stdout,"\t-csuffixes=<paths>    - list of C files suffixes separated by : (or ;)");
	fprintf(stdout,"\t-javasuffixes=<paths> - list of Java files suffixes separated by : (or ;)\n");
	fprintf(stdout,"\t-stdop <file>         - read options from <file>\n");
	fprintf(stdout,"\t-no_cpp_comment       - C++ like comments // not admitted\n");
	fprintf(stdout,"\t-license=<string>     - license string\n");
#if 0
	fprintf(stdout,"\t-olinelen=<n>         - length of lines for on-line output\n");
	fprintf(stdout,"\t-oocheckbits=<n>      - object-oriented resolution for completions\n");
	fprintf(stdout,"\t-olcxsearch           - search info about identifier\n");
	fprintf(stdout,"\t-olcxpush             - generate and push on-line cxrefs \n");
	fprintf(stdout,"\t-olcxrename           - generate and push xrfs for rename\n");
	fprintf(stdout,"\t-olcxlist             - generate, push and list on-line cxrefs \n");
	fprintf(stdout,"\t-olcxpop              - pop on-line cxrefs\n");	
	fprintf(stdout,"\t-olcxplus             - next on-line reference\n");	
	fprintf(stdout,"\t-olcxminus            - previous on-line reference\n");	
	fprintf(stdout,"\t-olcxgoto<n>          - go to the n-th on-line reference\n");
	fprintf(stdout,"\t-user                 - user logname for olcx\n");
	fprintf(stdout,"\t-o <file>             - put output to file \n");
	fprintf(stdout,"\t-qnxmsg               - xrefsrv will receives QNX mesages\n");
	fprintf(stdout,"\t-file <file>          - name of the file given to stdin\n");
#endif
	fprintf(stdout,"\t-refs <file>          - name of file with cxrefs\n");
	fprintf(stdout,"\t-refnum=<n>           - number of cxref files\n");
	fprintf(stdout,"\t-refalphahash         - split references alphabetically (-refnum=28)\n");
	fprintf(stdout,"\t-refalpha2hash        - split references alphabetically (-refnum=28*28)\n");
	fprintf(stdout,"\t-exactpositionresolve - resolve symbols by def. position\n");
	fprintf(stdout,"\t-mf<n>                - factor increasing cxMemory\n");
#	ifdef DEBUG
	fprintf(stdout,"\t-debug                - produce debug trace of the execution\n");
#	endif
#if 0
	fprintf(stdout,"\t-typedefs        - generate structure/enums typedefs\n");
	fprintf(stdout,"\t-str_fill        - generate structure fills\n");
	fprintf(stdout,"\t-enum_name       - generate enum text names\n");
	fprintf(stdout,"\t-str_copy        - generate structure copy functions\n");
	fprintf(stdout,"\t-header          - output is headers file\n");
	fprintf(stdout,"\t-body            - output is code file\n");
#endif
	fprintf(stdout,"\t-no_enum         - don't cross reference enumeration constants\n");
	fprintf(stdout,"\t-no_mac          - don't cross reference macros\n");
	fprintf(stdout,"\t-no_type         - don't cross reference type names\n");
	fprintf(stdout,"\t-no_str          - don't cross reference str. records\n");
	fprintf(stdout,"\t-no_local        - don't cross reference local vars\n");
	fprintf(stdout,"\t-nobrief         - generate cxrefs in long format\n");
	fprintf(stdout,"\t-update          - update old 'refs' reference file\n");
	fprintf(stdout,"\t-fastupdate      - fast update (modified files only)\n");
//&	fprintf(stdout,"\t-keep_old        - keep also all old references from 'refs' file\n");
	fprintf(stdout,"\t-no_stdop        - don't read the '~/.c-xrefrc' option file \n");
	fprintf(stdout,"\t-errors          - report all error messages\n");
	fprintf(stdout,"\t-version         - give this release version number\n");
}

static void aboutMessage() {
	time_t tt;
	int j;
	char ttt[REFACTORING_TMP_STRING_SIZE];
	sprintf(ttt,"This is C-xrefactory version %s (%s).\n", C_XREF_VERSION_NUMBER, __DATE__);
#if ZERO  // since 1.6.8 it is free
#if defined(TIME_LIMITED) && ! defined(BIN_RELEASE)
	tt = EXPIRATION;
	sprintf(ttt+strlen(ttt),"Time limit: %s",ctime(&tt));
#else
#if defined(BIN_RELEASE)
	setExpirationFromLicenseString();
	if (s_expTime!=((time_t)-1)) {
		sprintf(ttt+strlen(ttt),"Licensed until %s",ctime(&s_expTime));
	}
#else
/*&
  for(j=0; s_licensement[j]; j++) tmpBuff[j]=s_licensement[j]^7;
  tmpBuff[j]=0;
  fprintf(stdout,"%s\n",tmpBuff);
&*/
#endif
#endif
#endif
//&	fprintf(stdout,"The newest version of this software can be\n");
//&	fprintf(stdout,"purchased at the address 'http://www.xref-tech.com'.\n");
	sprintf(ttt+strlen(ttt),"(c) 1997-2004 by Xref-Tech, http://www.xref-tech.com\n");
	if (s_opt.exit) {
		sprintf(ttt+strlen(ttt),"Exiting!");
	}
	if (s_opt.xref2) {
		ppcGenRecord(PPC_INFORMATION, ttt, "\n");
	} else {
		fprintf(stdout, "%s", ttt);
	}
	if (s_opt.exit) exit(XREF_EXIT_BASE);
}

#define NEXT_FILE_ARG() {\
	i++;\
	if (i >= argc) { \
		sprintf(tmpBuff,"file name expected after %s\n",argv[i-1]); \
		error(ERR_ST,tmpBuff);\
		usage(argv[0]);\
	}\
}

#define NEXT_ARG() {\
	i++;\
	if (i >= argc) { \
		sprintf(tmpBuff,"further argument(s) expected after %s\n",argv[i-1]); \
		error(ERR_ST,tmpBuff);\
		usage(argv[0]);\
	}\
}

static int fileNameShouldBePruned(char *fn) {
	S_stringList	*pp;
	for(pp=s_opt.pruneNames; pp!=NULL; pp=pp->next) {
		JavaMapOnPaths(pp->d, {
			if (fnCmp(currentPath, fn)==0) return(1);
		});
	}
	return(0);
}

static void scheduleCommandLineEnteredFileToProcess(char *fn) {
	int ii;
	addFileTabItem(fn,&ii);
	if (s_opt.taskRegime!=RegimeEditServer) {
		// yes in edit server you processa also headers, etc.
		s_fileTab.tab[ii]->b.commandLineEntered = 1;
	}
//&fprintf(dumpOut,"recursively command line file %s\n",s_fileTab.tab[ii]->name);fflush(dumpOut);
	if (s_opt.updateOnlyModifiedFiles==0) {
		s_fileTab.tab[ii]->b.scheduledToProcess = 1;
	}
}

void dirInputFile(MAP_FUN_PROFILE) {
	char 			*dir,*fname,*fin,*suff, *wcp;
	void			*recurseFlag;
	void			*nrecurseFlag;
	struct stat 	st;
	char			fn[MAX_FILE_NAME_SIZE];
	char			ttt[MAX_FILE_NAME_SIZE];
	char 			wcPaths[MAX_OPTION_LEN];
	int				ii,len,topCallFlag,stt, wclen;
	dir = a1; fname = file; recurseFlag = a4; topCallFlag = *a5;
	if (topCallFlag == 0) {
		if (strcmp(fname,".")==0) return;
		if (strcmp(fname,"..")==0) return;
		if (fileNameShouldBePruned(fname)) return;
		sprintf(fn,"%s%c%s",dir,SLASH,fname);
		strcpy(fn, normalizeFileName(fn, s_cwd));
		if (fileNameShouldBePruned(fn)) return;
	} else {
		strcpy(fn, normalizeFileName(fname, s_cwd));
	}
	if (strlen(fn) >= MAX_FILE_NAME_SIZE) {
		sprintf(tmpBuff, "file name %s is too long", fn);
		fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
	}
	suff = getFileSuffix(fname);
	stt = statb(fn, &st);
	if (stt==0  && (st.st_mode & S_IFMT)==S_IFDIR) {
		if (recurseFlag!=NULL) {
			topCallFlag = 0;
			if (s_opt.recursivelyDirs) nrecurseFlag = &topCallFlag;
			else nrecurseFlag = NULL;
			mapDirectoryFiles(fn, dirInputFile,DO_NOT_ALLOW_EDITOR_FILES, 
							  fn, NULL, NULL, nrecurseFlag, &topCallFlag);
			editorMapOnNonexistantFiles(fn, dirInputFile, DEEP_ANY, 
							  fn, NULL, NULL, nrecurseFlag, &topCallFlag);
		} else {
			// no error, let it be
			//& sprintf(tmpBuff,"omitting directory %s, missing '-r' option ?",fn);
			//& warning(ERR_ST,tmpBuff);
		}
	} else if (stt==0) {
		// .class can be inside a jar archiv, but this makes problem on 
		// recursive read of a directory, it attempts to read .class
		if (	topCallFlag==0 
				&&	(! fileNameHasOneOfSuffixes(fname, s_opt.cFilesSuffixes))
				&& 	(! fileNameHasOneOfSuffixes(fname, s_opt.javaFilesSuffixes))
#ifdef CCC_ALLOWED
				&& 	(! fileNameHasOneOfSuffixes(fname, s_opt.cppFilesSuffixes))
#endif
#ifdef YACC_ALLOWED
				&& 	fnCmp(suff,".y")!=0
#endif
			) {
			return;
		}
		if (s_opt.javaFilesOnly && s_opt.taskRegime != RegimeEditServer
			&& (! fileNameHasOneOfSuffixes(fname, s_opt.javaFilesSuffixes))
			&& (! fileNameHasOneOfSuffixes(fname, "jar:class"))
			) {
			return;
		}
		scheduleCommandLineEnteredFileToProcess(fn);
	} else if (containsWildCharacter(fn)) {
		expandWildCharactersInOnePath(fn, wcPaths, MAX_OPTION_LEN);
//&fprintf(dumpOut, "wild char path %s expanded to %s\n", fn, wcPaths);
		JavaMapOnPaths(wcPaths,{
			dirInputFile(currentPath,"",NULL,NULL,recurseFlag,&topCallFlag);
			});
#if ZERO 
#ifdef __WIN32__				/*SBD*/
	} else if (strchr(fn,'*')!=NULL) {
		copyDir(ttt, fn, &ii);
		mapPatternFiles(fn, dirInputFile, ttt, 
						NULL, NULL, recurseFlag, &topCallFlag);
#endif          				/*SBD*/
#endif
	} else if (topCallFlag
			   && ((!s_opt.allowPackagesOnCl)
				   || packageOnCommandLine(fname)==0)) {
		if (s_opt.taskRegime!=RegimeEditServer) {
			error(ERR_CANT_OPEN, fn);
		} else {
			// hacked 16.4.2003 in order to can complete in buffers
			// without existing file
			scheduleCommandLineEnteredFileToProcess(fn);
		}
	}
}

static void optionAddToAllocatedList(char **dest) {
	S_stringAddrList *ll;
	for(ll=s_opt.allAllocatedStrings; ll!=NULL; ll=ll->next) {
		// reassignement, do not keep two copies
		if (ll->d == dest) break;
	}
	if (ll==NULL) {
		OPT_ALLOC(ll, S_stringAddrList);
		FILL_stringAddrList(ll, dest, s_opt.allAllocatedStrings);
		s_opt.allAllocatedStrings = ll;
	}
}

void allocOptionSpace(void **optAddress, int size) {
	char **res;
	res = (char**)optAddress;
	OPT_ALLOCC((*res), size, char);
	optionAddToAllocatedList(res);	
}

void crOptionStr(char **optAddress, char *text) {
	int alen = strlen(text)+1;
	allocOptionSpace((void**)optAddress, alen);
	strcpy(*optAddress, text);
}

static void copyOptionShiftPointer(char **lld, S_options *dest, S_options *src) {
	char	*str, **dlld;
	int 	offset, localOffset;
	offset = ((char*)dest) - ((char*)src);
	localOffset = ((char*)lld) - ((char*)src);
	dlld = ((char**) (((char*)dest) + localOffset));
	// dlld is dest equivalent of *lld from src
//&fprintf(dumpOut,"shifting (%x->%x) [%x]==%x ([%x]==%x), offsets == %d, %d, size==%d\n", src, dest, lld, *lld, dlld, *dlld, offset, localOffset, sizeof(S_options)); 
	if (*dlld != *lld) {
		fprintf(dumpOut,"problem %s\n", *lld);
	}
	assert(*dlld == *lld);
	*dlld = *lld + offset;
}

void copyOptions(S_options *dest, S_options *src) {
	S_stringAddrList 	**ll;
	memcpy(dest, src, sizeof(S_options));
	for(ll= &src->allAllocatedStrings; *ll!=NULL; ll = &(*ll)->next) {
		copyOptionShiftPointer((*ll)->d, dest, src);
		copyOptionShiftPointer(((char**)&(*ll)->d), dest, src);
		copyOptionShiftPointer(((char**)ll), dest, src);
	}
//&fprintf(dumpOut,"options copied\n");
}

void xrefSetenv(char *name, char *val) {
	S_setGetEnv *sge;
	int j, n;
	sge = &s_opt.setGetEnv;
	n = sge->num;
	if (n+1>=MAX_SET_GET_OPTIONS) {
		sprintf(tmpBuff, "maximum of %d -set options reached", MAX_SET_GET_OPTIONS);
		error(ERR_ST, tmpBuff);
		sge->num--; n--;
	}
	for(j=0; j<n; j++) {
		assert(sge->name[j]);
		if (strcmp(sge->name[j], name)==0) break;
	}
	if (j==n) crOptionStr(&(sge->name[j]), name);
	if (j==n || strcmp(sge->value[j], val)!=0) {
		crOptionStr(&(sge->value[j]), val);
	}
//&fprintf(dumpOut,"setting '%s' to '%s'\n", name, val);
	if (j==n) sge->num ++;
}


int mainHandleSetOption( int argc, char **argv, int i ) {
	S_setGetEnv *sge;
	char *name, *val;
	int j,n;
	NEXT_ARG();
	name = argv[i];
	assert(name);
	NEXT_ARG();
	val = argv[i];
	xrefSetenv(name, val);
	return(i);
}

static int mainHandleIncludeOption(int argc, char **argv, int i) {
	int nargc,aaa; 
	char **nargv;
	NEXT_FILE_ARG();
	s_opt.stdopFlag = 1;
	readOptionFile(argv[i],&nargc,&nargv,"",NULL);
	processOptions(nargc, nargv, INFILES_DISABLED);
	s_opt.stdopFlag = 0;
	return(i);
}

int addHtmlCutPath(char *ss ) {
	int i,ln,len, res;
	res = 0;
	ss = htmlNormalizedPath(ss);
	ln = strlen(ss);
	if (ln>=1 && ss[ln-1] == SLASH) {
		warning(ERR_ST,"slash at the end of -htmlcutpath path, ignoring it");
		return(res);
	}
	for(i=0; i<s_opt.htmlCut.pathesNum; i++) {
		// if yet in cutpathes, do nothing
		if (strcmp(s_opt.htmlCut.path[i], ss)==0) return(res);
	}
	crOptionStr(&(s_opt.htmlCut.path[s_opt.htmlCut.pathesNum]), ss);
	ss = s_opt.htmlCut.path[s_opt.htmlCut.pathesNum];
	s_opt.htmlCut.plen[s_opt.htmlCut.pathesNum] = ln;
//&fprintf(dumpOut,"adding cutpath %d %s\n",s_opt.htmlCut.pathesNum,ss);
	for(i=0; i<s_opt.htmlCut.pathesNum; i++) {
		// a more specialized path after a more general, exchange them
		len = s_opt.htmlCut.plen[i];
		if (fnnCmp(s_opt.htmlCut.path[i], ss, len)==0) {
			sprintf(tmpBuff,
					"htmlcutpath: '%s' supersede \n\t\t'%s', exchanging them",
					s_opt.htmlCut.path[i], ss);
			warning(ERR_ST, tmpBuff);
			res = 1;
			s_opt.htmlCut.path[s_opt.htmlCut.pathesNum]=s_opt.htmlCut.path[i];
			s_opt.htmlCut.plen[s_opt.htmlCut.pathesNum]=s_opt.htmlCut.plen[i];
			s_opt.htmlCut.path[i] = ss;
			s_opt.htmlCut.plen[i] = ln;
		}
	}
	if (s_opt.htmlCut.pathesNum+2 >= MAX_HTML_CUT_PATHES) {
		error(ERR_ST,"# of htmlcutpathes overflow over MAX_HTML_CUT_PATHES");
	} else {
		s_opt.htmlCut.pathesNum++;
	}
	return(res);
}

/* *************************************************************************** */
/*                                      OPTIONS                                */

static int processNegativeOption(int *ii, int argc, char **argv, int infilesFlag) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"--r")==0) {
		if (infilesFlag == INFILES_ENABLED) s_opt.recursivelyDirs = 0;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processAOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strncmp(argv[i],"-addimportdefault=",18)==0) {
		sscanf(argv[i]+18, "%d", &s_opt.defaultAddImportStrategy);
	}
	else if (strcmp(argv[i],"-version")==0 || strcmp(argv[i],"-about")==0){
		s_opt.cxrefs = OLO_ABOUT;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processBOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-brief")==0) 			s_opt.brief = 1;
	else if (strcmp(argv[i],"-briefoutput")==0) 	s_opt.briefoutput = 1;
	else if (strcmp(argv[i],"-body")==0) 			s_opt.body = 1;
	else if (strncmp(argv[i],"-browsedsym=",12)==0) 	{
		crOptionStr(&s_opt.browsedSymName, argv[i]+12);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processCOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-c_struct_scope")==0)	s_opt.c_struct_scope=1;
	else if (strcmp(argv[i],"-cacheincludes")==0)	s_opt.cacheIncludes=1;
	else if (strcmp(argv[i],"-cplusplus")==0)		s_opt.cIsCplusplus=1;
	else if (strcmp(argv[i],"-crlfconversion")==0)	s_opt.eolConversion|=CR_LF_EOL_CONVERSION;
	else if (strcmp(argv[i],"-crconversion")==0)	s_opt.eolConversion|=CR_EOL_CONVERSION;
	else if (strcmp(argv[i],"-completioncasesensitive")==0)	s_opt.completionCaseSensitive=1;
	else if (strcmp(argv[i],"-completeparenthesis")==0)	s_opt.completeParenthesis=1;
	else if (strncmp(argv[i],"-completionoverloadwizdeep=",27)==0)	{
		sscanf(argv[i]+27, "%d", &s_opt.completionOverloadWizardDeep);
	}
	else if (strcmp(argv[i],"-continuerefactoring")==0)	s_opt.continueRefactoring=RC_CONTINUE;
	else if (strncmp(argv[i],"-commentmovinglevel=",20)==0)	{
		sscanf(argv[i]+20, "%d", &s_opt.commentMovingLevel);
	}
	else if (strcmp(argv[i],"-continuerefactoring=importSingle")==0)	{
		s_opt.continueRefactoring = RC_IMPORT_SINGLE_TYPE;
	}
	else if (strcmp(argv[i],"-continuerefactoring=importOnDemand")==0)	{
		s_opt.continueRefactoring = RC_IMPORT_ON_DEMAND;
	}
	else if (strcmp(argv[i],"-classpath")==0) {
		NEXT_FILE_ARG();
#if ZERO
		if (s_opt.classpath!=NULL && *s_opt.classpath!=0 
			&& strcmp(s_opt.classpath,argv[i])!=0) {
			if (s_opt.taskRegime != RegimeEditServer) {
				warning(ERR_ST,"redefinition of classpath, new value ignored");
			}
		} else {
#endif
			crOptionStr(&s_opt.classpath, argv[i]);
#if ZERO
		}
#endif
	}
	else if (strncmp(argv[i],"-csuffixes=",11)==0) {
		crOptionStr(&s_opt.cFilesSuffixes, argv[i]+11);
	}
	else if (strcmp(argv[i],"-create")==0) 		s_opt.create = 1;
	else return(0);
	*ii = i;
	return(1);
}

static int processDOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
#	ifdef DEBUG
	else if (strcmp(argv[i],"-debug")==0) 		s_opt.debug = 1;
#	endif
	else if (strcmp(argv[i],"-d")==0)	{
		int ln;
		NEXT_FILE_ARG();
		ln=strlen(argv[i]);
		if (ln>1 && argv[i][ln-1] == SLASH) {
			warning(ERR_ST,"slash at the end of -d path");
		}
		if (! isAbsolutePath(argv[i])) {
			warning(ERR_ST,"'-d' option should be followed by an ABSOLUTE path!");
		}
		crOptionStr(&s_opt.htmlRoot, argv[i]);
	}
	// TODO, do this macro allocation differently!!!!!!!!!!!!!
	// just store macros in options and later add them into pp_memory 
	else if (strncmp(argv[i],"-D",2)==0) addMacroDefinedByOption(argv[i]+2);
	else if (strcmp(argv[i],"-displaynestedwithouters")==0) {
		s_opt.nestedClassDisplaying = NO_OUTERS_CUT;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processEOption(int *ii, int argc, char **argv) {
	int i = * ii;
	char ttt[TMP_STRING_SIZE];
	int alen;
	if (0) {}
	else if (strcmp(argv[i],"-errors")==0) 		s_opt.err = 1;
	else if (strcmp(argv[i],"-exit")==0) 		s_opt.exit = 1;
	else if (strcmp(argv[i],"-enum_name")==0) s_opt.enum_name = 1;
	else if (strcmp(argv[i],"-editor=emacs")==0) {
		s_opt.editor = ED_EMACS;
	}
	else if (strcmp(argv[i],"-editor=jedit")==0) {
		s_opt.editor = ED_JEDIT;
	}
	else if (strcmp(argv[i],"-emacs")==0) {
		// obsolete
		s_opt.editor = ED_EMACS;
	}
	else if (strncmp(argv[i],"-extractAddrParPrefix=",22)==0) {
		sprintf(ttt, "*%s", argv[i]+22);
		crOptionStr(&s_opt.olExtractAddrParPrefix, ttt);
#if ZERO
		olExtractAddrParPrefixStatChar[0]='*';
		strcpy(olExtractAddrParPrefixStatChar+1, argv[i]+22);
		s_opt.olExtractAddrParPrefix = olExtractAddrParPrefixStatChar;
#endif
	}
	else if (strcmp(argv[i],"-exactpositionresolve")==0) {
		s_opt.exactPositionResolve = 1;
	}
	else if (strncmp(argv[i],"-encoding=", 10)==0) {
		if (s_opt.fileEncoding == MULE_DEFAULT) {
			if (strcmp(argv[i],"-encoding=default")==0) {
				s_opt.fileEncoding = MULE_DEFAULT;
			} else if (strcmp(argv[i],"-encoding=european")==0) {
				s_opt.fileEncoding = MULE_EUROPEAN;
			} else if (strcmp(argv[i],"-encoding=euc")==0) {
				s_opt.fileEncoding = MULE_EUC;
			} else if (strcmp(argv[i],"-encoding=sjis")==0) {
				s_opt.fileEncoding = MULE_SJIS;
			} else if (strcmp(argv[i],"-encoding=utf")==0) {
				s_opt.fileEncoding = MULE_UTF;
			} else if (strcmp(argv[i],"-encoding=utf-8")==0) {
				s_opt.fileEncoding = MULE_UTF_8;
			} else if (strcmp(argv[i],"-encoding=utf-16")==0) {
				s_opt.fileEncoding = MULE_UTF_16;
			} else if (strcmp(argv[i],"-encoding=utf-16le")==0) {
				s_opt.fileEncoding = MULE_UTF_16LE;
			} else if (strcmp(argv[i],"-encoding=utf-16be")==0) {
				s_opt.fileEncoding = MULE_UTF_16BE;
			} else {
				sprintf(tmpBuff,"unsupported encoding, available values are 'default', 'european', 'euc', 'sjis', 'utf', 'utf-8', 'utf-16', 'utf-16le' and 'utf-16be'.");
				formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
				error(ERR_ST, tmpBuff);
			}
		}
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processFOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-filescasesensitive")==0) {
		s_opt.fileNamesCaseSensitive = 1;
	}
	else if (strcmp(argv[i],"-filescaseunsensitive")==0) {
		s_opt.fileNamesCaseSensitive = 0;
	}
	else if (strcmp(argv[i],"-fastupdate")==0) 	{
		s_opt.update = UP_FAST_UPDATE;
		s_opt.updateOnlyModifiedFiles = 1;
	}
	else if (strcmp(argv[i],"-fupdate")==0) {
		s_opt.update = UP_FULL_UPDATE;
		s_opt.updateOnlyModifiedFiles = 0;
	}
#ifdef ZERO
	else if (strcmp(argv[i],"-file")==0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.lineFileName, argv[i]);
	}
#endif
	else return(0);
	*ii = i;
	return(1);
}

static int processGOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-getlastimportline")==0) {
		s_opt.trivialPreCheckCode = TPC_GET_LAST_IMPORT_LINE;
	}
	else if (strcmp(argv[i],"-get")==0) {
		NEXT_ARG();
		crOptionStr(&s_opt.getValue, argv[i]);
		s_opt.cxrefs = OLO_GET_ENV_VALUE;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processHOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (strncmp(argv[i],"-htmltab=",9)==0)	{
		sscanf(argv[i]+9,"%d",&s_opt.tabulator);
	}
	else if (strcmp(argv[i],"-htmllinenums")==0)	s_opt.htmlLineNums = 1;
	else if (strcmp(argv[i],"-htmlnocolors")==0)	s_opt.htmlNoColors = 1;
	else if (strcmp(argv[i],"-htmlgxlist")==0)	s_opt.htmlglobalx = 1;
	else if (strcmp(argv[i],"-htmllxlist")==0) 	s_opt.htmllocalx = 1;
	else if (strcmp(argv[i],"-htmlrichlist")==0) 	{
		warning(ERR_ST,"-htmlrichlist option is no longer supported");
//&			s_opt.htmlRichLists= 1;
	}
	else if (strcmp(argv[i],"-htmlfunseparate")==0)s_opt.htmlFunSeparate=1;
	else if (strcmp(argv[i],"-html")==0) {
		s_opt.taskRegime = RegimeHtmlGenerate;
		s_opt.fileEncoding = MULE_EUROPEAN; // no multibyte encodings
		s_opt.multiHeadRefsCare = 0;	// JUST TEMPORARY !!!!!!!
	}
	else if (strncmp(argv[i],"-htmlroot=",10)==0)	{
		int ln;
		ln=strlen(argv[i]);
		if (ln>11 && argv[i][ln-1] == SLASH) {
			warning(ERR_ST,"slash at the end of -htmlroot path");
		}
		crOptionStr(&s_opt.htmlRoot, argv[i]+10);
	}
	else if (strcmp(argv[i],"-htmlgenjavadoclinks")==0)	{
		s_opt.htmlGenJdkDocLinks = 1;
	}
	else if (strncmp(argv[i],"-htmljavadocavailable=",22)==0) {
		crOptionStr(&s_opt.htmlJdkDocAvailable, argv[i]+22);
	} 
	else if (strncmp(argv[i],"-htmljavadocpath=",17)==0)	{
		crOptionStr(&s_opt.htmlJdkDocUrl, argv[i]+17);
	}
	else if (strncmp(argv[i],"-htmlcutpath=",13)==0)	{
		addHtmlCutPath(argv[i]+13);
	}
	else if (strcmp(argv[i],"-htmlcutcwd")==0)	{
		int tmp;
		tmp = addHtmlCutPath(s_cwd);
		if (s_opt.java2html && tmp) {
			fatalError(ERR_ST, "a path clash occurred, please, run java2html from different directory", XREF_EXIT_ERR);
		}
	}
	else if (strcmp(argv[i],"-htmlcutsourcepaths")==0)	{
		addSourcePathesCut();
	}
	else if (strcmp(argv[i],"-htmlcutsourcepathes")==0)	{
		addSourcePathesCut();
	}
	else if (strcmp(argv[i],"-htmlcutsuffix")==0)	{
		s_opt.htmlCutSuffix = 1;
	}
	else if (strncmp(argv[i],"-htmllinenumlabel=", 18)==0)	{
		crOptionStr(&s_opt.htmlLineNumLabel, argv[i]+18);
	}
	else if (strcmp(argv[i],"-htmlnounderline")==0)	{
		s_opt.htmlNoUnderline = 1;
	}
	else if (strcmp(argv[i],"-htmldirectx")==0)	{
		s_opt.htmlDirectX = 1;
	}
	else if (strncmp(argv[i],"-htmllinkcolor=",15)==0)	{
		crOptionStr(&s_opt.htmlLinkColor, argv[i]+15);
	}
	else if (strncmp(argv[i],"-htmllinenumcolor=",18)==0)	{
		crOptionStr(&s_opt.htmlLineNumColor, argv[i]+18);
	}
	else if (strncmp(argv[i],"-htmlcxlinelen=",15)==0)	{
		sscanf(argv[i]+15, "%d", &s_opt.htmlCxLineLen);
	}
	else if (strncmp(argv[i],"-htmlzip=",9)==0)	{
		crOptionStr(&s_opt.htmlZipCommand, argv[i]+9);
	}
	else if (strncmp(argv[i],"-htmllinksuffix=",16)==0)	{
		crOptionStr(&s_opt.htmlLinkSuffix, argv[i]+16);
	}
	else if (strcmp(argv[i],"-header")==0) 		s_opt.header = 1;
	else if (strcmp(argv[i],"-help")==0) {
		usage(argv[0]);
		exit(0);
	}
	else return(0);
	*ii = i;
	return(1);
}

static void mainAddStringListOption(S_stringList **optlist, char *argvi) {
	S_stringList **ll, *list;
	for(ll=optlist; *ll!=NULL; ll= &(*ll)->next) ;
	allocOptionSpace((void**)ll, sizeof(S_stringList));
	crOptionStr(&(*ll)->d, argvi);
	(*ll)->next = NULL;
}

static int processIOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-I")==0) {
		/* include dir */
		i++;
		if (i >= argc) { 
			sprintf(tmpBuff,"directory name expected after -I\n"); 
			error(ERR_ST,tmpBuff);
			usage(argv[0]);
		}
		mainAddStringListOption(&s_opt.includeDirs, argv[i]);
	} 
	else if (strncmp(argv[i],"-I", 2)==0 && argv[i][2]!=0) {
		mainAddStringListOption(&s_opt.includeDirs, argv[i]+2);
	}
	else if (strcmp(argv[i],"-include")==0) {
		warning(ERR_ST,"-include option is depreciated, use -optinclude instead");
		i = mainHandleIncludeOption(argc, argv, i);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processJOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-javadoc")==0)	    s_opt.javaDoc = 1;
	else if (strcmp(argv[i],"-java2html")==0)	s_opt.java2html = 1;
	else if (strcmp(argv[i],"-java1.4")==0)		{
		crOptionStr(&s_opt.javaVersion, JAVA_VERSION_1_4);
	}
	else if (strncmp(argv[i],"-jdoctmpdir=",12)==0)	{
		int ln;
		ln=strlen(argv[i]);
		if (ln>13 && argv[i][ln-1] == SLASH) {
			warning(ERR_ST,"slash at the end of -jdoctmpdir path");
		}
		crOptionStr(&s_opt.jdocTmpDir, argv[i]+12);
	}
	else if (strncmp(argv[i],"-javadocavailable=",18)==0)	{
		crOptionStr(&s_opt.htmlJdkDocAvailable, argv[i]+18);
	}
	else if (strncmp(argv[i],"-javadocurl=",12)==0)	{
		crOptionStr(&s_opt.htmlJdkDocUrl, argv[i]+12);
	}
	else if (strncmp(argv[i],"-javadocpath=",13)==0)	{
		crOptionStr(&s_opt.javaDocPath, argv[i]+13);
	} else if (strcmp(argv[i],"-javadocpath")==0)	{
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.javaDocPath, argv[i]);
	}
	else if (strncmp(argv[i],"-javasuffixes=",14)==0) {
		crOptionStr(&s_opt.javaFilesSuffixes, argv[i]+14);
	}
	else if (strcmp(argv[i],"-javafilesonly")==0) {
		s_opt.javaFilesOnly = 1;
	}
	else if (strcmp(argv[i],"-jdkclasspath")==0 || strcmp(argv[i],"-javaruntime")==0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.jdkClassPath, argv[i]);
	}
	else if (strcmp(argv[i],"-jeditc1")==0) {
		s_opt.jeditOldCompletions = 1;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processKOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-keep_old")==0) 	s_opt.keep_old = 1;
	else return(0);
	*ii = i;
	return(1);
}

static int processLOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strncmp(argv[i],"-last_message=",14)==0) {
		crOptionStr(&s_opt.last_message, argv[i]+14);
	} 
	else return(0);
	*ii = i;
	return(1);
}

static int processMOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strncmp(argv[i],"-mf",3)==0)	{
		int newmf;
		sscanf(argv[i]+3, "%d", &newmf);
		if (newmf<=0 || newmf>255) {
			fatalError(ERR_ST, "memory factor out of range <1,255>", XREF_EXIT_ERR);
		}
		s_opt.cxMemoryFaktor = newmf;
	}	
	else if (strncmp(argv[i],"-maxCompls=",11)==0 || strncmp(argv[i],"-maxcompls=",11)==0) 	{
		sscanf(argv[i]+11, "%d", &s_opt.maxCompletions);
	}
	else if (strncmp(argv[i],"-movetargetclass=",17)==0) {
		crOptionStr(&s_opt.moveTargetClass, argv[i]+17);
	}
	else if (strncmp(argv[i],"-movetargetfile=",16)==0) {
		crOptionStr(&s_opt.moveTargetFile, argv[i]+16);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processNOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-noincluderefs")==0)		s_opt.noIncludeRefs = 1;
	else if (strcmp(argv[i],"-noincluderefresh")==0)	s_opt.noIncludeRefs=1;
	else if (strcmp(argv[i],"-nocxfile")==0)	    	s_opt.noCxFile = 1;
	else if (strcmp(argv[i],"-no_cpp_comment")==0)		s_opt.cpp_comment = 0;
	else if (strcmp(argv[i],"-nobrief")==0)	 			s_opt.brief = 0;
	else if (strcmp(argv[i],"-no_enum")==0)				s_opt.no_ref_enumerator = 1;
	else if (strcmp(argv[i],"-no_mac")==0)				s_opt.no_ref_macro = 1;
	else if (strcmp(argv[i],"-no_type")==0)				s_opt.no_ref_typedef = 1;
	else if (strcmp(argv[i],"-no_str")==0)				s_opt.no_ref_records = 1;
	else if (strcmp(argv[i],"-no_local")==0)			s_opt.no_ref_locals = 1;
	else if (strcmp(argv[i],"-no_cfrefs")==0) 			s_opt.allowClassFileRefs = 0;
	else if (strcmp(argv[i],"-no_stdop")==0 
			 || strcmp(argv[i],"-nostdop")==0)			s_opt.no_stdop = 1;
	else if (strcmp(argv[i],"-noautoupdatefromsrc")==0) s_opt.javaSlAllowed = 0;
	else if (strcmp(argv[i],"-noerrors")==0)			s_opt.noErrors=1;
	else return(0);
	*ii = i;
	return(1);
}

static int processOOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strncmp(argv[i],"-oocheckbits=",13)==0)	{
		sscanf(argv[i]+13,"%o",&s_opt.ooChecksBits);
	}
	else if (strncmp(argv[i],"-olinelen=",10)==0) {
		sscanf(argv[i]+10,"%d",&s_opt.olineLen);
		if (s_opt.olineLen == 0) s_opt.olineLen =79;
		else s_opt.olineLen--;
	}
	else if (strncmp(argv[i],"-olcursor=",10)==0) {
		sscanf(argv[i]+10,"%d",&s_opt.olCursorPos);
	}
	else if (strncmp(argv[i],"-olmark=",8)==0) {
		sscanf(argv[i]+8,"%d",&s_opt.olMarkPos);
	}
	else if (strncmp(argv[i],"-olcheckversion=",16)==0) {
		crOptionStr(&s_opt.checkVersion, argv[i]+16);
		s_opt.cxrefs = OLO_CHECK_VERSION;
	}
	else if (strncmp(argv[i],"-olcxrefsuffix=",15)==0) 	{
		crOptionStr(&s_opt.olcxRefSuffix, argv[i]+15);
	}
	else if (strncmp(argv[i],"-olcxresetrefsuffix=",20)==0) 	{
		s_opt.cxrefs = OLO_RESET_REF_SUFFIX;
		crOptionStr(&s_opt.olcxRefSuffix, argv[i]+20);
	}
	else if (strncmp(argv[i],"-olcxextract=",13)==0) {
		// don't use this, obsolete
		s_opt.cxrefs = OLO_EXTRACT;
		//& crOptionStr(&s_opt.extractName, argv[i]+13);
	}
	else if (strcmp(argv[i],"-olcxextract")==0) {
		s_opt.cxrefs = OLO_EXTRACT;
	}
	else if (strcmp(argv[i],"-olcxtrivialprecheck")==0) {
		s_opt.cxrefs = OLO_TRIVIAL_PRECHECK;
	}
	else if (strcmp(argv[i],"-olmanualresolve")==0) {
		s_opt.manualResolve = RESOLVE_DIALOG_ALLWAYS;
	}
	else if (strcmp(argv[i],"-olnodialog")==0) {
		s_opt.manualResolve = RESOLVE_DIALOG_NEVER;
	}
	else if (strcmp(argv[i],"-olexaddress")==0) {
		s_opt.extractMode = EXTR_FUNCTION_ADDRESS_ARGS;
	}
	else if (strcmp(argv[i],"-olchecklinkage")==0) {
		s_opt.ooChecksBits |= OOC_LINKAGE_CHECK;
	}
	else if (strcmp(argv[i],"-olcheckaccess")==0) {
		s_opt.ooChecksBits |= OOC_ACCESS_CHECK;
	}
	else if (strcmp(argv[i],"-olnocheckaccess")==0) {
		s_opt.ooChecksBits &= ~OOC_ACCESS_CHECK;
	}
	else if (strcmp(argv[i],"-olallchecks")==0) {
		s_opt.ooChecksBits |= OOC_ALL_CHECKS;
	}
	else if (strncmp(argv[i],"-olfqtcompletionslevel=",23)==0) {
		s_opt.fqtNameToCompletions = 1;
		sscanf(argv[i]+23, "%d",&s_opt.fqtNameToCompletions);
	}
	else if (strcmp(argv[i],"-olexmacro")==0) s_opt.extractMode=EXTR_MACRO;
	else if (strcmp(argv[i],"-olcxunmodified")==0) 	{
		s_opt.modifiedFlag = 0;
	}
	else if (strcmp(argv[i],"-olcxmodified")==0) 	{
		s_opt.modifiedFlag = 1;
	}
	else if (strcmp(argv[i],"-olcxrename")==0) 	s_opt.cxrefs = OLO_RENAME;
	else if (strcmp(argv[i],"-olcxencapsulate")==0) s_opt.cxrefs = OLO_ENCAPSULATE;
	else if (strcmp(argv[i],"-olcxargmanip")==0) 	s_opt.cxrefs = OLO_ARG_MANIP;
	else if (strcmp(argv[i],"-olcxdynamictostatic1")==0) 	s_opt.cxrefs = OLO_VIRTUAL2STATIC_PUSH;
	else if (strcmp(argv[i],"-olcxsafetycheckinit")==0) s_opt.cxrefs = OLO_SAFETY_CHECK_INIT;
	else if (strcmp(argv[i],"-olcxsafetycheck1")==0) s_opt.cxrefs = OLO_SAFETY_CHECK1;
	else if (strcmp(argv[i],"-olcxsafetycheck2")==0) s_opt.cxrefs = OLO_SAFETY_CHECK2;
	else if (strcmp(argv[i],"-olcxintersection")==0) s_opt.cxrefs = OLO_INTERSECTION;
	else if (strcmp(argv[i],"-olcxsafetycheckmovedfile")==0
			 || strcmp(argv[i],"-olcxsafetycheckmoved")==0  /* backward compatibility */
		) {
		NEXT_ARG();
		crOptionStr(&s_opt.checkFileMovedFrom, argv[i]);
		NEXT_ARG();
		crOptionStr(&s_opt.checkFileMovedTo, argv[i]);
	}
	else if (strcmp(argv[i],"-olcxwindel")==0) {
		s_opt.cxrefs = OLO_REMOVE_WIN;
	}
	else if (strcmp(argv[i],"-olcxwindelfile")==0) {
		NEXT_ARG();
		crOptionStr(&s_opt.olcxWinDelFile, argv[i]);
	}
	else if (strncmp(argv[i],"-olcxwindelwin=",15)==0) {
		s_opt.olcxWinDelFromLine = s_opt.olcxWinDelToLine = 0;
		s_opt.olcxWinDelFromCol = s_opt.olcxWinDelToCol = 0;
		sscanf(argv[i]+15,"%d:%dx%d:%d", 
			   &s_opt.olcxWinDelFromLine, &s_opt.olcxWinDelFromCol,
			   &s_opt.olcxWinDelToLine, &s_opt.olcxWinDelToCol);
//&fprintf(dumpOut,"; delete refs %d:%d-%d:%d\n", s_opt.olcxWinDelFromLine, s_opt.olcxWinDelFromCol, s_opt.olcxWinDelToLine, s_opt.olcxWinDelToCol);
	}
	else if (strncmp(argv[i],"-olcxsafetycheckmovedblock=",27)==0) {
		sscanf(argv[i]+27, "%d:%d:%d", &s_opt.checkFirstMovedLine, 
			   &s_opt.checkLinesMoved, &s_opt.checkNewLineNumber);
//&fprintf(dumpOut,"safety check block moved == %d:%d:%d\n", s_opt.checkFirstMovedLine,s_opt.checkLinesMoved, s_opt.checkNewLineNumber);
	}
	else if (strcmp(argv[i],"-olcxgotodef")==0) s_opt.cxrefs = OLO_GOTO_DEF;
	else if (strcmp(argv[i],"-olcxgotocaller")==0) s_opt.cxrefs = OLO_GOTO_CALLER;
	else if (strcmp(argv[i],"-olcxgotocurrent")==0) s_opt.cxrefs = OLO_GOTO_CURRENT;
	else if (strcmp(argv[i],"-olcxgetcurrentrefn")==0)s_opt.cxrefs=OLO_GET_CURRENT_REFNUM;
	else if (strncmp(argv[i],"-olcxgotoparname",16)==0) {
		s_opt.cxrefs = OLO_GOTO_PARAM_NAME;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+16, "%d", &s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxgetparamcoord",18)==0) {
		s_opt.cxrefs = OLO_GET_PARAM_COORDINATES;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+18, "%d", &s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxparnum=",12)==0) {
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+12, "%d", &s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxparnum2=",13)==0) {
		s_opt.parnum2 = 0;
		sscanf(argv[i]+13, "%d", &s_opt.parnum2);
	}
	else if (strcmp(argv[i],"-olcxtops")==0) s_opt.cxrefs = OLO_SHOW_TOP;
	else if (strcmp(argv[i],"-olcxtoptype")==0) s_opt.cxrefs = OLO_SHOW_TOP_TYPE;
	else if (strcmp(argv[i],"-olcxtopapplcl")==0) s_opt.cxrefs = OLO_SHOW_TOP_APPL_CLASS;
	else if (strcmp(argv[i],"-olcxshowctree")==0) s_opt.cxrefs = OLO_SHOW_CLASS_TREE;
	else if (strcmp(argv[i],"-olcxedittop")==0) s_opt.cxrefs = OLO_TOP_SYMBOL_RES;
	else if (strcmp(argv[i],"-olcxclasstree")==0) s_opt.cxrefs = OLO_CLASS_TREE;
	else if (strcmp(argv[i],"-olcxsyntaxpass")==0) s_opt.cxrefs = OLO_SYNTAX_PASS_ONLY;
	else if (strcmp(argv[i],"-olcxprimarystart")==0) {
		s_opt.cxrefs = OLO_GET_PRIMARY_START;
		//&s_opt.parsingDeep = PD_ZERO;
	}
	else if (strcmp(argv[i],"-olcxuselesslongnames")==0) s_opt.cxrefs = OLO_USELESS_LONG_NAME;
	else if (strcmp(argv[i],"-olcxuselesslongnamesinclass")==0) s_opt.cxrefs = OLO_USELESS_LONG_NAME_IN_CLASS;
	else if (strcmp(argv[i],"-olcxmaybethis")==0) s_opt.cxrefs = OLO_MAYBE_THIS;
	else if (strcmp(argv[i],"-olcxnotfqt")==0) s_opt.cxrefs = OLO_NOT_FQT_REFS;
	else if (strcmp(argv[i],"-olcxnotfqtinclass")==0) s_opt.cxrefs = OLO_NOT_FQT_REFS_IN_CLASS;
	else if (strcmp(argv[i],"-olcxgetrefactorings")==0) 	{
		s_opt.cxrefs = OLO_GET_AVAILABLE_REFACTORINGS;
	}
	else if (strcmp(argv[i],"-olcxpush")==0) 	s_opt.cxrefs = OLO_PUSH;
	else if (strcmp(argv[i],"-olcxrepush")==0) 	s_opt.cxrefs = OLO_REPUSH;
	else if (strcmp(argv[i],"-olcxpushonly")==0) s_opt.cxrefs = OLO_PUSH_ONLY;
	else if (strcmp(argv[i],"-olcxpushandcallmacro")==0) s_opt.cxrefs = OLO_PUSH_AND_CALL_MACRO;
	else if (strcmp(argv[i],"-olcxencapsulatesc1")==0) s_opt.cxrefs = OLO_PUSH_ENCAPSULATE_SAFETY_CHECK;
	else if (strcmp(argv[i],"-olcxencapsulatesc2")==0) s_opt.cxrefs = OLO_ENCAPSULATE_SAFETY_CHECK;
	else if (strcmp(argv[i],"-olcxpushallinmethod")==0) s_opt.cxrefs = OLO_PUSH_ALL_IN_METHOD;
	else if (strcmp(argv[i],"-olcxmmprecheck")==0) s_opt.cxrefs = OLO_MM_PRE_CHECK;
	else if (strcmp(argv[i],"-olcxppprecheck")==0) s_opt.cxrefs = OLO_PP_PRE_CHECK;
	else if (strcmp(argv[i],"-olcxpushforlm")==0) {
		s_opt.cxrefs = OLO_PUSH_FOR_LOCALM;
		s_opt.manualResolve = RESOLVE_DIALOG_NEVER;		 
	}
	else if (strcmp(argv[i],"-olcxpushglobalunused")==0) 	s_opt.cxrefs = OLO_GLOBAL_UNUSED;
	else if (strcmp(argv[i],"-olcxpushfileunused")==0) 	s_opt.cxrefs = OLO_LOCAL_UNUSED;
	else if (strcmp(argv[i],"-olcxlist")==0) 	s_opt.cxrefs = OLO_LIST;
	else if (strcmp(argv[i],"-olcxlisttop")==0) s_opt.cxrefs=OLO_LIST_TOP;
	else if (strcmp(argv[i],"-olcxpop")==0) 	s_opt.cxrefs = OLO_POP;
	else if (strcmp(argv[i],"-olcxpoponly")==0) s_opt.cxrefs =OLO_POP_ONLY;
	else if (strcmp(argv[i],"-olcxplus")==0) 	s_opt.cxrefs = OLO_PLUS;
	else if (strcmp(argv[i],"-olcxminus")==0) 	s_opt.cxrefs = OLO_MINUS;
	else if (strcmp(argv[i],"-olcxsearch")==0) 	s_opt.cxrefs = OLO_SEARCH;
	else if (strcmp(argv[i],"-olcxcomplet")==0)s_opt.cxrefs=OLO_COMPLETION;
	else if (strcmp(argv[i],"-olcxtarget")==0)	s_opt.cxrefs=OLO_SET_MOVE_TARGET;
	else if (strcmp(argv[i],"-olcxmctarget")==0)	s_opt.cxrefs=OLO_SET_MOVE_CLASS_TARGET;
	else if (strcmp(argv[i],"-olcxmmtarget")==0)	s_opt.cxrefs=OLO_SET_MOVE_METHOD_TARGET;
	else if (strcmp(argv[i],"-olcxcurrentclass")==0)	s_opt.cxrefs=OLO_GET_CURRENT_CLASS;
	else if (strcmp(argv[i],"-olcxcurrentsuperclass")==0)	s_opt.cxrefs=OLO_GET_CURRENT_SUPER_CLASS;
	else if (strcmp(argv[i],"-olcxmethodlines")==0)	s_opt.cxrefs=OLO_GET_METHOD_COORD;
	else if (strcmp(argv[i],"-olcxclasslines")==0)	s_opt.cxrefs=OLO_GET_CLASS_COORD;
	else if (strcmp(argv[i],"-olcxgetsymboltype")==0) s_opt.cxrefs=OLO_GET_SYMBOL_TYPE;
	else if (strcmp(argv[i],"-olcxgetprojectname")==0) {
		s_opt.cxrefs=OLO_ACTIVE_PROJECT;
	}
	else if (strcmp(argv[i],"-olcxgetjavahome")==0) {
		s_opt.cxrefs=OLO_JAVA_HOME;
	}
	else if (strncmp(argv[i],"-olcxlccursor=",14)==0) {
		// position of the cursor in line:column format
		crOptionStr(&s_opt.olcxlccursor, argv[i]+14);
	}
	else if (strncmp(argv[i],"-olcxcplsearch=",15)==0) {
		s_opt.cxrefs=OLO_SEARCH;
		crOptionStr(&s_opt.olcxSearchString, argv[i]+15);
	}
	else if (strncmp(argv[i],"-olcxtagsearch=",15)==0) {
		s_opt.cxrefs=OLO_TAG_SEARCH;
		crOptionStr(&s_opt.olcxSearchString, argv[i]+15);
	}
	else if (strcmp(argv[i],"-olcxtagsearchforward")==0) {
		s_opt.cxrefs=OLO_TAG_SEARCH_FORWARD;
	}
	else if (strcmp(argv[i],"-olcxtagsearchback")==0) {
		s_opt.cxrefs=OLO_TAG_SEARCH_BACK;
	}
	else if (strncmp(argv[i],"-olcxpushname=",14)==0) 	{
		s_opt.cxrefs = OLO_PUSH_NAME;
		crOptionStr(&s_opt.pushName, argv[i]+14);
	}
	else if (strncmp(argv[i],"-olcxpushspecialname=",21)==0) 	{
		s_opt.cxrefs = OLO_PUSH_SPECIAL_NAME;
		crOptionStr(&s_opt.pushName, argv[i]+21);
	}
	else if (strncmp(argv[i],"-olcomplselect",14)==0) {
		s_opt.cxrefs=OLO_CSELECT;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+14,"%d",&s_opt.olcxGotoVal);
	}
	else if (strcmp(argv[i],"-olcomplback")==0) {
		s_opt.cxrefs=OLO_COMPLETION_BACK;
	}
	else if (strcmp(argv[i],"-olcomplforward")==0) {
		s_opt.cxrefs=OLO_COMPLETION_FORWARD;
	}
	else if (strncmp(argv[i],"-olcxcgoto",10)==0) {
		s_opt.cxrefs = OLO_CGOTO;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+10,"%d",&s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxtaggoto",12)==0) {
		s_opt.cxrefs = OLO_TAGGOTO;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+12,"%d",&s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxtagselect",14)==0) {
		s_opt.cxrefs = OLO_TAGSELECT;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+14,"%d",&s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxcbrowse",12)==0) {
		s_opt.cxrefs = OLO_CBROWSE;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+12,"%d",&s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxgoto",9)==0) {
		s_opt.cxrefs = OLO_GOTO;
		s_opt.olcxGotoVal = 0;
		sscanf(argv[i]+9,"%d",&s_opt.olcxGotoVal);
	}
	else if (strncmp(argv[i],"-olcxfilter=",12)==0) {
		s_opt.cxrefs = OLO_REF_FILTER_SET;
		sscanf(argv[i]+12,"%d",&s_opt.filterValue);
	}
	else if (strncmp(argv[i], "-olcxmenusingleselect",21)==0) {
		s_opt.cxrefs = OLO_MENU_SELECT_ONLY;
		s_opt.olcxMenuSelectLineNum = 0;
		sscanf(argv[i]+21,"%d",&s_opt.olcxMenuSelectLineNum);
	}
	else if (strncmp(argv[i],"-olcxmenuselect",15)==0) {
		s_opt.cxrefs = OLO_MENU_SELECT;
		s_opt.olcxMenuSelectLineNum = 0;
		sscanf(argv[i]+15,"%d",&s_opt.olcxMenuSelectLineNum);
	}
	else if (strncmp(argv[i],"-olcxmenuinspectdef",19)==0) {
		s_opt.cxrefs = OLO_MENU_INSPECT_DEF;
		s_opt.olcxMenuSelectLineNum = 0;
		sscanf(argv[i]+19,"%d",&s_opt.olcxMenuSelectLineNum);
	}
	else if (strncmp(argv[i],"-olcxmenuinspectclass",21)==0) {
		s_opt.cxrefs = OLO_MENU_INSPECT_CLASS;
		s_opt.olcxMenuSelectLineNum = 0;
		sscanf(argv[i]+21,"%d",&s_opt.olcxMenuSelectLineNum);
	}
	else if (strncmp(argv[i],"-olcxctinspectdef",17)==0) {
		s_opt.cxrefs = OLO_CT_INSPECT_DEF;
		s_opt.olcxMenuSelectLineNum = 0;
		sscanf(argv[i]+17,"%d",&s_opt.olcxMenuSelectLineNum);
	}
	else if (strcmp(argv[i],"-olcxmenuall")==0) {
		s_opt.cxrefs = OLO_MENU_SELECT_ALL;
	}
	else if (strcmp(argv[i],"-olcxmenunone")==0) {
		s_opt.cxrefs = OLO_MENU_SELECT_NONE;
	}
	else if (strcmp(argv[i],"-olcxmenugo")==0) {
		s_opt.cxrefs = OLO_MENU_GO;
	}
	else if (strncmp(argv[i],"-olcxmenufilter=",16)==0) {
		s_opt.cxrefs = OLO_MENU_FILTER_SET;
		sscanf(argv[i]+16,"%d",&s_opt.filterValue);
	}
	else if (strcmp(argv[i],"-optinclude")==0) {
		i = mainHandleIncludeOption(argc, argv, i);
	}
	else if (strcmp(argv[i],"-o")==0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.outputFileName, argv[i]);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processPOption(int *ii, int argc, char **argv) {
	char	ttt[MAX_FILE_NAME_SIZE];
	int 	i = * ii;
	if (0) {}
	else if (strncmp(argv[i],"-pass",5)==0) {
		error(ERR_ST,"'-pass' option can't be entered from command line");
	}
	else if (strcmp(argv[i],"-packages")==0) {
		s_opt.allowPackagesOnCl = 1;
	}
	else if (strcmp(argv[i],"-p")==0) {
		NEXT_FILE_ARG();
		//fprintf(dumpOut,"current project '%s'\n", argv[i]);
		crOptionStr(&s_opt.project, argv[i]);
	}
	else if (strcmp(argv[i],"-preload")==0) {
		char *buff, *file, *fromFile;
		NEXT_FILE_ARG();
		file = argv[i];
		strcpy(ttt, normalizeFileName(file, s_cwd));
		NEXT_FILE_ARG();
		fromFile = argv[i];
		// TODO, maybe do this also through allocated list of options
		// and serve them later ?
//&sprintf(tmpBuff,"-preload %s %s\n", ttt, fromFile); ppcGenRecord(PPC_IGNORE, tmpBuff, "\n");
		editorOpenBufferNoFileLoad(ttt, fromFile);
	}
	else if (strcmp(argv[i],"-prune")==0) {
		NEXT_ARG();
		mainAddStringListOption(&s_opt.pruneNames, argv[i]);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processQOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else return(0);
	*ii = i;
	return(1);
}

static void setXrefsFile(char *argvi) {
	static int message=0;
	if (s_opt.taskRegime==RegimeXref && message==0 && ! isAbsolutePath(argvi)) {
		message = 1;
		sprintf(tmpBuff,"'%s' is not an absolute path, correct -refs option",argvi);
		warning(ERR_ST, tmpBuff);
	}
	crOptionStr(&s_opt.cxrefFileName, normalizeFileName(argvi, s_cwd));
}

static int processROption(int *ii, int argc, char **argv, int infilesFlag) {
	int i = * ii;
	if (0) {}
	else if (strncmp(argv[i],"-refnum=",8)==0)	{
		sscanf(argv[i]+8, "%d", &s_opt.refnum);
	}
	else if (strcmp(argv[i],"-refalphahash")==0
			 || strcmp(argv[i],"-refalpha1hash")==0)	{
		int check;
		s_opt.xfileHashingMethod = XFILE_HASH_ALPHA1;
		check = changeRefNumOption(XFILE_HASH_ALPHA1_REFNUM);
		if (check == 0)	{
			assert(s_opt.taskRegime);
			fatalError(ERR_ST,"'-refalphahash' conflicts with '-refnum' option", XREF_EXIT_ERR);
		}
	}
	else if (strcmp(argv[i],"-refalpha2hash")==0)	{
		int check;
		s_opt.xfileHashingMethod = XFILE_HASH_ALPHA2;
		check = changeRefNumOption(XFILE_HASH_ALPHA2_REFNUM);
		if (check == 0)	{
			assert(s_opt.taskRegime);
			fatalError(ERR_ST,"'-refalpha2hash' conflicts with '-refnum' option", XREF_EXIT_ERR);
		}
	}
	else if (strcmp(argv[i],"-r")==0) {
		if (infilesFlag == INFILES_ENABLED) s_opt.recursivelyDirs = 1;
	}
	else if (strncmp(argv[i],"-renameto=", 10)==0) {
		crOptionStr(&s_opt.renameTo, argv[i]+10);
	}
	else if (strcmp(argv[i],"-resetIncludeDirs")==0) {
		s_opt.includeDirs = NULL;
	}
	else if (strcmp(argv[i],"-refs")==0)	{
		NEXT_FILE_ARG();
		setXrefsFile(argv[i]);
	}
	else if (strncmp(argv[i],"-refs=",6)==0)	{
		setXrefsFile(argv[i]+6);
	}
	else if (strcmp(argv[i],"-rlistwithoutsrc")==0)	{
		s_opt.referenceListWithoutSource = 1;
	}
	else if (strcmp(argv[i],"-refactory")==0)	{
		s_opt.refactoringRegime = RegimeRefactory;
	}
	else if (strcmp(argv[i],"-rfct-rename")==0)	{
		s_opt.theRefactoring = PPC_AVR_RENAME_SYMBOL;
	}
	else if (strcmp(argv[i],"-rfct-rename-class")==0)	{
		s_opt.theRefactoring = PPC_AVR_RENAME_CLASS;
	}
	else if (strcmp(argv[i],"-rfct-rename-package")==0)	{
		s_opt.theRefactoring = PPC_AVR_RENAME_PACKAGE;
	}
	else if (strcmp(argv[i],"-rfct-expand")==0)	{
		s_opt.theRefactoring = PPC_AVR_EXPAND_NAMES;
	}
	else if (strcmp(argv[i],"-rfct-reduce")==0)	{
		s_opt.theRefactoring = PPC_AVR_REDUCE_NAMES;
	}
	else if (strcmp(argv[i],"-rfct-add-param")==0)	{
		s_opt.theRefactoring = PPC_AVR_ADD_PARAMETER;
	}
	else if (strcmp(argv[i],"-rfct-del-param")==0)	{
		s_opt.theRefactoring = PPC_AVR_DEL_PARAMETER;
	}
	else if (strcmp(argv[i],"-rfct-move-param")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_PARAMETER;
	}
	else if (strcmp(argv[i],"-rfct-move-field")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_FIELD;
	}
	else if (strcmp(argv[i],"-rfct-move-static-field")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_STATIC_FIELD;
	}
	else if (strcmp(argv[i],"-rfct-move-static-method")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_STATIC_METHOD;
	}
	else if (strcmp(argv[i],"-rfct-move-class")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_CLASS;
	}
	else if (strcmp(argv[i],"-rfct-move-class-to-new-file")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_CLASS_TO_NEW_FILE;
	}
	else if (strcmp(argv[i],"-rfct-move-all-classes-to-new-file")==0)	{
		s_opt.theRefactoring = PPC_AVR_MOVE_ALL_CLASSES_TO_NEW_FILE;
	}
	else if (strcmp(argv[i],"-rfct-static-to-dynamic")==0)	{
		s_opt.theRefactoring = PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC;
	}
	else if (strcmp(argv[i],"-rfct-dynamic-to-static")==0)	{
		s_opt.theRefactoring = PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC;
	}
	else if (strcmp(argv[i],"-rfct-extract-method")==0)	{
		s_opt.theRefactoring = PPC_AVR_EXTRACT_METHOD;
	}
	else if (strcmp(argv[i],"-rfct-extract-macro")==0)	{
		s_opt.theRefactoring = PPC_AVR_EXTRACT_MACRO;
	}
	else if (strcmp(argv[i],"-rfct-reduce-long-names-in-the-file")==0)	{
		s_opt.theRefactoring = PPC_AVR_ADD_ALL_POSSIBLE_IMPORTS;
	}
	else if (strcmp(argv[i],"-rfct-add-to-imports")==0)	{
		s_opt.theRefactoring = PPC_AVR_ADD_TO_IMPORT;
	}
	else if (strcmp(argv[i],"-rfct-self-encapsulate-field")==0)	{
		s_opt.theRefactoring = PPC_AVR_SELF_ENCAPSULATE_FIELD;
	}
	else if (strcmp(argv[i],"-rfct-encapsulate-field")==0)	{
		s_opt.theRefactoring = PPC_AVR_ENCAPSULATE_FIELD;
	}
	else if (strcmp(argv[i],"-rfct-push-down-field")==0)	{
		s_opt.theRefactoring = PPC_AVR_PUSH_DOWN_FIELD;
	}
	else if (strcmp(argv[i],"-rfct-push-down-method")==0)	{
		s_opt.theRefactoring = PPC_AVR_PUSH_DOWN_METHOD;
	}
	else if (strcmp(argv[i],"-rfct-pull-up-field")==0)	{
		s_opt.theRefactoring = PPC_AVR_PULL_UP_FIELD;
	}
	else if (strcmp(argv[i],"-rfct-pull-up-method")==0)	{
		s_opt.theRefactoring = PPC_AVR_PULL_UP_METHOD;
	}
#if 0
	else if (strcmp(argv[i],"-rfct-")==0)	{
		s_opt.theRefactoring = PPC_AVR_;
	}
#endif
	else if (strncmp(argv[i], "-rfct-param1=", 13)==0)	{
		crOptionStr(&s_opt.refpar1, argv[i]+13);
	}
	else if (strncmp(argv[i], "-rfct-param2=", 13)==0)	{
		crOptionStr(&s_opt.refpar2, argv[i]+13);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processSOption(int *ii, int argc, char **argv) {
	int i = * ii;
	char *name, *val;
	if (0) {}
	else if (strcmp(argv[i],"-strict")==0)		s_opt.strictAnsi = 1;
	else if (strcmp(argv[i],"-str_fill")==0) 	s_opt.str_fill = 1;
	else if (strcmp(argv[i],"-str_copy")==0) 	s_opt.str_copy = 1;
	else if (strcmp(argv[i],"-stderr")==0) 			errOut = stdout;
	else if (strcmp(argv[i],"-source")==0)	{
		NEXT_ARG();
		if (strcmp(argv[i], JAVA_VERSION_1_3)!=0 && strcmp(argv[i], JAVA_VERSION_1_4)!=0) {
			sprintf(tmpBuff,"wrong -javaversion=<value>, available values are %s, %s",
					JAVA_VERSION_1_3, JAVA_VERSION_1_4);
			error(ERR_ST, tmpBuff);
		} else {
			crOptionStr(&s_opt.javaVersion, argv[i]);
		}
	}
	else if (strcmp(argv[i],"-sourcepath")==0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.sourcePath, argv[i]);
		xrefSetenv("-sourcepath", s_opt.sourcePath);
	}
	else if (strcmp(argv[i],"-stdop")==0) {
		i = mainHandleIncludeOption(argc, argv, i);
	}
	else if (strcmp(argv[i],"-set")==0) {
		i = mainHandleSetOption(argc, argv, i);
	}
	else if (strncmp(argv[i],"-set",4)==0) {
		name = argv[i]+4;
		NEXT_ARG();
		val = argv[i];
		xrefSetenv(name, val);
	}
	else if (strcmp(argv[i],"-searchdef")==0) {
		s_opt.tagSearchSpecif = TSS_SEARCH_DEFS_ONLY;
	}
	else if (strcmp(argv[i],"-searchshortlist")==0) {
		s_opt.tagSearchSpecif = TSS_FULL_SEARCH_SHORT;
	}
	else if (strcmp(argv[i],"-searchdefshortlist")==0) {
		s_opt.tagSearchSpecif = TSS_SEARCH_DEFS_ONLY_SHORT;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processTOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-typedefs")==0) 	s_opt.typedefg = 1;
	else if (strcmp(argv[i],"-task_regime_server")==0) {
		s_opt.taskRegime = RegimeEditServer;
	}
	else if (strcmp(argv[i],"-task_regime_generate")==0) {
		s_opt.taskRegime = RegimeGenerate;
	}
	else if (strcmp(argv[i],"-thread")==0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.user, argv[i]);
	}
	else if (strcmp(argv[i],"-tpchrenamepackage")==0) {
		s_opt.trivialPreCheckCode = TPC_RENAME_PACKAGE;
	}
	else if (strcmp(argv[i],"-tpchrenameclass")==0) {
		s_opt.trivialPreCheckCode = TPC_RENAME_CLASS;
	}
	else if (strcmp(argv[i],"-tpchmoveclass")==0) {
		s_opt.trivialPreCheckCode = TPC_MOVE_CLASS;
	}
	else if (strcmp(argv[i],"-tpchmovefield")==0) {
		s_opt.trivialPreCheckCode = TPC_MOVE_FIELD;
	}
	else if (strcmp(argv[i],"-tpchmovestaticfield")==0) {
		s_opt.trivialPreCheckCode = TPC_MOVE_STATIC_FIELD;
	}
	else if (strcmp(argv[i],"-tpchmovestaticmethod")==0) {
		s_opt.trivialPreCheckCode = TPC_MOVE_STATIC_METHOD;
	}
	else if (strcmp(argv[i],"-tpchturndyntostatic")==0) {
		s_opt.trivialPreCheckCode = TPC_TURN_DYN_METHOD_TO_STATIC;
	}
	else if (strcmp(argv[i],"-tpchturnstatictodyn")==0) {
		s_opt.trivialPreCheckCode = TPC_TURN_STATIC_METHOD_TO_DYN;
	}
	else if (strcmp(argv[i],"-tpchpullupmethod")==0) {
		s_opt.trivialPreCheckCode = TPC_PULL_UP_METHOD;
	}
	else if (strcmp(argv[i],"-tpchpushdownmethod")==0) {
		s_opt.trivialPreCheckCode = TPC_PUSH_DOWN_METHOD;
	}
	else if (strcmp(argv[i],"-tpchpushdownmethodpostcheck")==0) {
		s_opt.trivialPreCheckCode = TPC_PUSH_DOWN_METHOD_POST_CHECK;
	}
	else if (strcmp(argv[i],"-tpchpullupfield")==0) {
		s_opt.trivialPreCheckCode = TPC_PULL_UP_FIELD;
	}
	else if (strcmp(argv[i],"-tpchpushdownfield")==0) {
		s_opt.trivialPreCheckCode = TPC_PUSH_DOWN_FIELD;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processUOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-urlmanualredirect")==0) 	{
		s_opt.urlAutoRedirect = 0;
	}
	else if (strcmp(argv[i],"-urldirect")==0) 	{
		s_opt.urlGenTemporaryFile = 0;
	}
	else if (strcmp(argv[i],"-user")==0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.user, argv[i]);
	}
	else if (strcmp(argv[i],"-update")==0) 	{
		s_opt.update = UP_FULL_UPDATE;
		s_opt.updateOnlyModifiedFiles = 1;
	}
	else if (strcmp(argv[i],"-updatem")==0) {
		s_opt.update = UP_FULL_UPDATE;
		s_opt.updateOnlyModifiedFiles = 1;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processVOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-version")==0||strcmp(argv[i],"-about")==0){
		s_opt.cxrefs = OLO_ABOUT;
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processWOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else return(0);
	*ii = i;
	return(1);
}

static int processXOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else if (strcmp(argv[i],"-xrefactory-II") == 0){
		s_opt.xref2 = 1;
	}
	else if (strncmp(argv[i],"-xrefrc=",8) == 0) {
		crOptionStr(&s_opt.xrefrc, argv[i]+8);
	}
	else if (strcmp(argv[i],"-xrefrc") == 0) {
		NEXT_FILE_ARG();
		crOptionStr(&s_opt.xrefrc, argv[i]);
	}
	else return(0);
	*ii = i;
	return(1);
}

static int processYOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else return(0);
	*ii = i;
	return(1);
}

static int processZOption(int *ii, int argc, char **argv) {
	int i = * ii;
	if (0) {}
	else return(0);
	*ii = i;
	return(1);
}

static void mainScheduleInputFileOptionToFileTable(char *infile) {
	int topCallFlag;
	void *recursFlag;
	javaSetSourcePath(1);      // for case of packages on command line
	topCallFlag = 1;
	recursFlag = &topCallFlag;
	JavaMapOnPaths(infile, {
		dirInputFile(currentPath,"",NULL,NULL,recursFlag,&topCallFlag);
	});
}

static void mainProcessInFileOption(char *infile) {
	int i;
	if (infile[0]=='`' && infile[strlen(infile)-1]=='`') {
		int nargc,aaa; 
		char **nargv, *pp;
		char command[MAX_OPTION_LEN];
		s_opt.stdopFlag = 1;
		strcpy(command, infile+1);
		pp = strchr(command, '`');
		if (pp!=NULL) *pp = 0;
		readOptionPipe(command, &nargc, &nargv, "");
		for(i=1; i<nargc; i++) {
			if (nargv[i][0]!='-' && nargv[i][0]!='`') {
				mainScheduleInputFileOptionToFileTable(nargv[i]);
			}
		}
		s_opt.stdopFlag = 0;
	} else {
		mainScheduleInputFileOptionToFileTable(infile);
	}
}

void processOptions(int argc, char **argv, int infilesFlag) {
	S_stringList 	*ll;
	S_fileItem		*fi,ffi;
	char			*fin,*ftin;
	int 			i,ii,newRefNum,ln,topCallFlag,processed,tmp;
	void			*recursFlag;
	for (i=1; i<argc; i++) {
		if (s_opt.taskRegime==RegimeEditServer && 
			strncmp(argv[i],"-last_message=",14)==0) {
			// because of emacs-debug
			DPRINTF1("option -lastmessage=...\n");
		} else {
			DPRINTF2("option %s\n",argv[i]);
		}
		processed = 0;
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case '-': processed = processNegativeOption(&i, argc, argv, infilesFlag);
				break;
			case 'a': case 'A': processed = processAOption(&i, argc, argv);
				break;
			case 'b': case 'B': processed = processBOption(&i, argc, argv);
				break;
			case 'c': case 'C': processed = processCOption(&i, argc, argv);
				break;
			case 'd': case 'D': processed = processDOption(&i, argc, argv);
				break;
			case 'e': case 'E': processed = processEOption(&i, argc, argv);
				break;
			case 'f': case 'F': processed = processFOption(&i, argc, argv);
				break;
			case 'g': case 'G': processed = processGOption(&i, argc, argv);
				break;
			case 'h': case 'H': processed = processHOption(&i, argc, argv);
				break;
			case 'i': case 'I': processed = processIOption(&i, argc, argv);
				break;
			case 'j': case 'J': processed = processJOption(&i, argc, argv);
				break;
			case 'k': case 'K': processed = processKOption(&i, argc, argv);
				break;
			case 'l': case 'L': processed = processLOption(&i, argc, argv);
				break;
			case 'm': case 'M': processed = processMOption(&i, argc, argv);
				break;
			case 'n': case 'N': processed = processNOption(&i, argc, argv);
				break;
			case 'o': case 'O': processed = processOOption(&i, argc, argv);
				break;
			case 'p': case 'P': processed = processPOption(&i, argc, argv);
				break;
			case 'q': case 'Q': processed = processQOption(&i, argc, argv);
				break;
			case 'r': case 'R': processed = processROption(&i, argc, argv, infilesFlag);
				break;
			case 's': case 'S': processed = processSOption(&i, argc, argv);
				break;
			case 't': case 'T': processed = processTOption(&i, argc, argv);
				break;
			case 'u': case 'U': processed = processUOption(&i, argc, argv);
				break;
			case 'v': case 'V': processed = processVOption(&i, argc, argv);
				break;
			case 'w': case 'W': processed = processWOption(&i, argc, argv);
				break;
			case 'x': case 'X': processed = processXOption(&i, argc, argv);
				break;
			case 'y': case 'Y': processed = processYOption(&i, argc, argv);
				break;
			case 'z': case 'Z': processed = processZOption(&i, argc, argv);
				break;
			default: processed = 0;
			}
		} else {
			/* input file */
			processed = 1;
			if (infilesFlag == INFILES_ENABLED) {
				mainAddStringListOption(&s_opt.inputFiles, argv[i]);
			}
		}
		if (! processed) {
			sprintf(tmpBuff,"unknown option %s, (try xref -help)\n",argv[i]); 
			error(ERR_ST,tmpBuff);
			if (	s_opt.taskRegime==RegimeXref 
					|| 	s_opt.taskRegime==RegimeHtmlGenerate) {
				emergencyExit(XREF_EXIT_ERR);
			}
		}
	}
}

void mainScheduleInputFilesFromOptionsToFileTable() {
	S_stringList *ll;
	for(ll=s_opt.inputFiles; ll!=NULL; ll=ll->next) {
		mainProcessInFileOption(ll->d);
	}
}

/* *************************************************************************** */


static char * getInputFileFromFtab(int *fArgCount, int flag) {
	char		*res;
	int 		i;
	S_fileItem	*fi;
	for(i= *fArgCount; i<s_fileTab.size; i++) {
		fi = s_fileTab.tab[i];
		if (fi!=NULL) {
			if (flag==FF_SCHEDULED_TO_PROCESS&&fi->b.scheduledToProcess)break;
			if (flag==FF_COMMAND_LINE_ENTERED&&fi->b.commandLineEntered)break;
		}
	}
	*fArgCount = i;
	if (i<s_fileTab.size) return(s_fileTab.tab[i]->name);
	else return(NULL);
}

char * getInputFile(int *fArgCount) {
	return(getInputFileFromFtab(fArgCount,FF_SCHEDULED_TO_PROCESS));
}

static char * getCommandLineFile(int *fArgCount) {
	return(getInputFileFromFtab(fArgCount,FF_COMMAND_LINE_ENTERED));
}

static void mainGenerateReferenceFile() {
	char *inputRefFile;
	static int updateFlag = 0;
	if (s_opt.cxrefFileName == NULL) return;
	if (updateFlag == 0 && s_opt.update == 0) {
		genReferenceFile(0, s_opt.cxrefFileName);
		updateFlag = 1;
	} else {
		genReferenceFile(1,s_opt.cxrefFileName);
	}
}

static void schedulingUpdateToProcess(S_fileItem *p) {
	if (p->b.scheduledToUpdate && p->b.commandLineEntered) {
		p->b.scheduledToProcess = 1;
	}
}

static void schedulingToUpdate(S_fileItem *p, void *rs) {
	struct stat fstat,hstat,*refStat;
	char		sss[MAX_FILE_NAME_SIZE];
	refStat = (struct stat *) rs;
	if (p == s_fileTab.tab[s_noneFileIndex]) return;
//&	if (s_opt.update==UP_FAST_UPDATE && p->b.commandLineEntered == 0) return;
//&fprintf(dumpOut,"checking %s for update\n",p->name); fflush(dumpOut);
	if (statb(p->name, &fstat)) {
		// removed file, remove it from watched updates, load no reference
		if (p->b.commandLineEntered) {
			// no messages during refactorings
			if (s_ropt.refactoringRegime != RegimeRefactory) {
				sprintf(tmpBuff,"file %s not accessible",	p->name);
				warning(ERR_ST, tmpBuff);
			}
		}
		p->b.commandLineEntered = 0;
		p->b.scheduledToProcess = 0;
		p->b.scheduledToUpdate = 0;
		// (missing of following if) has caused that all class hierarchy items
		// as well as all cxreferences based in .class files were lost
		// on -update, a very serious bug !!!!
		if (p->name[0] != ZIP_SEPARATOR_CHAR) {
			p->b.cxLoading = 1;		/* Hack, to remove references from file */
		}
	} else if (s_opt.taskRegime == RegimeHtmlGenerate) {
		concatPathes(sss,MAX_FILE_NAME_SIZE,s_opt.htmlRoot,p->name,".html");
		strcat(sss, s_opt.htmlLinkSuffix);
		InternalCheck(strlen(sss) < MAX_FILE_NAME_SIZE-2);
		if (statb(sss, &hstat) || fstat.st_mtime >= hstat.st_mtime) {
			p->b.scheduledToUpdate = 1;
		}
	} else if (s_opt.update == UP_FULL_UPDATE) {
		if (fstat.st_mtime != p->lastFullUpdateMtime) {
			p->b.scheduledToUpdate = 1;
//&			p->lastFullUpdateMtime = fstat.st_mtime;
//&			p->lastUpdateMtime = fstat.st_mtime;
		}
	} else {
		if (fstat.st_mtime != p->lastUpdateMtime) {
			p->b.scheduledToUpdate = 1;
//&			p->lastUpdateMtime = fstat.st_mtime;
		}
	}
//&if (p->b.scheduledToUpdate) {fprintf(dumpOut,"scheduling %s to update\n", p->name); fflush(dumpOut);}
}

void searchDefaultOptionsFile(char *file, char *ttt, char *sect) {
	struct stat fst;
	int fnum, ii, findFlag=0;
	FILE *ff=NULL;
	FILE *ffn;
	int nargc,aaa,hlen;
	char **nargv,*hh;
	ttt[0] = 0; sect[0]=0;
	if (file == NULL) return;
	if (s_opt.stdopFlag || s_opt.no_stdop) return;
	/* first try to find section in HOME config. */
	getXrefrcFileName( ttt);
	ff = fopen(ttt,"r");
	if (ff!=NULL) {
		findFlag = readOptionFromFile(ff,&nargc,&nargv,MEM_NO_ALLOC,file,s_opt.project,sect);
		if (findFlag) {
			DPRINTF3("options file '%s' section '%s'\n",ttt,sect);
		}
		fclose(ff);		
	}
	if (findFlag) return;
	/* then look for source directory  'Xref.opt' */
	strcpy(ttt,normalizeFileName(file, s_cwd));
	for(; findFlag==0; ) {
		copyDir(ttt,ttt,&ii);
		if (ii == 0) break;
		InternalCheck(ii+15<MAX_FILE_NAME_SIZE);
		sprintf(ttt+ii,"Xref.opt");
/*fprintf(dumpOut,"try to open %s\n",ttt);*/
		if (stat(ttt,&fst)==0 && (fst.st_mode & S_IFMT) != S_IFDIR) {
			DPRINTF2("options file '%s'\n",ttt);
			findFlag = 1;
		} else {
			ttt[ii-1]=0;
		}
	}
	if (findFlag) {
		if (s_opt.taskRegime!=RegimeEditServer) {
			sprintf(tmpBuff,"%s\n\t\t%s", 
					"using of 'Xref.opt' file is an obsolete way of passing",
#ifdef __WIN32__	/*SBD*/
					"options to xref task. Please, use '%HOME%\\_c-xrefrc' file"
#else				/*SBD*/
#ifdef __OS2__		/*SBD*/
					"options to xref task. Please, use '%HOME%\\.c-xrefrc' file"
#else				/*SBD*/
					"options to xref task. Please, use '~/.c-xrefrc' file"
#endif				/*SBD*/
#endif				/*SBD*/
					);
			error(ERR_ST, tmpBuff);
		}
		return;
	}
	// finally if automatic selection did not find project, keep last one
	if (s_opt.project==NULL) {
		// but do this only if file is from cxfile, would be better to
		// check if it is from active project, but nothing is perfect

		// I should check here whether the project still exists in the .c-xrefrc file
		// it may happen that after deletion of the project, the request for active
		// project will return non-existent project.
		fnum = getFileNumberFromName(file);
		if (fnum!=s_noneFileIndex && s_fileTab.tab[fnum]->b.isFromCxfile) {
			strcpy(ttt, oldStdopFile);
			strcpy(sect, oldStdopSection);
			return;
		}
	}
	ttt[0]=0;
	return;
}

static void writeOptionsFileMessage( char *file, 
									 char *outFName, char *outSect ) {
	if (s_opt.refactoringRegime==RegimeRefactory) return;
	if (outFName[0]==0) {
		if (s_opt.project!=NULL) {
			sprintf(tmpBuff,"'%s' project options not found", 
					s_opt.project);
			if (s_opt.taskRegime == RegimeEditServer) {
				error(ERR_ST, tmpBuff);
			} else {
				fatalError(ERR_ST, tmpBuff, XREF_EXIT_NO_PROJECT);
			}
		} else if (! JAVA2HTML()) {
			if (s_opt.xref2) {
				ppcGenRecord(PPC_NO_PROJECT,file,"\n");
			} else {
				sprintf(tmpBuff,"no project name covers '%s'",file);
				warning(ERR_ST, tmpBuff);
			}
		}
	//&} else if (s_opt.xref2) {
		//& sprintf(tmpBuff,"C-xrefactory project: %s", outSect);
		//& ppcGenRecord(PPC_BOTTOM_INFORMATION, tmpBuff, "\n");
	} else if (s_opt.taskRegime==RegimeXref) {
		if (s_opt.xref2) {
			sprintf(tmpBuff,"C-xrefactory project: %s", outSect);
			ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
		} else {
			fprintf(dumpOut,"[C-xref] active project: '%s'\n", outSect);
			fflush(dumpOut);
		}
	}
}

static void handlePathologicProjectCases(char *file,char *outFName,char *outSect,int errMessage){
	// all this stuff should be reworked, but be very carefull when refactoring it
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime == RegimeEditServer) {
		if (errMessage!=NO_ERROR_MESSAGE) {
			writeOptionsFileMessage(file, outFName, outSect);
		}
	} else {
		if (*oldStdopFile == 0) {
			static int messageYetWritten=0;
			if (errMessage!=NO_ERROR_MESSAGE && messageYetWritten == 0) {
				messageYetWritten = 1;
				writeOptionsFileMessage(file, outFName, outSect);
			}
		} else {
			if (outFName[0]==0 || outSect[0]==0) {
				warning(ERR_ST,"no project name covers this file");
			}
			if (outFName[0]==0 && outSect[0]==0) {
				strcpy(outSect, oldStdopSection);
			}
			if (outFName[0]==0) {
				strcpy(outFName, oldStdopFile);
			}
			if(strcmp(oldStdopFile,outFName)||strcmp(oldStdopSection,outSect)){
				if (s_opt.xref2) {
					sprintf(tmpBuff, "[Xref] new project: '%s'", outSect);
					ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
				} else {
					fprintf(dumpOut,"[Xref] new project: '%s'\n", outSect);
				}
			}
		}
	}
}

static void getOptionsFile(char *file, char *outFName, char *outSect,int errMessage) {
	searchDefaultOptionsFile(file, outFName, outSect);
	handlePathologicProjectCases(file, outFName, outSect, errMessage);
	return;
}

static int computeAndOpenInputFile() {
	FILE 			*inputIn;
	S_editorBuffer 	*inputBuff;
	assert(s_language);
	inputBuff = NULL;
	//!!!! hack for .jar files !!!
	if (LANGUAGE(LAN_JAR) || LANGUAGE(LAN_CLASS)) return(0);
	if (s_input_file_name == NULL) {
		assert(0);
		inputIn = stdin;
		s_input_file_name = "__none__";
//&	} else if (s_opt.cxrefs == OLO_GET_ENV_VALUE) {
		// hack for getenv
		// not anymore, parse input for getenv ${__class}, ... settings
		// also if yes, then move this to 'mainEditSrvParseInputFile' !
//&		return(0);
	} else {
		inputIn = NULL;
		//& inputBuff = editorGetOpenedAndLoadedBuffer(s_input_file_name);
		inputBuff = editorFindFile(s_input_file_name);
		if (inputBuff == NULL) {
#if defined (__WIN32__) || defined (__OS2__)   	/*SBD*/
			inputIn = fopen(s_input_file_name,"rb");
#else					/*SBD*/
			inputIn = fopen(s_input_file_name,"r");
#endif					/*SBD*/
			if (inputIn == NULL) {
				error(ERR_CANT_OPEN, s_input_file_name);
			}
#if ZERO
		} else {
			ppcGenRecord(PPC_IGNORE,"LOADING BUFFERED FILE","");
			ppcGenRecord(PPC_IGNORE,s_input_file_name,"\n");
			inputBuff->a.text[inputBuff->a.bufferSize]=0;
			ppcGenRecord(PPC_IGNORE,inputBuff->a.text,"\n");			
#endif
		}
	}
	initInput(inputIn, inputBuff, "\n", s_input_file_name);
	if (inputIn==NULL && inputBuff==NULL) {
		return(0);
	} else {
		return(1);
	}
}

static void initOptions() {
	copyOptions(&s_opt, &s_initOpt);
	s_opt.stdopFlag = 0;
	s_input_file_number = s_noneFileIndex;
}

static void initDefaultCxrefFileName(char *inputfile) {
	int			ii;
	static char dcx[MAX_FILE_NAME_SIZE];
	copyDir(dcx, normalizeFileName(inputfile, s_cwd), &ii);
	InternalCheck(ii < MAX_FILE_NAME_SIZE);
	strcpy(&dcx[ii], DEFAULT_CXREF_FILE);
	InternalCheck(strlen(dcx) < MAX_FILE_NAME_SIZE);
	strcpy(dcx, getRealFileNameStatic(normalizeFileName(dcx, s_cwd)));
	InternalCheck(strlen(dcx) < MAX_FILE_NAME_SIZE);
	s_opt.cxrefFileName = dcx;
}

static void initializationsPerInvocation() {
	int i;
	s_cp = s_cpInit;
	s_cps = s_cpsInit;
	for(i=0; i<SPP_MAX; i++) s_spp[i] = s_noPos;
	s_cxRefFlag=0;
	s_cxRefPos = s_noPos;
	s_olstring[0]=0;
	s_olstringFound = 0;
	s_olstringServed = 0;
	s_olstringInMbody = NULL;
	s_yygstate = s_initYygstate;
	s_jsl = NULL;
	s_javaObjectSymbol = NULL;
}

static void fileTabInit() {
	int len;
	char *ff;
	struct fileItem ffi,*ffii;
	idTabNAInit( &s_fileTab,MAX_FILES);
	len = strlen(NON_FILE_NAME);
	FT_ALLOCC(ff, len+1, char);
	strcpy(ff, NON_FILE_NAME);
	FT_ALLOC(ffii, S_fileItem);
	FILLF_fileItem(ffii,ff, 0, 0,0,0, 0,0,0,0,0,0,0,0,0,s_noneFileIndex,
				   NULL,NULL,s_noneFileIndex, NULL);
	idTabAdd(&s_fileTab, ffii, &s_noneFileIndex);
}

/*///////////////////////// parsing /////////////////////////////////// */
static void mainParseInputFile() {
	if (s_language == LAN_JAVA) {
		uniyylval = & s_yygstate->gyylval;
		javayyparse();
	}
#	ifdef CCC_ALLOWED
	else if (s_language == LAN_CCC) {
		uniyylval = & cccyylval;
		cccyyparse();
	}
#	endif
#	ifdef YACC_ALLOWED
	else if (s_language == LAN_YACC) {
		uniyylval = & yaccyylval;
		yaccyyparse();
	}
#	endif
	else {
		uniyylval = & cyylval;
		cyyparse();
	}
	s_cache.activeCache = 0;
	cFile.fileName = NULL;
#ifdef LINEAR_ADD_REFERENCE
	purgeCxReferenceTable();
#endif

/*//////////////////////// after parsing actions ////////////////////// */
#if ZERO
if (s_opt.taskRegime==RegimeXref || s_opt.taskRegime==RegimeHtmlGenerate) { 
	int ui,el,mdp,i,cr,ri;
	S_symbol *pp;
	S_symbolRefItem *rfi;
	S_reference *rr;
fprintf(dumpOut,"olcxMemoryAllocatedBytes == %d\n", olcxMemoryAllocatedBytes);
//&{static int mp=0; int p; p=s_topBlock->firstFreeIndex*100/SIZE_workMemory;if (p>mp)mp=p;fprintf(dumpOut,": workmax == (%d%%)\n", mp);}
//&{static int mp=0; int p; p=ppmMemoryi*100/SIZE_ppmMemory;if (p>mp)mp=p;fprintf(dumpOut,": ppmmax == (%d%%) == %d\n", mp, SIZE_ppmMemory/100*mp);}
#if 1 //ZERO
//&	fprintf(dumpOut,"sizeof(S_symbol,S_fileItem,S_symbolRefItem,S_ref) == %d, %d, %d, %d\n",sizeof(S_symbol),sizeof(S_fileItem),sizeof(S_symbolRefItem),sizeof(S_reference));
//&fprintf(dumpOut,": mem == %d%% == %d/%d %x-%x\n", s_topBlock->firstFreeIndex*100/SIZE_workMemory, s_topBlock->firstFreeIndex,SIZE_workMemory,memory, memory+SIZE_workMemory);
//&	fprintf(dumpOut,": cxmem == %d/%d %x-%x\n",cxMemory->i,cxMemory->size, ((char*)&cxMemory->b),((char*)&cxMemory->b)+cxMemory->size);
//&	fprintf(dumpOut,": ftmem == %d/%d %x-%x\n",ftMemoryi,SIZE_ftMemory,ftMemory,ftMemory+SIZE_ftMemory);
//&fprintf(dumpOut,": ppmmem == (%d%%) == %d/%d %x-%x\n",ppmMemoryi*100/SIZE_ppmMemory,ppmMemoryi,SIZE_ppmMemory,ppmMemory,ppmMemory+SIZE_ppmMemory);
//&	fprintf(dumpOut,": tmpMem == %d/%d %x-%x\n",tmpWorkMemoryi,SIZE_tmpWorkMemory,tmpWorkMemory,tmpWorkMemory+SIZE_tmpWorkMemory);
//&	fprintf(dumpOut,": cachedLex == %d/%d in %d cps\n",s_cache.lbcc-s_cache.lb,
//&						LEX_BUF_CACHE_SIZE,s_cache.cpi);
//&	fprintf(dumpOut,": cache[cpi].lb == %x <-> %x\n",s_cache.lbcc,
//&			s_cache.cp[s_cache.cpi-1].lbcc);
#endif
#if 1 //ZERO
	symTabStatistics(s_symTab, &ui, &el,&mdp);
	fprintf(dumpOut,": symtab == %d elems, usage %d/%d,\tratio %1.2f, maxdeep==%d\n",
				el,ui,s_symTab->size, ((float)el)/(ui+1e-30), mdp);
	fflush(dumpOut);
	refTabStatistics(&s_cxrefTab, &ui, &el,&mdp);
	fprintf(dumpOut,": reftab == %d elems, usage %d/%d,\tratio %1.2f, maxdeep==%d\n",
		el,ui,s_cxrefTab.size, ((float)el)/(ui+1e-30), mdp);
	for(i=0; i<	s_cxrefTab.size; i++) {
		cr = 0;
		for(rfi=s_cxrefTab.tab[i]; rfi!=NULL; rfi=rfi->next) {
			cr++;
			ri = 0;
			for(rr=rfi->refs; rr!=NULL; rr=rr->next) ri++;
			if (ri>1000) {
				fprintf(dumpOut,"\nsymbol %s has %d references", rfi->name, ri);
			}
		}
		if (cr>1000) {
			fprintf(dumpOut,"\n\n>>symbol exceeding 1000");
			fprintf(dumpOut," at index %d: ", i);
			for(rfi=s_cxrefTab.tab[i]; rfi!=NULL; rfi=rfi->next) {
				fprintf(dumpOut,"%s ", rfi->name);
			}
			fprintf(dumpOut,"\n");
		}
	}
#endif
}
#endif
}


void mainSetLanguage(char *inFileName, int *outLanguage) {
	char *suff;
	if (inFileName == NULL
		|| fileNameHasOneOfSuffixes(inFileName, s_opt.javaFilesSuffixes)
		|| (fnnCmp(simpleFileName(inFileName), "Untitled-", 9)==0)  // jEdit unnamed buffer
		) {
		*outLanguage = LAN_JAVA;
		typesName[TypeStruct] = "class";
	} else {
		suff = getFileSuffix(inFileName);
		if (fnCmp(suff,".zip")==0 || fnCmp(suff,".jar")==0) {
			*outLanguage = LAN_JAR;
		} else if (fnCmp(suff,".class")==0) {
			*outLanguage = LAN_CLASS;
#	ifdef YACC_ALLOWED
		} else if (fnCmp(suff,".y")==0) {
			*outLanguage = LAN_YACC;
			typesName[TypeStruct] = "struct";
#	endif
#	ifdef CCC_ALLOWED
		} else if (fileNameHasOneOfSuffixes(inFileName, s_opt.cppFilesSuffixes)) {
			*outLanguage = LAN_CCC;
			typesName[TypeStruct] = "class";
#	endif
		} else {
			*outLanguage = LAN_C;
			typesName[TypeStruct] = "struct";
		}
	}
}

void getAndProcessXrefrcOptions(char *dffname, char *dffsect,char *project) {
	int dfargc; 
	char **dfargv;
	if (*dffname != 0 && s_opt.stdopFlag==0 && s_opt.no_stdop==0) {
		readOptionFile(dffname,&dfargc,&dfargv,dffsect,project);
		// warning, the following can overwrite variables like 
		// 's_cxref_file_name' allocated in PPM_MEMORY, then when memory
		// is got back by caching, it may provoke a problem
		processOptions(dfargc, dfargv, INFILES_DISABLED); /* .c-xrefrc opts*/
	}
}

static void checkExactPositionUpdate(int message) {
	if (s_opt.update == UP_FAST_UPDATE && s_opt.exactPositionResolve) {
		s_opt.update = UP_FULL_UPDATE;
		if (message) {
			warning(ERR_ST,"-exactpositionresolve implies full update");
		}
	}
}

#if ZERO
// this will not work, before options are not reloaded when only
// file changes, but no project
static void setPredefinedFileEnvVariables(char *fileName) {
	char *fvv;
	fvv = getRealFileNameStatic(fileName);
	strcpy(s_file, fvv);
	InternalCheck(strlen(s_file) < MAX_FILE_NAME_SIZE-1);
	xrefSetenv("__FILE", s_file);
	strcpy(s_path, directoryName_st(fvv));
	xrefSetenv("__PATH", s_path);
	xrefSetenv("__FNAME", simpleFileName(s_file));
	strcpy(s_name, simpleFileNameWithoutSuffix_st(fvv));
	xrefSetenv("__NAME", s_name);
}
#endif

static void writeProgressInformation(int progress) {
	static int 		lastprogress;
	static time_t	timeZero;
	static int		dialogDisplayed = 0;
	static int		initialCall = 1;
	time_t			ct;
	if (progress == 0 || initialCall) {
		initialCall = 0;
		dialogDisplayed = 0;
		lastprogress = 0;
		timeZero = time(NULL);
	} else {
		if (progress <= lastprogress) return;
	}
	ct = time(NULL);
	// write progress only if it seems to be longer than 3 sec
	if (dialogDisplayed
		|| (progress == 0 && ct-timeZero > 1) 
		|| (progress != 0 && ct-timeZero >= 1 && 100*((double)ct-timeZero)/progress > 3)
		) {
		if (! dialogDisplayed) {
			// display progress bar
			fprintf(stdout,"<%s>0 \n", PPC_PROGRESS);
			dialogDisplayed = 1;
		}
		fprintf(stdout,"<%s>%d \n", PPC_PROGRESS, progress);
		fflush(stdout);
		lastprogress = progress;
	}
}

void writeRelativeProgress(int val) {
	writeProgressInformation((100*s_progressOffset + val)/s_progressFactor);
	if (val==100) s_progressOffset++;
}

static void mainFileProcessingInitialisations(
					int *firstPassing,
					int argc, char **argv,		// command-line options
					int nargc, char **nargv,	// piped options
					int *outInputIn,
					int *outLanguage
	) {
	int 			stargc; 
	char 			**stargv;
	int 			dfargc; 
	char 			**dfargv;
	char 			dffname[MAX_FILE_NAME_SIZE];
	char 			dffsect[MAX_FILE_NAME_SIZE];
	char 			*ss, *fvv;
	struct stat 	dffstat;
	char 			*fileName;
	int				lc;

	fileName = s_input_file_name;
	mainSetLanguage(fileName,  outLanguage);
	getOptionsFile(fileName, dffname, dffsect,DEFAULT_VALUE);
	initAllInputs();
	if (dffname[0] != 0 ) stat(dffname, &dffstat);
	else dffstat.st_mtime = oldStdopTime;				// !!! just for now
//&fprintf(dumpOut,"checking oldcp==%s\n",oldOnLineClassPath);
//&fprintf(dumpOut,"checking newcp==%s\n",s_opt.classpath);
	if (	*firstPassing 
			|| oldCppPass != s_currCppPass
			|| strcmp(oldStdopFile,dffname) 
			|| strcmp(oldStdopSection,dffsect) 
			|| oldStdopTime != dffstat.st_mtime
			|| oldLanguage!= *outLanguage
			|| strcmp(oldOnLineClassPath, s_opt.classpath)
			|| s_cache.cpi == 1     /* some kind of reset was made */
		) {
		if (*firstPassing) {
			initCaching();
			*firstPassing = 0;
		} else {
			recoverCachePointZero();
		}
		strcpy(oldOnLineClassPath, s_opt.classpath);
		InternalCheck(strlen(oldOnLineClassPath)<MAX_OPTION_LEN-1);
		s_opt.stdopFlag = 0;
		initPreCreatedTypes();
		initCwd();
		initOptions();
		initDefaultCxrefFileName(fileName);
		// this maybe useless here, it is reseted after reading options
		mainSetLanguage(fileName,  outLanguage);
		getStandardOptions(&stargc,&stargv);
		processOptions(stargc,stargv, INFILES_DISABLED); /* default options */
		processOptions(argc, argv, INFILES_DISABLED);	/* command line opts */
		/* piped options (no include or define options)
		   must be befor .xrefrc file options, but, the s_cachedOPtions
		   must be set after .c-xrefrc file, but s_cachedOptions can't contain
           piped options, !!! berk.
		*/
		{ 
			copyOptions(&s_tmpOptions, &s_opt);
			processOptions(nargc, nargv, INFILES_DISABLED); 
			// get options file once more time, because of -license ???
			// if takes into account the -p option from piped options
			// but copy new project name into old to avoid warning message
			strcpy(oldStdopFile,dffname);
			strcpy(oldStdopSection,dffsect);
			getOptionsFile(fileName, dffname, dffsect,DEFAULT_VALUE);
//&		s_tmpOptions.setGetEnv = s_opt.setGetEnv; // hack, take new env. vals
			copyOptions(&s_opt, &s_tmpOptions);
		}
		//& setPredefinedFileEnvVariables(fileName);
		reInitCwd(dffname, dffsect);
		getAndProcessXrefrcOptions(dffname, dffsect, dffsect);
		if (s_opt.taskRegime != RegimeEditServer && s_input_file_name == NULL) {
			*outInputIn = 0;
			goto fini;
		}
		copyOptions(&s_cachedOptions, &s_opt);  // before getJavaClassPath, it modifies ???
		processOptions(nargc, nargv, INFILES_DISABLED); 
		getJavaClassAndSourcePath();
		*outInputIn = computeAndOpenInputFile();
		strcpy(oldStdopFile,dffname);
		strcpy(oldStdopSection,dffsect);
		oldStdopTime = dffstat.st_mtime;
		oldLanguage = *outLanguage;
		oldCppPass = s_currCppPass;
		// this was before 'getAndProcessXrefrcOptions(df...' I hope it will not cause
		// troubles to move it here, because of autodetection of -javaVersion from jdkcp
		initTokenNameTab();
		s_cache.activeCache = 1;
		poseCachePoint(0);
		s_cache.activeCache = 0;
		assert(s_cache.lbcc == s_cache.cp[0].lbcc);
		assert(s_cache.lbcc == s_cache.cp[1].lbcc);
	} else {
		copyOptions(&s_opt, &s_cachedOptions);
		processOptions(nargc, nargv, INFILES_DISABLED); /* no include or define options */
		/*& // if someone understand why this is here, I will uncomment it 
		  searchDefaultOptionsFile(s_input_file_name, dffname, dffsect);
		  &*/
		*outInputIn = computeAndOpenInputFile();
	}
	// reset language once knowing all language suffixes
	mainSetLanguage(fileName,  outLanguage);
	s_input_file_number = cFile.lb.cb.fileNumber;
	assert(s_opt.taskRegime);
	if (	(s_opt.taskRegime==RegimeXref 
			 || s_opt.taskRegime==RegimeHtmlGenerate)
			&& (! s_javaPreScanOnly)) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_INFORMATION, getRealFileNameStatic(s_input_file_name), "\n");
		} else {
			fprintf(dumpOut,"'%s'\n", getRealFileNameStatic(s_input_file_name));
		}
		fflush(dumpOut);
	}
 fini:
	initializationsPerInvocation();
	// some final touch to options
	if (s_opt.keep_old) {
		s_opt.update = UP_FAST_UPDATE;
		s_opt.updateOnlyModifiedFiles = 0;
	}
	if (s_opt.brief) s_opt.long_cxref = 0;
	else s_opt.long_cxref = 1;
	if (s_opt.debug) errOut = dumpOut;
	checkExactPositionUpdate(0);
	// so s_input_file_number is not set if the file is not really opened!!!
}

static void createXrefrcDefaultLicense() {
	char 			fn[MAX_FILE_NAME_SIZE];
	struct stat		st;
	FILE			*ff;
	int				rand,rr,ed,em,ey,eh,emi,es,i;
	time_t			tt;
	struct tm		*tmm;
	char			*lic,*own,*ss;
	// if this is registered binary distribution, return
	//& if (s_initOpt.licenseString[0] == '0') return;
	getXrefrcFileName(fn);
	if (stat(fn, &st)!=0) {
		// does not exists
		ff = fopen(fn,"w");
		if (ff == NULL) {
			sprintf(tmpBuff, "home directory %s does not exists", fn);
			fatalError(ERR_ST,tmpBuff, XREF_EXIT_ERR);
		} else {
#if ZERO
#ifdef BIN_RELEASE
			tt = time(NULL);
			tmm = localtime(&tt);
			UNFILL_TM(tmm,ed,em,ey,eh,emi,es);
			own = "evaluation"; rr = 1;
			ed += 15;
			lic = stringNumStr(rr, ed, em, ey, own);
			fprintf(ff, "\n\n// license string:\n");
			fprintf(ff, "-license=%d/%d/%d/%d:%s:%s\n\n", 
					em,ed,ey,rr,own,lic);
#endif
#endif
			fclose(ff);
		}
	}
}

static int power(int x, int y) {
	int i,res = 1;
	for(i=0; i<y; i++) res *= x;
	return(res);
}

static void mainTotalTaskEntryInitialisations(int argc, char **argv) {
	int mm;
	errOut = stderr;
	dumpOut = stdout; /*fopen("/dev/tty6","w");*/
	s_fileAbortionEnabled = 0;
//&dumpOut = fopen("/dev/tty2","w");
//&fprintf(dumpOut,"\nPROCESS START!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(dumpOut);
/*	dumpOut = errOut = stderr; */
	cxOut = stdout;
	ccOut = stdout;
	if (s_opt.taskRegime == RegimeEditServer) errOut = stdout;

	assert(MAX_TYPE < power(2,SYMTYPES_LN));
	assert(MAX_STORAGE < power(2,STORAGES_LN));
	assert(MAX_SCOPES < power(2,SCOPES_LN));
	assert(MAX_REQUIRED_ACCESS < power(2, MAX_REQUIRED_ACCESS_LN));

	assert(PPC_MAX_AVAILABLE_REFACTORINGS < MAX_AVAILABLE_REFACTORINGS);

	// initialize cxMemory
	mm = cxMemoryOverflowHandler(1);
	assert(mm);
	// initoptions
	FILL_memory(((S_memory*)&s_initOpt.pendingMemory), 
				optionsOverflowHandler, 0, SIZE_opiMemory, 0);

	// just for very beginning
	s_fileProcessStartTime = time(NULL);

	// following will be displayed only at third pass or so, because
	// s_opt.debug is set only after passing through option processing
	DPRINTF("Initialisations.\n");
	memset(&s_count, 0, sizeof(S_counters));
	s_opt.includeDirs = NULL;
	SM_INIT(ftMemory);
	FT_ALLOCC(s_fileTab.tab, MAX_FILES, struct fileItem *);\
	FILL_EXP_COMMAND();\
    fileTabInit();\
	FILL_position(&s_noPos, s_noneFileIndex, 0, 0);
	FILL_usageBits(&s_noUsage, UsageNone, 0, 0);
	FILL_reference(&s_noRef, s_noUsage, s_noPos, NULL);
	s_input_file_number = s_noneFileIndex;
	s_javaAnonymousClassName.p = s_noPos;
	olcxInit();
	editorInit();
}

static void mainReinitFileTabEntry(S_fileItem *ft, int i) {
	// well be less strict
	//&FILLF_fileItem(ft, ft->name, 0, 0,0,0, 0,0,0,0,0,0,0,0,0,s_noneFileIndex,
	//&			   NULL,NULL,s_noneFileIndex, NULL);
	ft->infs = ft->sups = NULL;
	ft->directEnclosingInstance = s_noneFileIndex;
	ft->b.scheduledToProcess = 0;
	ft->b.scheduledToUpdate = 0;
	ft->b.fullUpdateIncludesProcessed = 0;
	ft->b.cxLoaded = ft->b.cxLoading = ft->b.cxSaved = 0;
}

void mainTaskEntryInitialisations(int argc, char **argv) {
	char 		tt[MAX_FILE_NAME_SIZE];
	char 		ttt[MAX_FILE_NAME_SIZE];
	char 		dffname[MAX_FILE_NAME_SIZE];
	char 		dffsect[MAX_FILE_NAME_SIZE];
	char 		relcwd[MAX_FILE_NAME_SIZE];
	char		*ss;
	int			dfargc;
	char		**dfargv;
	int 		argcount;
	char 		*sss,*cmdlnInputFile;
	int			inmode,ii,topCallFlag, noerropt;
	void		*recursFlag;
	static int 	firstmemory=0;

	s_fileAbortionEnabled = 0;

	// supposing that file table is still here, but reinit it
	idTabMap3(&s_fileTab, mainReinitFileTabEntry);

	DM_INIT(cxMemory);
	// the following causes long jump, berk.
	CX_ALLOCC(sss, CX_MEMORY_CHUNK_SIZE, char);
	CX_FREE_UNTIL(sss);
	CX_ALLOCC(s_cxrefTab.tab,MAX_CXREF_SYMBOLS, struct symbolRefItem *);\
																			refTabNAInit( &s_cxrefTab,MAX_CXREF_SYMBOLS);\
																															 if (firstmemory==0) {firstmemory=1;SET_EXPIRATION();}\
																																													  SM_INIT(ppmMemory);
	ppMemInit();
	stackMemoryInit();

	// init options as soon as possible! for exampl initCwd needs them
	initOptions();

	XX_ALLOC(s_symTab, S_symTab);
	symTabInit( s_symTab, MAX_SYMBOLS);
	FILL_javaStat(&s_initJavaStat,NULL,NULL,NULL,0, NULL, NULL, NULL,
				  s_symTab,NULL,ACC_DEFAULT,s_cpInit,s_noneFileIndex,NULL);
	XX_ALLOC(s_javaStat, S_javaStat);
	*s_javaStat = s_initJavaStat;
	javaFqtTabInit( &s_javaFqtTab, FQT_CLASS_TAB_SIZE);
	// initialize recursive java parsing
	XX_ALLOC(s_yygstate, struct yyGlobalState);
	memset(s_yygstate, 0, sizeof(struct yyGlobalState));
	s_initYygstate = s_yygstate;

	initAllInputs();
	oldStdopFile[0] = 0;	oldStdopSection[0] = 0;
	initCwd();
	initTypeCharCodeTab();
	initTypeModifiersTabs();
	initJavaTypePCTIConvertIniTab();
	initTypesNamesTab();
	initExtractStoragesNameTab();
	FILL_caching(&s_cache,0,0,0,NULL,NULL,NULL,NULL);
	initArchaicTypes();
	oldStdopFile[0] = oldStdopSection[0] = 0;
	/* now pre-read the option file */
	processOptions(argc, argv, INFILES_ENABLED);
	mainScheduleInputFilesFromOptionsToFileTable();
	if (s_opt.refactoringRegime == RegimeRefactory) {
		// some more memory for refactoring task
		assert(s_opt.cxMemoryFaktor>=1);
		CX_ALLOCC(sss, 6*s_opt.cxMemoryFaktor*CX_MEMORY_CHUNK_SIZE, char);
		CX_FREE_UNTIL(sss);
	}
	if (s_opt.taskRegime==RegimeXref || s_opt.taskRegime==RegimeHtmlGenerate) {
		// get some memory if cross referencing
		assert(s_opt.cxMemoryFaktor>=1);
		CX_ALLOCC(sss, 3*s_opt.cxMemoryFaktor*CX_MEMORY_CHUNK_SIZE, char);
		CX_FREE_UNTIL(sss);
	}
	if (s_opt.cxMemoryFaktor > 1) {
		// reinit cxmemory taking into account -mf
		// just make an allocation provoking resizing
		CX_ALLOCC(sss, s_opt.cxMemoryFaktor*CX_MEMORY_CHUNK_SIZE, char);
		CX_FREE_UNTIL(sss);
	}
	// must be after processing command line options
	createXrefrcDefaultLicense();
	initCaching(); 
	// enclosed in cache point, because of persistent #define in XrefEdit
	argcount = 0;
	s_input_file_name = cmdlnInputFile = getCommandLineFile(&argcount);
	if (s_input_file_name==NULL) {
		ss = strmcpy(tt, s_cwd);
		if (ss!=tt && ss[-1] == SLASH) ss[-1]=0;
		assert(strlen(tt)+1<MAX_FILE_NAME_SIZE);
		s_input_file_name=tt;
//&fprintf(dumpOut,"here we are '%s'\n",tt);fflush(dumpOut);
	} else {
		strcpy(tt, s_input_file_name);
	}
	getOptionsFile(tt, dffname, dffsect, NO_ERROR_MESSAGE);
	reInitCwd(dffname, dffsect);
	if (dffname[0]!=0) {
		readOptionFile(dffname, &dfargc, &dfargv, dffsect, dffsect);
		if (s_opt.refactoringRegime == RegimeRefactory) {
			inmode = INFILES_DISABLED;
		} else if (s_opt.taskRegime==RegimeEditServer) {
			inmode = INFILES_DISABLED;
		} else if (s_opt.create || s_opt.project!=NULL || s_opt.update) {
			inmode = INFILES_ENABLED;
		} else {
			inmode = INFILES_DISABLED;
		}
		// disable error reporting on xref task on this pre-reading of .c-xrefrc
		if (s_opt.taskRegime==RegimeEditServer) {
			noerropt = s_opt.noErrors;
			s_opt.noErrors = 1;
		}
		// there is a problem with INFILES_ENABLED (update for safetycheck),
		// I should first load cxref file, in order to protect file numbers.
		if (inmode==INFILES_ENABLED && s_opt.update && !s_opt.create) {
			//&fprintf(dumpOut,"PREREADING !!!!!!!!!!!!!!!!\n");
			// this makes a problem: I need to preread cxref file before
			// reading input files in order to preserve hash numbers, but 
			// I need to read options first in order to have the name 
			// of cxref file.
			// I need to read fstab also to remove removed files on update
			processOptions(dfargc, dfargv, INFILES_DISABLED);
			smartReadFileTabFile();
		}
		processOptions(dfargc, dfargv, inmode);
		// recover value of errors messages
		if (s_opt.taskRegime==RegimeEditServer) s_opt.noErrors = noerropt;
		checkExactPositionUpdate(0);
		if (inmode == INFILES_ENABLED) mainScheduleInputFilesFromOptionsToFileTable();
	}
	recoverCachePointZero();

	s_opt.stdopFlag = 0;
//&	getJavaClassAndSourcePath();
	initCaching();

	DPRINTF("Leaving all task initialisations.\n");
}

static void mainReferencesOverflowed(char *cxMemFreeBase, int mess) {
	int i,fi,savingFlag;
	if (mess!=MESS_NONE && s_opt.taskRegime!=RegimeHtmlGenerate) {
		if (s_opt.xref2) {
			ppcGenRecord(PPC_INFORMATION,"swapping references on disk", "\n");
			ppcGenRecord(PPC_INFORMATION,"", "\n");
		} else {
			fprintf(dumpOut,"\nswapping references on disk (please wait)\n");
			fflush(dumpOut);
		}
	}
	if (s_opt.cxrefFileName == NULL) {
		fatalError(ERR_ST,"sorry no file for cxrefs, use -refs option", XREF_EXIT_ERR);
	}
	for(i=0; i<inStacki; i++) {
		if (inStack[i].lb.cb.ff != stdin) {
			fi = inStack[i].lb.cb.fileNumber;
			assert(s_fileTab.tab[fi]);
			s_fileTab.tab[fi]->b.cxLoading = 0;
			if (inStack[i].lb.cb.ff!=NULL) charBuffClose(&inStack[i].lb.cb);
		}
	}
	if (cFile.lb.cb.ff != stdin) {
		fi = cFile.lb.cb.fileNumber;
		assert(s_fileTab.tab[fi]);
		s_fileTab.tab[fi]->b.cxLoading = 0;
		if (cFile.lb.cb.ff!=NULL) charBuffClose(&cFile.lb.cb);
	}
	if (s_opt.taskRegime==RegimeHtmlGenerate) {
	  if (s_opt.noCxFile) {
		sprintf(tmpBuff,"cross-references overflowed, use -mf<n> option");
		fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
	  }
	  generateHtml();
	}
	if (s_opt.taskRegime==RegimeXref) mainGenerateReferenceFile();
	recoverMemoriesAfterOverflow(cxMemFreeBase);
	/* ************ start with CXREFS and memories clean ************ */
	savingFlag = 0;
	for(i=0; i<s_fileTab.size; i++) {
		if (s_fileTab.tab[i]!=NULL) {
			if (s_fileTab.tab[i]->b.cxLoading) {
				s_fileTab.tab[i]->b.cxLoading = 0;
				s_fileTab.tab[i]->b.cxSaved = 1;
				if (s_fileTab.tab[i]->b.commandLineEntered 
					|| !s_opt.multiHeadRefsCare) savingFlag = 1;
				// before, but do not work as scheduledToProcess is auto-cleared 
//&				if (s_fileTab.tab[i]->b.scheduledToProcess 
//&					|| !s_opt.multiHeadRefsCare) savingFlag = 1;
//&fprintf(dumpOut," -># '%s'\n",s_fileTab.tab[i]->name);fflush(dumpOut);
			}
		}
	}
	if (savingFlag==0 && mess!=MESS_FILE_ABORT) { 
		/* references overflowed, but no whole file readed */
		fatalError(ERR_NO_MEMORY,"cxMemory", XREF_EXIT_ERR);
	}
}

void getPipedOptions(int *outNargc,char ***outNargv){
	char nsect[MAX_FILE_NAME_SIZE];
	int c;
	*outNargc = 0;
	assert(s_opt.taskRegime);
	if (s_opt.taskRegime == RegimeEditServer) {
		readOptionFromFile(stdin,outNargc,outNargv,MEM_ALLOC_ON_SM,
								"",NULL,nsect);
		/* those options can't contain include or define options, */
		/* sections neither */
		c = getc(stdin);
		if (c==EOF) fatalError(ERR_INTERNAL, "broken input pipe", XREF_EXIT_ERR);
	}
}

static void fillIncludeRefItem( S_symbolRefItem *ddd , int fnum) {
	FILL_symbolRefItemBits(&ddd->b,TypeCppInclude,StorageExtern,
						   ScopeGlobal,ACC_DEFAULT,CatGlobal,0);
	FILL_symbolRefItem(ddd,LINK_NAME_INCLUDE_REFS,
					   cxFileHashNumber(LINK_NAME_INCLUDE_REFS),
					   fnum, fnum, ddd->b,NULL,NULL);
}

static void makeIncludeClosureOfFilesToUpdate() {
	char				*cxFreeBase;
	int					i,ii,fileAddedFlag, isJavaFileFlag;
	S_fileItem			*fi,*includer;
	S_symbolRefItem		sri,ddd,*memb;
	S_reference			*rr;
	CX_ALLOCC(cxFreeBase,0,char);
	readOneAppropReferenceFile(LINK_NAME_INCLUDE_REFS,
							   s_cxFullUpdateScanFunTab); // get include refs
	// iterate over scheduled files
	fileAddedFlag = 1;
	while (fileAddedFlag) {
		fileAddedFlag = 0;
		for(i=0; i<s_fileTab.size; i++) {
			fi = s_fileTab.tab[i];
			if (fi!=NULL && fi->b.scheduledToUpdate 
				&& !fi->b.fullUpdateIncludesProcessed) {
				fi->b.fullUpdateIncludesProcessed = 1;
				isJavaFileFlag = fileNameHasOneOfSuffixes(fi->name, s_opt.javaFilesSuffixes);
				fillIncludeRefItem( &ddd, i);
				if (refTabIsMember(&s_cxrefTab, &ddd, &ii,&memb)) {
					for(rr=memb->refs; rr!=NULL; rr=rr->next) {
						includer = s_fileTab.tab[rr->p.file];
						assert(includer);
						if (includer->b.scheduledToUpdate == 0) {
							includer->b.scheduledToUpdate = 1;
							fileAddedFlag = 1;
							if (isJavaFileFlag) {
								// no transitive closure for Java
								includer->b.fullUpdateIncludesProcessed = 1;
							}
						}
					}
				}
			}
		}
	}
	recoverMemoriesAfterOverflow(cxFreeBase);
}

static void scheduleModifiedFilesToUpdate() {
	char 		ttt[MAX_FILE_NAME_SIZE];
	char		*filestab;
	struct stat	refStat;
	char		*fnamesuff;
	checkExactPositionUpdate(1);
	if (s_opt.refnum <= 1) {
		fnamesuff = "";
		filestab = s_opt.cxrefFileName;
	} else {
		fnamesuff = PRF_FILES;
		sprintf(ttt,"%s%s", s_opt.cxrefFileName, fnamesuff);
		InternalCheck(strlen(ttt) < MAX_FILE_NAME_SIZE-1);
		filestab = ttt;
	}
	if (statb(filestab, &refStat)) refStat.st_mtime = 0;
	scanReferenceFile(s_opt.cxrefFileName, fnamesuff,"", s_cxScanFileTab);
	idTabMap2(&s_fileTab, schedulingToUpdate, &refStat);
	if (s_opt.update==UP_FULL_UPDATE /*& && !LANGUAGE(LAN_JAVA) &*/) {
		makeIncludeClosureOfFilesToUpdate();
	}
	idTabMap(&s_fileTab, schedulingUpdateToProcess);
}


void resetPendingSymbolMenuData() {
	S_olcxReferences *rstack;
	olcxSetCurrentUser(s_opt.user);
	rstack = s_olcxCurrentUser->browserStack.top;
	if (rstack == NULL) return;
	if (rstack->menuSym != NULL) {
		olcxFreeResolutionMenu(rstack->menuSym);
		rstack->menuSym = NULL;
	}
}

void mainCloseOutputFile() {
	if (ccOut!=stdout) {
//&fprintf(dumpOut,"CLOSING OUTPUT FILE\n");
		fclose(ccOut);
		ccOut = stdout;
	}
	errOut = ccOut;
	dumpOut = ccOut;
}

void mainOpenOutputFile(char *ofile) {
	mainCloseOutputFile();
	if (ofile!=NULL) {
//&fprintf(dumpOut,"OPENING OUTPUT FILE %s\n", s_opt.outputFileName);
#if defined (__WIN32__) || defined (__OS2__)	/*SBD*/
		// open it as binary file, so that record lengths will be correct
		ccOut = fopen(ofile,"wb");
#else					/*SBD*/
		ccOut = fopen(ofile,"w");
#endif					/*SBD*/
	} else {
		ccOut = stdout;
	}
	if (ccOut == NULL) {
		error(ERR_CANT_OPEN, ofile);
		ccOut = stdout;
	}
	errOut = ccOut;
	dumpOut = ccOut;
}

static int scheduleFileUsingTheMacro() {
	int 				rr,ii;
	S_symbolRefItem 	ddd,*memb;
	S_olSymbolsMenu		mm, *oldMenu;
	S_olcxReferences 	*tmpc;
	assert(s_olstringInMbody);
	tmpc = NULL;
	FILL_symbolRefItemBits(&ddd.b, TypeMacro, StorageExtern,
						   ScopeGlobal,ACC_DEFAULT,CatGlobal,0);
	FILL_symbolRefItem(&ddd,s_olstringInMbody,
					   cxFileHashNumber(s_olstringInMbody),
					   s_noneFileIndex, s_noneFileIndex,ddd.b,NULL,NULL);

//&	rr = refTabIsMember(&s_cxrefTab, &ddd, &ii, &memb);
//&	assert(rr);
//&	if (rr==0) return(s_noneFileIndex);

	FILL_olSymbolsMenu(&mm, ddd, 1,1,0,UsageUsed,0,0,0,UsageNone,s_noPos,0, NULL, NULL);
	if (s_olcxCurrentUser==NULL || s_olcxCurrentUser->browserStack.top==NULL) {
		olcxSetCurrentUser(s_opt.user);
		olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
		assert(s_olcxCurrentUser);
		tmpc = s_olcxCurrentUser->browserStack.top;
	}
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	oldMenu = s_olcxCurrentUser->browserStack.top->menuSym;
	s_olcxCurrentUser->browserStack.top->menuSym = &mm;
	s_olMacro2PassFile = s_noneFileIndex;
//&fprintf(dumpOut,":here I am, looking for usage of %s\n",memb->name);
	readOneAppropReferenceFile(s_olstringInMbody,s_cxScanFunTabFor2PassMacroUsage);
	s_olcxCurrentUser->browserStack.top->menuSym = oldMenu;
	if (tmpc!=NULL) {
		olStackDeleteSymbol(tmpc);
	}
//&fprintf(dumpOut,":scheduling file %s\n", s_fileTab.tab[s_olMacro2PassFile]->name); fflush(dumpOut);
	if (s_olMacro2PassFile == s_noneFileIndex) return(s_noneFileIndex);
	return(s_olMacro2PassFile);
}

// this is necessary to put new mtimies for header files
static void setFullUpdateMtimesInFileTab(S_fileItem *fi) {
	if (fi->b.scheduledToUpdate || s_opt.create) {
		fi->lastFullUpdateMtime = fi->lastModif;
	}
}

static void setUpdateMtimesInFileTab(S_fileItem *fi) {
	if (fi->b.scheduledToUpdate || s_opt.create) {
		fi->lastUpdateMtime = fi->lastModif;
	}
}

static void mainCloseInputFile(int inputIn ) {
	if (inputIn) {
		if (cFile.lb.cb.ff!=stdin) {
			charBuffClose(&cFile.lb.cb);
		}
	}
}

static void mainEditSrvParseInputFile( int *firstPassing, int inputIn ) {
//&fprintf(dumpOut,":here I am %s\n",s_fileTab.tab[s_input_file_number]->name);
	if (inputIn) {
//&fprintf(dumpOut,"parse start\n");fflush(dumpOut);
		if (s_opt.cxrefs!=OLO_TAG_SEARCH && s_opt.cxrefs!=OLO_PUSH_NAME) {
			recoverFromCache();
			mainParseInputFile();
//&fprintf(dumpOut,"parse stop\n");fflush(dumpOut);
			*firstPassing = 0;
		}
		cFile.lb.cb.isAtEOF = 0;
		mainCloseInputFile(inputIn);
	}
}

static void mainPushThisFileIncludeReferences(int fnum) {
	S_symbolRefItem sref;
	fillIncludeRefItem(&sref,fnum);
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	olAddBrowsedSymbol(&sref, &s_olcxCurrentUser->browserStack.top->hkSelectedSym,
					   1, 1, 0, UsageDefined,0,&s_noPos, UsageNone);
}

static int mainSymbolCanBeIdentifiedByPosition(int fnum) {
	int 	line,col;
	char dffname[MAX_FILE_NAME_SIZE];
	char dffsect[MAX_FILE_NAME_SIZE];
	// there is a serious problem with options memory for options got from 
	// the .c-xrefrc file. so for the moment this will not work.
    // which problem ??????
	// seems that those options are somewhere in ppmMemory overwritten?
//&return(0); 
	if (!creatingOlcxRefs()) return(0);
	if (s_opt.browsedSymName == NULL) return(0);
//&fprintf(dumpOut,"looking for sym %s on %s\n",s_opt.browsedSymName,s_opt.olcxlccursor);
	// modified file, can't identify the reference
//&fprintf(dumpOut,":modif flag == %d\n", s_opt.modifiedFlag);
	if (s_opt.modifiedFlag == 1) return(0);
	// here I will need also the symbol name
	// do not bypass commanline entered files, because of local symbols
	// and because references from currently processed file would
	// be not loaded from the TAG file (it expects they are loaded
	// by parsing).
//&fprintf(dumpOut,"checking if cmd %s, == %d\n", s_fileTab.tab[fnum]->name,s_fileTab.tab[fnum]->b.commandLineEntered);
	if (s_fileTab.tab[fnum]->b.commandLineEntered) return(0);
	// if references are not updated do not search it here
	// there were fullUpdate time? why?
//&fprintf(dumpOut,"checking that lastmodif %d, == %d\n", s_fileTab.tab[fnum]->lastModif, s_fileTab.tab[fnum]->lastUpdateMtime);
	if (s_fileTab.tab[fnum]->lastModif!=s_fileTab.tab[fnum]->lastUpdateMtime) return(0);
	// here read one reference file looking for the refs
	// assume s_opt.olcxlccursor is correctly set;
	getLineColCursorPositionFromCommandLineOption( &line, &col);
	FILL_position(&s_olcxByPassPos, fnum, line, col);
	olSetCallerPosition(&s_olcxByPassPos);
	readOneAppropReferenceFile(s_opt.browsedSymName, s_cxByPassFunTab);
	// if no symbol found, it may be a local symbol, try by parsing
//&fprintf(dumpOut,"checking that %d, != NULL\n", s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	if (s_olcxCurrentUser->browserStack.top->hkSelectedSym==NULL) return(0);
	// here I should set caching to 1 and recover the cachePoint ???
	// yes, because last file references are still stored, even if I 
    // update the cxref file, so do it only if switching file?
	// but how to ensure that next pass will start parsing?
	// By recovering of point 0 handled as such a special case.
	recoverCachePointZero();
//&fprintf(dumpOut,"yes, it can be identified by position\n");
    return(1);
}

static void mainEditSrvFileSingleCppPass( int argc, char **argv, 
										  int nargc, char **nargv, 
										  int *firstPassing
	) {
	int inputIn;
	int ol2procfile,line,col;
	inputIn = 0;
	s_olStringSecondProcessing = 0;
	mainFileProcessingInitialisations(firstPassing, argc, argv,
					  nargc, nargv, &inputIn, &s_language);
	smartReadFileTabFile();
	s_olOriginalFileNumber = s_input_file_number;
	if (mainSymbolCanBeIdentifiedByPosition(s_input_file_number)) {
		mainCloseInputFile(inputIn);
		return;
	}
	mainEditSrvParseInputFile( firstPassing, inputIn);
	if (s_opt.olCursorPos==0 && !LANGUAGE(LAN_JAVA)) {
		// special case, push the file as include reference
		if (creatingOlcxRefs()) {
			S_position dpos;
			FILL_position(&dpos, s_input_file_number, 1, 0);
			gotOnLineCxRefs(&dpos);
		}
		addThisFileDefineIncludeReference(s_input_file_number);
	}
	if (s_olstringFound && s_olstringServed==0) {
		// on-line action with cursor in an un-used macro body ???
		ol2procfile = scheduleFileUsingTheMacro();
		if (ol2procfile!=s_noneFileIndex) {
			s_input_file_name = s_fileTab.tab[ol2procfile]->name;
			inputIn = 0;
			s_olStringSecondProcessing=1;
			mainFileProcessingInitialisations(firstPassing, argc, argv,
					  nargc, nargv, &inputIn, &s_language);
			mainEditSrvParseInputFile( firstPassing, inputIn);
		}
	}
}


static void mainEditServerProcessFile( int argc, char **argv, 
									   int nargc, char **nargv, 
									   int *firstPassing 
	) {
	FILE 						*inputIn;
	int 						ol2procfile;
	assert(s_fileTab.tab[s_olOriginalComFileNumber]->b.scheduledToProcess);
	s_maximalCppPass = 1;
	s_currCppPass = 1;
	for(s_currCppPass=1; s_currCppPass<=s_maximalCppPass; s_currCppPass++) {
		s_input_file_name = s_fileTab.tab[s_olOriginalComFileNumber]->name;
		assert(s_input_file_name!=NULL);
		mainEditSrvFileSingleCppPass( argc, argv, nargc, nargv, firstPassing);
		if (s_opt.cxrefs==OLO_EXTRACT
			|| (s_olstringServed && ! creatingOlcxRefs())) goto fileParsed;
		if (LANGUAGE(LAN_JAVA)) goto fileParsed;
	}
	fileParsed:
	s_fileTab.tab[s_olOriginalComFileNumber]->b.scheduledToProcess = 0;
}

static void initAvailableRefactorings() {
	int i;
	for(i=0; i<MAX_AVAILABLE_REFACTORINGS; i++) {
		s_availableRefactorings[i].available=0;
		s_availableRefactorings[i].option = "";
	}
}


char * presetEditServerFileDependingStatics() {
	int 	i, fArgCount;
	char	*fileName;
	s_fileProcessStartTime = time(NULL);
	//&s_paramPosition = s_noPos;
	//&s_paramBeginPosition = s_noPos;
	//&s_paramEndPosition = s_noPos;
	s_primaryStartPosition = s_noPos;
	s_staticPrefixStartPosition = s_noPos;
	// THIS is pretty stupid, there is always only one input file
	// in edit server, otherwise it is an eror
	fArgCount = 0; s_input_file_name = getInputFile(&fArgCount);
	if (fArgCount>=s_fileTab.size) {
		// conservative message, probably macro invoked on nonsaved file
		s_olOriginalComFileNumber = s_noneFileIndex;
		return(NULL);
	}
	assert(fArgCount>=0 && fArgCount<s_fileTab.size && s_fileTab.tab[fArgCount]->b.scheduledToProcess);
	for(i=fArgCount+1; i<s_fileTab.size; i++) {
		if (s_fileTab.tab[i] != NULL) {
#ifdef CORE_DUMP
			assert(s_fileTab.tab[i]->b.scheduledToProcess == 0);
#else
			s_fileTab.tab[i]->b.scheduledToProcess = 0;
#endif
		}
	}
	s_olOriginalComFileNumber = fArgCount;
	fileName = s_input_file_name;
	mainSetLanguage(fileName,  &s_language);
	// O.K. just to be sure, there is no other input file
	return(fileName);
}

int creatingOlcxRefs() {
	return (
			s_opt.cxrefs==OLO_PUSH 
		|| 	s_opt.cxrefs==OLO_PUSH_ONLY
		|| 	s_opt.cxrefs==OLO_PUSH_AND_CALL_MACRO
		|| 	s_opt.cxrefs==OLO_GOTO_PARAM_NAME
		|| 	s_opt.cxrefs==OLO_GET_PARAM_COORDINATES
		|| 	s_opt.cxrefs==OLO_GET_AVAILABLE_REFACTORINGS
		|| 	s_opt.cxrefs==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
		|| 	s_opt.cxrefs==OLO_PUSH_NAME
		|| 	s_opt.cxrefs==OLO_PUSH_SPECIAL_NAME
		|| 	s_opt.cxrefs==OLO_PUSH_ALL_IN_METHOD
		|| 	s_opt.cxrefs==OLO_PUSH_FOR_LOCALM
		|| 	s_opt.cxrefs==OLO_TRIVIAL_PRECHECK
		|| 	s_opt.cxrefs==OLO_GET_SYMBOL_TYPE
		|| 	s_opt.cxrefs==OLO_GLOBAL_UNUSED
		|| 	s_opt.cxrefs==OLO_LOCAL_UNUSED
		|| 	s_opt.cxrefs==OLO_LIST
		||  s_opt.cxrefs==OLO_RENAME
		||  s_opt.cxrefs==OLO_ENCAPSULATE
		||  s_opt.cxrefs==OLO_ARG_MANIP
		||  s_opt.cxrefs==OLO_VIRTUAL2STATIC_PUSH
//&		||  s_opt.cxrefs==OLO_SAFETY_CHECK1
		||  s_opt.cxrefs==OLO_SAFETY_CHECK2
		||  s_opt.cxrefs==OLO_CLASS_TREE
		||  s_opt.cxrefs==OLO_SYNTAX_PASS_ONLY
		||  s_opt.cxrefs==OLO_GET_PRIMARY_START
		||  s_opt.cxrefs==OLO_USELESS_LONG_NAME
		||  s_opt.cxrefs==OLO_USELESS_LONG_NAME_IN_CLASS
		||  s_opt.cxrefs==OLO_MAYBE_THIS
		||  s_opt.cxrefs==OLO_NOT_FQT_REFS
		||  s_opt.cxrefs==OLO_NOT_FQT_REFS_IN_CLASS
	);
}

int needToProcessInputFile() {
	return(
		s_opt.cxrefs==OLO_COMPLETION 
		|| s_opt.cxrefs==OLO_SEARCH
		|| s_opt.cxrefs==OLO_EXTRACT 
		|| s_opt.cxrefs==OLO_TAG_SEARCH
		|| s_opt.cxrefs==OLO_SET_MOVE_TARGET
		|| s_opt.cxrefs==OLO_SET_MOVE_CLASS_TARGET
		|| s_opt.cxrefs==OLO_SET_MOVE_METHOD_TARGET
		|| s_opt.cxrefs==OLO_GET_CURRENT_CLASS
		|| s_opt.cxrefs==OLO_GET_CURRENT_SUPER_CLASS
		|| s_opt.cxrefs==OLO_GET_METHOD_COORD
		|| s_opt.cxrefs==OLO_GET_CLASS_COORD
		|| s_opt.cxrefs==OLO_GET_ENV_VALUE
		|| creatingOlcxRefs()
		);
}

int needToLoadOptions() {
	return(needToProcessInputFile()
		   || s_opt.cxrefs==OLO_CT_INSPECT_DEF
		   || s_opt.cxrefs==OLO_MENU_INSPECT_CLASS
		   || s_opt.cxrefs==OLO_MENU_INSPECT_DEF
		   || s_opt.cxrefs==OLO_CBROWSE
		);
}

/* *************************************************************** */
/*                          Xref regime                            */
/* *************************************************************** */
static int dummy(	char **cxFreeBase0, char **cxFreeBase, 
					S_fileItem **ffc, S_fileItem **pffc,
					int *inputIn,	int *firstPassing) {
	return(0);
}

static void mainXrefProcessInputFile( int argc, char **argv, int *_inputIn, int *_firstPassing, int *_atLeastOneProcessed ) {
	int inputIn = *_inputIn;
	int firstPassing = *_firstPassing;
	int atLeastOneProcessed = *_atLeastOneProcessed;
	s_maximalCppPass = 1;
	for(s_currCppPass=1; s_currCppPass<=s_maximalCppPass; s_currCppPass++) {
		if (! firstPassing) copyOptions(&s_opt, &s_cachedOptions);
		mainFileProcessingInitialisations(&firstPassing, 
										  argc, argv, 0, NULL, &inputIn, 
										  &s_language);
		s_olOriginalFileNumber = s_input_file_number;
		s_olOriginalComFileNumber = s_olOriginalFileNumber;
		LICENSE_CHECK();
		if (inputIn) {
			recoverFromCache();
			s_cache.activeCache = 0;	/* no caching in cxref */
			mainParseInputFile();
			charBuffClose(&cFile.lb.cb);
			inputIn = 0;
			cFile.lb.cb.ff = stdin;
			atLeastOneProcessed=1;
		} else if (LANGUAGE(LAN_JAR)) {
			jarFileParse();
			atLeastOneProcessed=1;
		} else if (LANGUAGE(LAN_CLASS)) {
			classFileParse();
			atLeastOneProcessed=1;
		} else {
			error(ERR_CANT_OPEN,s_input_file_name);
			fprintf(dumpOut,"\tmaybe forgotten -p option?\n");
		}
		// no multiple passes for java programs
		firstPassing = 0;
		cFile.lb.cb.isAtEOF = 0;
		if (LANGUAGE(LAN_JAVA)) goto fileParsed;
	}

 fileParsed:
	*_inputIn = inputIn;
	*_firstPassing = firstPassing;
	*_atLeastOneProcessed = atLeastOneProcessed;
}

static void mainXrefOneWholeFileProcessing(int argc, char **argv, 
										   S_fileItem *ff,
										   int *firstPassing, int *atLeastOneProcessed) {
	int			inputIn;
	s_input_file_name = ff->name;
	s_fileProcessStartTime = time(NULL);
	// O.K. but this is missing all header files
	ff->lastUpdateMtime = ff->lastModif;
	if (s_opt.update == UP_FULL_UPDATE || s_opt.create) {
		ff->lastFullUpdateMtime = ff->lastModif;
	}
	mainXrefProcessInputFile(argc, argv, &inputIn, 
							 firstPassing, atLeastOneProcessed);
	// now free the buffer because it tooks too much memory, 
	// but I can not free it when refactoring, nor when preloaded,
	// so be very carefull about this!!!
	if (s_ropt.refactoringRegime!=RegimeRefactory) {
		editorCloseBufferIfClosable(s_input_file_name);
		if (! s_opt.cacheIncludes) editorCloseAllBuffersIfClosable();
	}
}

static void printPrescanningMessage() {
	if (s_opt.xref2) {
		sprintf(tmpBuff, "Prescanning classes, please wait.");
		ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
	} else {
		fprintf(dumpOut, "Prescanning classes, please wait.\n");
		fflush(dumpOut);
	}			
}

static int inputFileItemLess(S_fileItem *f1, S_fileItem *f2) {
	int cc;
	char dd1[MAX_FILE_NAME_SIZE];
	char dd2[MAX_FILE_NAME_SIZE];
	// first compare directory
	strcpy(dd1, directoryName_st(f1->name));
	strcpy(dd2, directoryName_st(f2->name));
	cc = strcmp(dd1, dd2);
	if (cc<0) return(1);
	if (cc>0) return(0);
	// then full file name
	cc = strcmp(f1->name, f2->name);
	if (cc<0) return(1);
	if (cc>0) return(0);
	return(0);
}

static S_fileItem *mainCreateListOfInputFiles() {
	S_fileItem *res;
	char *nn;
	int n;
	res = NULL;
	n = 0;
	for(nn=getInputFile(&n); nn!=NULL; n++,nn=getInputFile(&n)) {
		s_fileTab.tab[n]->next = res;
		res = s_fileTab.tab[n];
	}
	LIST_MERGE_SORT(S_fileItem, res, inputFileItemLess);
	return(res);
}

void mainCallXref(int argc, char **argv) {
	static char 	*cxFreeBase0, *cxFreeBase;
	static char 	*fn;
	static int 		fc,pfc;
	static int 		inputIn;
	static int 		firstPassing,mess,atLeastOneProcessed;
	S_fileItem		*ffc, *pffc;
	int				messagePrinted = 0;
	int				numberOfInputs, inputCounter, pinputCounter;

	/* some compilers have problems with restoring regs after longjmp */
	dummy(&cxFreeBase0, &cxFreeBase, &ffc, &pffc, &inputIn, &firstPassing);
	s_currCppPass = ANY_CPP_PASS;
	CX_ALLOCC(cxFreeBase0,0,char);
	if (s_opt.taskRegime == RegimeHtmlGenerate && ! s_opt.noCxFile) {
		htmlGetDefinitionReferences();
	}
	CX_ALLOCC(cxFreeBase,0,char);
	s_cxResizingBlocked = 1;
	if (s_opt.update) scheduleModifiedFilesToUpdate();
	pfc = fc = 0; atLeastOneProcessed = 0;
	ffc = pffc = mainCreateListOfInputFiles();
	inputCounter = pinputCounter = 0;
	LIST_LEN(numberOfInputs, S_fileItem, ffc);
	for(;;) {
		s_currCppPass = ANY_CPP_PASS;
		firstPassing = 1;
		if ((mess=setjmp(cxmemOverflow))!=0) {
			mainReferencesOverflowed(cxFreeBase,mess);
			if (mess==MESS_FILE_ABORT) {
				if (pffc!=NULL) pffc=pffc->next;
				else if (ffc!=NULL) ffc=ffc->next;
			}
		} else {
#if 1 //defined(VERSION_BETA3)
			s_javaPreScanOnly = 1;
			for(; pffc!=NULL; pffc=pffc->next) {
				if (! messagePrinted) {
					printPrescanningMessage();
					messagePrinted = 1;
				}
				mainSetLanguage(pffc->name, &s_language);
				if (LANGUAGE(LAN_JAVA)) {
					mainXrefOneWholeFileProcessing(argc, argv, pffc, &firstPassing, &atLeastOneProcessed);
				}
				if (s_opt.xref2) writeRelativeProgress(10*pinputCounter/numberOfInputs);
				pinputCounter++;		
			}
#endif
			s_javaPreScanOnly = 0;
			s_fileAbortionEnabled = 1;
			for(; ffc!=NULL; ffc=ffc->next) {
				mainXrefOneWholeFileProcessing(argc, argv, ffc, &firstPassing, &atLeastOneProcessed);
				ffc->b.scheduledToProcess = 0;
				ffc->b.scheduledToUpdate = 0;
				if (s_opt.xref2) writeRelativeProgress(10+90*inputCounter/numberOfInputs);
				inputCounter++;
				CHECK_FINAL();
			}
			goto regime1fini;
		}
	}
 regime1fini:
	s_fileAbortionEnabled = 0;
	if (atLeastOneProcessed) {
		if (s_opt.taskRegime==RegimeHtmlGenerate) {
			// following is for case if an internalCheckFail, will rejump here
			atLeastOneProcessed = 0; 
			generateHtml();
			if (s_opt.noCxFile) CX_ALLOCC(cxFreeBase0,0,char);
			htmlGenGlobalReferenceLists(cxFreeBase0);
			//& if (s_opt.htmlglobalx || s_opt.htmllocalx) htmlGenEmptyRefsFile();
		}
		if (s_opt.taskRegime==RegimeXref) {
//&			idTabMap(&s_fileTab, setUpdateMtimesInFileTab);
			if (s_opt.update==0 || s_opt.update==UP_FULL_UPDATE) {
				idTabMap(&s_fileTab, setFullUpdateMtimesInFileTab);
			}
			if (s_opt.xref2) {
				sprintf(tmpBuff, "Generating '%s'",s_opt.cxrefFileName);
				ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
			} else {
				fprintf(dumpOut, "Generating '%s'\n",s_opt.cxrefFileName);
				fflush(dumpOut);
			}
			mainGenerateReferenceFile();
		}
	} else if (s_opt.cxrefs == OLO_ABOUT) {
		aboutMessage();
	} else if (! s_opt.update)  {
		sprintf(tmpBuff,"no input file");
		error(ERR_ST, tmpBuff);
	}
	if (s_opt.xref2) {
		writeRelativeProgress(100);
	}
}

//#define HTML_INTERACT 1

void mainXref(int argc, char **argv) {
	int lc;

	mainOpenOutputFile(s_opt.outputFileName);
	editorLoadAllOpenedBufferFiles();

	mainCallXref(argc, argv);
	mainCloseOutputFile();
	if (s_opt.xref2) {
		ppcGenSynchroRecord();
	}
	if (s_opt.last_message!=NULL) {
		fflush(dumpOut);
		fprintf(dumpOut,"%s\n\n", s_opt.last_message); fflush(dumpOut);
		fflush(dumpOut);
	}
//&	fprintf(dumpOut,"\n\nDUMP\n\n"); fflush(dumpOut);
//&	refTabMap(&s_cxrefTab, symbolRefItemDump);
}

/* *************************************************************** */
/*                          Edit regime                            */
/* *************************************************************** */

void mainCallEditServerInit(int nargc, char **nargv) {
	initAvailableRefactorings();
	s_opt.classpath = "";
	processOptions(nargc, nargv, INFILES_ENABLED); /* no include or define options */
	mainScheduleInputFilesFromOptionsToFileTable();
	if (s_opt.cxrefs == OLO_EXTRACT) s_cache.cpi = 2; // !!!! no cache
	olcxSetCurrentUser(s_opt.user);
	FILL_completions(&s_completions, 0, s_noPos, 0, 0, 0, 0, 0, 0);
}

void mainCallEditServer(int argc, char **argv, 
						int nargc, char **nargv, 
						int *firstPassing
	) {
	int inputIn;
	editorLoadAllOpenedBufferFiles();
	olcxSetCurrentUser(s_opt.user);
	if (creatingOlcxRefs()) olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
	if (needToProcessInputFile()) {
		if (presetEditServerFileDependingStatics() == NULL) {
			error(ERR_ST, "No input file");
		} else {
			//&resetPendingSymbolMenuData();
			mainEditServerProcessFile(argc,argv,nargc,nargv,firstPassing);
		}
	} else {
		if (presetEditServerFileDependingStatics() != NULL) {
			s_fileTab.tab[s_olOriginalComFileNumber]->b.scheduledToProcess = 0;
			// added [26.12.2002] because of loading options without input file
			s_input_file_name = NULL;
		}
#if ZERO
		if (needToLoadOptions()) {
			assert(s_input_file_name == NULL);
			mainFileProcessingInitialisations(firstPassing, argc, argv, nargc, nargv,
											  &inputIn, &s_language);
		}
#endif
	}
}

static void mainEditServer(int argc, char **argv) {
	int 	nargc;	char **nargv;
	FILE 	*inputIn;
	int 	firstPassing, fArgCount;
	pid_t 	qnxClientPid;
	s_cxResizingBlocked = 1;
	firstPassing = 1;
	copyOptions(&s_cachedOptions, &s_opt);
	for(;;) {
		s_currCppPass = ANY_CPP_PASS;
		copyOptions(&s_opt, &s_cachedOptions);
		getPipedOptions(&nargc, &nargv);
		// O.K. -o option given on command line should catch also file not found
		// message
		mainOpenOutputFile(s_opt.outputFileName);
//&dumpOptions(nargc, nargv);
/*fprintf(dumpOut,"getting request\n");fflush(dumpOut);*/
		mainCallEditServerInit(nargc, nargv);
		if (ccOut==stdout && s_opt.outputFileName!=NULL) {
			mainOpenOutputFile(s_opt.outputFileName);
		}
		mainCallEditServer(argc, argv, nargc, nargv, &firstPassing);
		if (s_opt.cxrefs == OLO_ABOUT) {
			aboutMessage();
		} else {
			LICENSE_CHECK(); 
			mainAnswerEditAction();
		}
		//& s_opt.outputFileName = NULL;  // why this was here ???
		//editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
		editorCloseAllBuffers();
		mainCloseOutputFile();
		if (s_opt.cxrefs == OLO_EXTRACT) s_cache.cpi = 2; // !!!! no cache
		if (s_opt.last_message != NULL) {
			fprintf(ccOut,"%s",s_opt.last_message); 
			fflush(ccOut);
		}
		if (s_opt.xref2) ppcGenSynchroRecord();
/*fprintf(dumpOut,"request answered\n\n");fflush(dumpOut);*/
	}
	assert(0);
}

/* *************************************************************** */
/*                       Generate regime                           */
/* *************************************************************** */

static void mainGenerate(int argc, char **argv) {
	int 	inputIn;
	int 	fArgCount, firstPassing;
	s_currCppPass = ANY_CPP_PASS;
	s_fileProcessStartTime = time(NULL);
	fArgCount = 0; s_input_file_name = getInputFile(&fArgCount);
	s_maximalCppPass = 0;
	firstPassing = 1;
	mainFileProcessingInitialisations(&firstPassing, argc, argv, 0, NULL,
				&inputIn, &s_language);
	s_olOriginalFileNumber = s_input_file_number;
	s_olOriginalComFileNumber = s_olOriginalFileNumber;
	LICENSE_CHECK();
	if (inputIn) {
		recoverFromCache();
		mainParseInputFile();
		cFile.lb.cb.isAtEOF = 0;
	}
	if (s_opt.str_fill) genProjections(20);
	symTabMap(s_symTab, generate);
}

/* *********************************************************************** */
/* **************************       MAIN      **************************** */
/* *********************************************************************** */

int main(int argc, char **argv) {
	setjmp(s_memoryResize);
	if (s_cxResizingBlocked) {
		fatalError(ERR_ST,"cx_memory resizing required, see the TROUBLES section of README file", XREF_EXIT_ERR);
	}
	s_currCppPass = ANY_CPP_PASS;
	mainTotalTaskEntryInitialisations(argc, argv);
	mainTaskEntryInitialisations(argc, argv);
//&editorTest();
	if (s_opt.refactoringRegime == RegimeRefactory) mainRefactory(argc, argv);
	if (s_opt.taskRegime == RegimeXref) mainXref(argc, argv);
	if (s_opt.taskRegime == RegimeHtmlGenerate) mainXref(argc, argv);
	if (s_opt.taskRegime == RegimeEditServer) mainEditServer(argc, argv);
	if (s_opt.taskRegime == RegimeGenerate) mainGenerate(argc, argv);
	return(0);
}

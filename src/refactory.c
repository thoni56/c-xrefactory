/*
	$Revision: 1.73 $
	$Date: 2002/09/09 19:38:35 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/

#include "protocol.h"
//

#define RRF_CHARS_TO_PRE_CHECK_AROUND 		1
#define MAX_NARGV_OPTIONS_NUM 				50

static S_editorUndo *s_refactoringStartPoint;

static int s_refactoryXrefEditSrvSubTaskFirstPassing = 1;

static char *s_refactoryEditSrvInitOptions[] = {
	"xref",
	"-xrefactory-II",
//&	"-debug",
	"-task_regime_server",
	NULL,
};

static char *s_refactoryXrefInitOptions[] = {
	"xref",
	"-xrefactory-II",
	"-briefoutput",
	NULL,
};
static char *s_refactoryUpdateOption = "-fastupdate";

static int argnum(char **margv) {
	int 	res;
	char 	**aa;
	for(res=0, aa=margv; *margv!=NULL; res++,margv++) ;
	return(res);
}

int filter0(S_reference *rr, void *dummy) {
	if (rr->usg.base < UsageMaxOLUsages) return(1);
	return(0);
}

int filter1(S_reference *rr, void *dummy) {
	if (rr->usg.base < s_refListFilters[1]) return(1);
	return(0);
}

int filter2(S_reference *rr, void *dummy) {
	if (rr->usg.base < s_refListFilters[2]) return(1);
	return(0);
}

int filterBuffer(S_reference *rr, void *buffer) {
	S_editorBuffer *buff;
	buff = (S_editorBuffer *) buffer;
	if (rr->usg.base >= UsageMaxOLUsages) return(0);
	if (rr->p.file == buff->ftnum) return(1);
	return(0);
}

static void refactoringGenGUISynchronisation() {
}

static void refactorySetNargv(char *nargv[MAX_NARGV_OPTIONS_NUM], 
							  S_editorBuffer *buf, 
							  char *project, 
							  S_editorMarker *point, 
							  S_editorMarker *mark
	) {
	static char optPoint[TMP_STRING_SIZE];
	static char optMark[TMP_STRING_SIZE];
	static char optXrefrc[MAX_FILE_NAME_SIZE];
	int i;
	i = 0;
	nargv[i] = "null";
	i++;
	if (s_ropt.xrefrc!=NULL) {
		sprintf(optXrefrc, "-xrefrc=%s", s_ropt.xrefrc);
		InternalCheck(strlen(optXrefrc)+1 < MAX_FILE_NAME_SIZE);
		nargv[i] = optXrefrc;
		i++;
	}
	if (s_ropt.eolConversion & CR_LF_EOL_CONVERSION) {
		nargv[i] = "-crlfconversion";
		i++;
	}
	if (s_ropt.eolConversion & CR_EOL_CONVERSION) {
		nargv[i] = "-crconversion";
		i++;
	}
	if (project!=NULL) {
		nargv[i] = "-p";
		i++;
		nargv[i] = project;
		i++;
	}
	assert(i < MAX_NARGV_OPTIONS_NUM);
	if (point!=NULL) {
		sprintf(optPoint, "-olcursor=%d", point->offset);
		nargv[i] = optPoint;
		i++;
	}
	assert(i < MAX_NARGV_OPTIONS_NUM);
	if (mark!=NULL) {
		sprintf(optMark, "-olmark=%d", mark->offset);
		nargv[i] = optMark;
		i++;
	}
	assert(i < MAX_NARGV_OPTIONS_NUM);
	if (buf!=NULL) {
		// if following assertion does not fail, you can delet buf parameter
		assert(buf == point->buffer);
		nargv[i] = buf->name;
		i++;
	}
#if ZERO
	assert(i < MAX_NARGV_OPTIONS_NUM);
	if (s_ropt.outputFileName!=NULL) {
		nargv[i] = "-o";
		i++;
		nargv[i] = s_ropt.outputFileName;
		i++;		
	}
#endif
	assert(i < MAX_NARGV_OPTIONS_NUM);
	if (s_ropt.user!=NULL) {
		nargv[i] = "-user";
		i++;
		nargv[i] = s_ropt.user;
		i++;
	}
	// finally mark end of options
	nargv[i] = NULL;
	i++;
	assert(i < MAX_NARGV_OPTIONS_NUM);
}

static void dumpNargv(int argc, char **argv) {
	int i;
	tmpBuff[0]=0;
	for (i=0; i<argc; i++) {
		sprintf(tmpBuff+strlen(tmpBuff), " %s", argv[i]);
	}
	ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
}

////////////////////////////////////////////////////////////////////////////////////////
// ----------------------- interface to edit server sub-task --------------------------

// be very carefull when calling this function as it is messing all static variables
// including options in s_opt, ...
// call to this function MUST be followed by a pushing action, to refresh options
static void refactoryUpdateReferences(char *project) {
	int 				i, nargc, refactoryXrefInitOptionsNum;
	S_editorBuffer 		bb;
	char 				*nargv[MAX_NARGV_OPTIONS_NUM];
	// following woud be too long to be allocated on stack
	//static S_options	savedCachedOptions;
	static S_options	savedOptions;

	if (s_refactoryUpdateOption==NULL || *s_refactoryUpdateOption==0) {
		writeRelativeProgress(100);
		return;
	}

	ppcGenRecordBegin(PPC_UPDATE_REPORT);

	editorQuasySaveModifiedBuffers();

	copyOptions(&savedOptions, &s_opt);
	//copyOptions(&savedCachedOptions, &s_cachedOptions);

	refactorySetNargv(nargv, NULL, project, NULL, NULL);
	nargc = argnum(nargv);
	refactoryXrefInitOptionsNum = argnum(s_refactoryXrefInitOptions);
	for(i=1; i<refactoryXrefInitOptionsNum; i++) {
		nargv[nargc++] = s_refactoryXrefInitOptions[i];
	}
	nargv[nargc++] = s_refactoryUpdateOption;

	s_currCppPass = ANY_CPP_PASS;
	mainTaskEntryInitialisations(nargc, nargv);
	//copyOptions(&s_cachedOptions, &s_opt);

	mainCallXref(nargc, nargv);

	copyOptions(&s_opt, &savedOptions);
	//&copyOptions(&s_cachedOptions, &savedCachedOptions);

	//&resetImportantOptionsFromRefactoringCommandLine();
//&fprintf(dumpOut,"here I am, %s, %s %s %s %s\n", s_ropt.user, s_opt.user, savedOptions.user, s_cachedOptions.user, savedCachedOptions.user);
	ppcGenRecordEnd(PPC_UPDATE_REPORT);

	// return into editSubTaskState
	mainTaskEntryInitialisations(argnum(s_refactoryEditSrvInitOptions), 
								 s_refactoryEditSrvInitOptions);
	s_refactoryXrefEditSrvSubTaskFirstPassing = 1;
	return;
}

static void refactoryEditServerParseBuffer(char *project, 
										   S_editorBuffer *buf, 
										   S_editorMarker *point, S_editorMarker *mark, 
										   char *pushOption, char *pushOption2
	) {
	char 				*nargv[MAX_NARGV_OPTIONS_NUM];
	int 				nargc;
	s_currCppPass = ANY_CPP_PASS;
	// this probably creates a memory leak in options
	// is it necessary? Try to put it in comment [2/8/2003]
	//& copyOptions(&s_cachedOptions, &s_opt);

	assert(s_opt.taskRegime == RegimeEditServer);

	refactorySetNargv(nargv, buf, project, point, mark);
	nargc = argnum(nargv);
	if (pushOption!=NULL) {
		nargv[nargc++] = pushOption;
	}
	if (pushOption2!=NULL) {
		nargv[nargc++] = pushOption2;
	}
//&dumpNargv(nargc, nargv);
	mainCallEditServerInit(nargc, nargv);
	mainCallEditServer(argnum(s_refactoryEditSrvInitOptions), 
					   s_refactoryEditSrvInitOptions, 
					   nargc, nargv, &s_refactoryXrefEditSrvSubTaskFirstPassing);
}

static void refactoryBeInteractive() {
	int 				pargc;	
	char 				**pargv;
	copyOptions(&s_cachedOptions, &s_opt);
	for(;;) {
		mainCloseOutputFile();
		ppcGenSynchroRecord();
		copyOptions(&s_opt, &s_cachedOptions);
		processOptions(argnum(s_refactoryEditSrvInitOptions), 
					   s_refactoryEditSrvInitOptions, INFILES_DISABLED);
		getPipedOptions(&pargc, &pargv);
		mainOpenOutputFile(s_ropt.outputFileName);
//&ppcGenRecord(PPC_INFORMATION, "Refactoring task answering", "\n");
		// old way how to finish dialog
		if (pargc <= 1) break;
		mainCallEditServerInit(pargc, pargv);
		if (s_opt.continueRefactoring) break;
		mainCallEditServer(argnum(s_refactoryEditSrvInitOptions), 
						   s_refactoryEditSrvInitOptions, 
						   pargc, pargv, &s_refactoryXrefEditSrvSubTaskFirstPassing);
		mainAnswerEditAction();
	}
}

// -------------------- end of interface to edit server sub-task ----------------------
////////////////////////////////////////////////////////////////////////////////////////


void refactoryDisplayResolutionDialog(char *message,int messageType,int continuation) {
	int 				pargc;	
	char 				**pargv;
	strcpy(tmpBuff, message);
	formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
	ppcGenDisplaySelectionRecord(tmpBuff, messageType, continuation);
	refactoryBeInteractive();
}

#define STANDARD_SELECT_SYMBOLS_MESSAGE "Select classes in left window. Those classes will be processed during refactoring. It is highly recommended to process whole hierarchy of related classes at once. Unselection of any class and its exclusion from refactoring may cause changes in your program behaviour. "
#define STANDARD_C_SELECT_SYMBOLS_MESSAGE "There are several symbols referred from this place. Continuing this refactoring will process selected symbols in once."
#define ERROR_SELECT_SYMBOLS_MESSAGE "If you see this screen, then probably something is going wrong. You are refactoring a virtual method when only statically linked symbol is required. It is strongly recommended to cancel the refactoring. "

static void refactoryPushReferences(S_editorBuffer *buf, S_editorMarker *point,
									char *pushOption, char *resolveMessage,
									int messageType
	) {
	/* now remake task initialisation as for edit server */
	refactoryEditServerParseBuffer( s_ropt.project, buf, point,NULL, pushOption,NULL);

	assert(s_olcxCurrentUser!=NULL && s_olcxCurrentUser->browserStack.top!=NULL);
	if (s_olcxCurrentUser->browserStack.top->hkSelectedSym==NULL) {
		error(ERR_INTERNAL, "no symbol found for refactoring push");
	}
	olCreateSelectionMenu(s_olcxCurrentUser->browserStack.top->command);
	if (resolveMessage!=NULL && olcxShowSelectionMenu()) {
		refactoryDisplayResolutionDialog( resolveMessage, messageType,CONTINUATION_ENABLED);
	}
}

static void refactorySafetyCheck(char *project, S_editorBuffer *buf, S_editorMarker *point) {
	// !!!!update references MUST be followed by a pushing action, to refresh options
	refactoryUpdateReferences(s_ropt.project);
	refactoryEditServerParseBuffer( project, buf, point,NULL, "-olcxsafetycheck2",NULL);

	assert(s_olcxCurrentUser!=NULL && s_olcxCurrentUser->browserStack.top!=NULL);
	if (s_olcxCurrentUser->browserStack.top->hkSelectedSym==NULL) {
		error(ERR_ST, "No symbol found for refactoring safety check");
	}
	olCreateSelectionMenu(s_olcxCurrentUser->browserStack.top->command);
	if (safetyCheck2ShouldWarn()) {
		if (LANGUAGE(LAN_JAVA)) {
			sprintf(tmpBuff, "This is class hierarchy of given symbol as it will appear after the refactoring. It does not corresponf to the hierarchy before the refactoring. It is probable that the refactoring will not be behaviour preserving. If you are not sure about your action, you should abandon this refactoring!");
		} else {
			sprintf(tmpBuff, "Those symbols will be refererred at this place after the refactoring. It is probable that the refactoring will not be behaviour preserving. If you are not sure about your action, you should abandon this refactoring!");
		}
		refactoryDisplayResolutionDialog(tmpBuff, PPCV_BROWSER_TYPE_WARNING,CONTINUATION_ENABLED);
	}
}

static char *refactoryGetIdentifierOnMarker_st(S_editorMarker *pos) {
	S_editorBuffer	*buff;
	char 			*s, *e, *smax, *smin;
	static char		res[TMP_STRING_SIZE];
	int				reslen;
	buff = pos->buffer;
	assert(buff && buff->a.text && pos->offset<=buff->a.bufferSize);
	s = buff->a.text + pos->offset;
	smin = buff->a.text;
	smax = buff->a.text + buff->a.bufferSize;
	// move to the beginning of identifier
	for(; s>=smin && (isalpha(*s) || isdigit(*s) || *s=='_' || *s=='$'); s--) ;
	for(s++; s<smax && isdigit(*s); s++) ;
	// now get it
	for(e=s; e<smax && (isalpha(*e) || isdigit(*e) || *e=='_' || *e=='$'); e++) ;
	reslen = e-s;
	InternalCheck(reslen < TMP_STRING_SIZE-1);
	strncpy(res, s, reslen);
	res[reslen] = 0;
	return(res);
}

static void refactoryReplaceString(S_editorMarker *pos, int len, char *newVal) {
	char 			*bVal;
	bVal = pos->buffer->a.text + pos->offset;
	editorReplaceString(pos->buffer, pos->offset, len, 
						newVal, strlen(newVal), &s_editorUndo);
}

static void refactoryCheckedReplaceString(S_editorMarker *pos, int len, 
										  char *oldVal, char *newVal) {
	char 	*bVal;
	int		check, i, d;
	bVal = pos->buffer->a.text + pos->offset;
	check = (strlen(oldVal)==len && strncmp(oldVal, bVal, len) == 0);
	if (check) {
		refactoryReplaceString(pos, len, newVal);
	} else {
		sprintf(tmpBuff, "checked replacement of %s to %s failed on ", oldVal, newVal);
		d = strlen(tmpBuff);
		for(i=0; i<len; i++) tmpBuff[d++] = bVal[i];
		tmpBuff[d++]=0;
		error(ERR_INTERNAL, tmpBuff);
	}
}

// -------------------------- Undos

static void editorFreeSingleUndo(S_editorUndo *uu) {
	if (uu->u.replace.str!=NULL && uu->u.replace.strlen!=0) {
		switch (uu->operation) {
		case UNDO_REPLACE_STRING:
			ED_FREE(uu->u.replace.str, uu->u.replace.strlen+1);
			break;
		case UNDO_RENAME_BUFFER:
			ED_FREE(uu->u.rename.name, strlen(uu->u.rename.name)+1);
			break;
		case UNDO_MOVE_BLOCK:
			break;
		default:
			error(ERR_INTERNAL,"Unknown operation to undo");
		}
	}
	ED_FREE(uu, sizeof(S_editorUndo));
}

static void editorFreeUndos(S_editorUndo *undos) {
	S_editorUndo *uu, *next;
	uu=undos; 
	while (uu!=NULL) {
		next = uu->next;
		editorFreeSingleUndo(uu);
		uu = next;		
	}
}

void editorApplyUndos(S_editorUndo *undos, S_editorUndo *until, 
					  S_editorUndo **undoundo, int gen) {
	S_editorUndo *uu, *next;
	S_editorMarker *m1, *m2;
	uu = undos;
	while (uu!=until && uu!=NULL) {
		switch (uu->operation) {
		case UNDO_REPLACE_STRING:
			if (gen == GEN_FULL_OUTPUT) {
				ppcGenReplaceRecord(uu->buffer->name, uu->u.replace.offset,
									uu->buffer->a.text+uu->u.replace.offset, 
									uu->u.replace.size, uu->u.replace.str);
			}
			editorReplaceString(uu->buffer, uu->u.replace.offset, uu->u.replace.size, 
								uu->u.replace.str, uu->u.replace.strlen, undoundo);
																	
			break;
		case UNDO_RENAME_BUFFER:
			if (gen == GEN_FULL_OUTPUT) {
				ppcGenGotoOffsetPosition(uu->buffer->name, 0);
				ppcGenRecord(PPC_MOVE_FILE_AS, uu->u.rename.name, "\n");
			}
			editorRenameBuffer(uu->buffer, uu->u.rename.name, undoundo);
			break;
		case UNDO_MOVE_BLOCK:
			m1 = editorCrNewMarker(uu->buffer, uu->u.moveBlock.offset);
			m2 = editorCrNewMarker(uu->u.moveBlock.dbuffer, uu->u.moveBlock.doffset);
			if (gen == GEN_FULL_OUTPUT) {
				ppcGenGotoMarkerRecord(m1);
				ppcGenNumericRecord(PPC_REFACTORING_CUT_BLOCK,uu->u.moveBlock.size,"","\n");
			}
			editorMoveBlock(m2, m1, uu->u.moveBlock.size, undoundo);
			if (gen == GEN_FULL_OUTPUT) {
				ppcGenGotoMarkerRecord(m1);
				ppcGenRecord(PPC_REFACTORING_PASTE_BLOCK, "", "\n");
			}
			editorFreeMarker(m2);
			editorFreeMarker(m1);
			break;
		default:
			error(ERR_INTERNAL,"Unknown operation to undo");
		}
		next = uu->next;
		editorFreeSingleUndo(uu);
		uu = next;
	}
	assert(uu==until);
}

void editorUndoUntil(S_editorUndo *until, S_editorUndo **undoundo) {
	editorApplyUndos(s_editorUndo, until, undoundo, GEN_NO_OUTPUT);
	s_editorUndo = until;
}

static void refactoryAplyWholeRefactoringFromUndo() {
	S_editorUndo 		*redoTrack;
	redoTrack = NULL;
	editorUndoUntil(s_refactoringStartPoint, &redoTrack);
	editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);
}

static void refactoryFatalErrorOnPosition(S_editorMarker *p, int errType, char *message) {
	S_editorUndo *redo;
	redo = NULL;
	editorUndoUntil(s_refactoringStartPoint, &redo);
	ppcGenGotoMarkerRecord(p);
	fatalError(errType, message, XREF_EXIT_ERR);
	// unreachable, but do the things properly
	editorApplyUndos(redo, NULL, &s_editorUndo, GEN_NO_OUTPUT);
}

// -------------------------- end of Undos

static void refactoryRemoveNonCommentCode(S_editorMarker *m, int len) {
	int 			c, nn, n;
	char 			*s;
	S_editorMarker 	*mm;
	assert(m->buffer && m->buffer->a.text);
	s = m->buffer->a.text + m->offset;
	nn = len;
	mm = editorCrNewMarker(m->buffer, m->offset);
	if (m->offset + nn > m->buffer->a.bufferSize) {
		nn = m->buffer->a.bufferSize - m->offset;
	}
	n = 0;
	while(nn>0) {
		c = *s;
		if (c=='/' && nn>1 && *(s+1)=='*' && (nn<=2 || *(s+2)!='&')) {
			// /**/ comment
			refactoryReplaceString(mm, n, "");
			s = mm->buffer->a.text + mm->offset;
			s += 2; nn -= 2;
			while (! (*s=='*' && *(s+1)=='/')) {s++; nn--;}
			s += 2; nn -= 2;
			mm->offset = s - mm->buffer->a.text;
			n = 0;
		} else if (c=='/' && nn>1 && *(s+1)=='/' && (nn<=2 || *(s+2)!='&')) {
			// // comment
			refactoryReplaceString(mm, n, "");
			s = mm->buffer->a.text + mm->offset;
			s += 2; nn -= 2;
			while (*s!='\n') {s++; nn--;}
			s += 1; nn -= 1;
			mm->offset = s - mm->buffer->a.text;
			n = 0;
		} else if (c=='"') {
			// string, pass it removing all inside (also /**/ comments)
			s++; nn--; n++;
			while (*s!='"' && nn>0) {
				s++; nn--; n++;
				if (*s=='\\') {
					s++; nn--; n++;
					s++; nn--; n++;
				}
			}
		} else {
			s++; nn--; n++;
		}
	}
	if (n>0) {
		refactoryReplaceString(mm, n, "");
	}
	editorFreeMarker(mm);
}

// basically move marker to the first non blank and non comment symbol at the same
// line as the marker is or to the newline character
static void refactoryMoveMarkerToTheEndOfDefinitionScope(S_editorMarker *mm) {
	int offset;
	offset = mm->offset;
	editorMoveMarkerToNonBlankOrNewline(mm, 1);
	if (mm->offset >= mm->buffer->a.bufferSize) {
		return;
	}
	if (CHAR_ON_MARKER(mm)=='/' && CHAR_AFTER_MARKER(mm)=='/') {
		if (s_ropt.commentMovingLevel == CM_NO_COMMENT) return;
		editorMoveMarkerToNewline(mm, 1);
		mm->offset ++;
	} else if (CHAR_ON_MARKER(mm)=='/' && CHAR_AFTER_MARKER(mm)=='*') {
		if (s_ropt.commentMovingLevel == CM_NO_COMMENT) return;
		mm->offset ++; mm->offset ++;
		while (mm->offset<mm->buffer->a.bufferSize && 
			   (CHAR_ON_MARKER(mm)!='*' || CHAR_AFTER_MARKER(mm)!='/')) {
			mm->offset++;
		}
		if (mm->offset<mm->buffer->a.bufferSize) {
			mm->offset ++;
			mm->offset ++;
		}
		offset = mm->offset;
		editorMoveMarkerToNonBlankOrNewline(mm, 1);
		if (CHAR_ON_MARKER(mm)=='\n') mm->offset ++;
		else mm->offset = offset;
	} else if (CHAR_ON_MARKER(mm)=='\n') {
		mm->offset ++;
	} else {
		if (s_ropt.commentMovingLevel == CM_NO_COMMENT) return;
		mm->offset = offset;
	}
}

static int refactoryMarkerWRTCommentary(S_editorMarker *mm, int *comBeginOffset) {
	char 			*b, *s, *e, *mms;
	assert(mm->buffer && mm->buffer->a.text);
	s = mm->buffer->a.text;
	e = s + mm->buffer->a.bufferSize;
	mms = s + mm->offset;
	while(s<e && s<mms) {
		b = s;
		if (*s=='/' && (s+1)<e && *(s+1)=='*') {
			// /**/ comment
			s += 2;
			while ((s+1)<e && ! (*s=='*' && *(s+1)=='/')) s++;
			if (s+1<e) s += 2;
			if (s>mms) {
				*comBeginOffset = b-mm->buffer->a.text;
				return(MARKER_IS_IN_STAR_COMMENT);
			}
		} else if (*s=='/' && s+1<e && *(s+1)=='/') {
			// // comment
			s += 2;
			while (s<e && *s!='\n') s++;
			if (s<e) s += 1;
			if (s>mms) {
				*comBeginOffset = b-mm->buffer->a.text;
				return(MARKER_IS_IN_SLASH_COMMENT);
			}
		} else if (*s=='"') {
			// string, pass it removing all inside (also /**/ comments)
			s++;
			while (s<e && *s!='"') {
				s++;
				if (*s=='\\') { s++; s++; }
			}
			if (s<e) s++;
		} else {
			s++;
		}
	}
	return(MARKER_IS_IN_CODE);
}

static void refactoryMoveMarkerToTheBeginOfDefinitionScope(S_editorMarker *mm) {
	int 			theBeginningOffset, comBeginOffset, mp;
	int				slashedCommentsProcessed, staredCommentsProcessed;

	slashedCommentsProcessed = staredCommentsProcessed = 0;
	for(;;) {
		theBeginningOffset = mm->offset;
		mm->offset --;
		editorMoveMarkerToNonBlankOrNewline(mm, -1);
		if (CHAR_ON_MARKER(mm)=='\n') {
			theBeginningOffset = mm->offset+1;
			mm->offset --;
		}
		if (s_ropt.commentMovingLevel == CM_NO_COMMENT) goto fini;
		editorMoveMarkerToNonBlank(mm, -1);
		mp = refactoryMarkerWRTCommentary(mm, &comBeginOffset);
		if (mp == MARKER_IS_IN_CODE) goto fini;
		else if (mp == MARKER_IS_IN_STAR_COMMENT) {
			if (s_ropt.commentMovingLevel == CM_SINGLE_SLASHED) goto fini;
			if (s_ropt.commentMovingLevel == CM_ALL_SLASHED) goto fini;
			if (staredCommentsProcessed>0 && s_ropt.commentMovingLevel==CM_SINGLE_STARED) goto fini;
			if (staredCommentsProcessed>0 && s_ropt.commentMovingLevel==CM_SINGLE_SLASHED_AND_STARED) goto fini;
			staredCommentsProcessed ++;
			mm->offset = comBeginOffset;
		}
		// slash comment, skip them all
		else if (mp == MARKER_IS_IN_SLASH_COMMENT) {
			if (s_ropt.commentMovingLevel == CM_SINGLE_STARED) goto fini;
			if (s_ropt.commentMovingLevel == CM_ALL_STARED) goto fini;
			if (slashedCommentsProcessed>0 && s_ropt.commentMovingLevel==CM_SINGLE_SLASHED) goto fini;
			if (slashedCommentsProcessed>0 && s_ropt.commentMovingLevel==CM_SINGLE_SLASHED_AND_STARED) goto fini;
			slashedCommentsProcessed ++;
			mm->offset = comBeginOffset;
		} else {
			warning(ERR_INTERNAL, "A new commentary?");
			goto fini;
		}
	}
 fini:
	mm->offset = theBeginningOffset;
}

static void refactoryRenameTo(S_editorMarker *pos, char *oldName, char *newName) {
	char 	*actName;
	int		nlen;
	nlen = strlen(oldName);
	actName = refactoryGetIdentifierOnMarker_st(pos);
	InternalCheck(strcmp(actName, oldName)==0);
	refactoryCheckedReplaceString(pos, nlen, oldName, newName);
}

static S_editorMarker *refactoryPointMark(S_editorBuffer *buf, int offset) {
	S_editorMarker *point;
	point = NULL;
	if (offset >= 0) {
		point = editorCrNewMarker(buf, offset);
	}
	return(point);
}
static S_editorMarker *refactoryGetPointFromRefactoryOptions(S_editorBuffer *buf) {
	assert(buf);
	return(refactoryPointMark(buf, s_ropt.olCursorPos));
}
static S_editorMarker *refactoryGetMarkFromRefactoryOptions(S_editorBuffer *buf) {
	assert(buf);
	return(refactoryPointMark(buf, s_ropt.olMarkPos));
}

static void refactoryPushMarkersAsReferences(S_editorMarkerList **markers,
											 S_olcxReferences *refs, char *sym) {
	S_olSymbolsMenu 	*mm;
	S_reference			*r, *rr;
	rr = editorMarkersToReferences(markers);
	for(mm=refs->menuSym; mm!=NULL; mm=mm->next) {
		if (strcmp(mm->s.name, sym)==0) {
			for(r=rr; r!=NULL; r=r->next) {
				//&sprintf(tmpBuff,"adding mis ref on %d:%d", r->p.line, r->p.coll);
				//&ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
				olcxAddReference(&mm->s.refs, r, 0);
			}
		}
	}
	olcxFreeReferences(rr);
	olcxRecomputeSelRefs(refs);
}

static int validTargetPlace(S_editorMarker *target, char *checkOpt) {
	int res;
	res = 1;
	refactoryEditServerParseBuffer(s_ropt.project, target->buffer, target,NULL, checkOpt, NULL);
	if (!s_cps.moveTargetApproved) {
		res = 0;
		error(ERR_ST, "Invalid target place");
	}
	return(res);
}

// ------------------------- Trivial prechecks --------------------------------------

static S_olSymbolsMenu *javaGetRelevantHkSelectedItem(S_symbolRefItem *ri) {
	S_olSymbolsMenu 	*ss;
	S_olcxReferences 	*rstack;
	rstack = s_olcxCurrentUser->browserStack.top;
	for(ss=rstack->hkSelectedSym; ss!=NULL; ss=ss->next) {
		if (itIsSameCxSymbol(ri, &ss->s)
			&& ri->vFunClass == ss->s.vFunClass) {
			break;
		}
	}
	return(ss);
}

static void tpCheckFutureAccOfLocalReferences(S_symbolRefItem *ri, void *ddd) {
	S_reference				*rr;
	S_olcxReferences  		*rstack;
	S_tpCheckMoveClassData 	*dd;
	char					symclass[MAX_FILE_NAME_SIZE];
	int						sclen, symclen;
	S_position 				*defpos;
    int 					defusage;
	S_olSymbolsMenu 		*ss;
	dd = (S_tpCheckMoveClassData *) ddd;
//&fprintf(ccOut,"!mapping %s\n", ri->name);
	MOVE_CLASS_MAP_FUN_RETURN_ON_UNINTERESTING_SYMBOLS(ri, dd);
	ss = javaGetRelevantHkSelectedItem(ri);
	if (ss!=NULL) {
		// relevant symbol
		for(rr=ri->refs; rr!=NULL; rr=rr->next) {
			// I should check only references from this file
			if (rr->p.file == s_input_file_number) {
				// check if the reference is outside moved class
				if ((! DM_IS_BETWEEN(cxMemory, rr, dd->mm.minMemi, dd->mm.maxMemi))
					&& OL_VIEWABLE_REFS(rr)) {
					// yes there is a reference from outside to our symbol
					ss->selected = ss->visible = 1;
					break;
				}
			}
		}
	}
}

static void tpCheckMoveClassPutClassDefaultSymbols(S_symbolRefItem *ri, void *ddd) {
	S_reference				*rr;
	S_olcxReferences  		*rstack;
	S_tpCheckMoveClassData 	*dd;
	char					symclass[MAX_FILE_NAME_SIZE];
	int						sclen, symclen;
	S_position 				*defpos;
    int 					defusage;
	dd = (S_tpCheckMoveClassData *) ddd;
//&fprintf(ccOut,"!mapping %s\n", ri->name);
	MOVE_CLASS_MAP_FUN_RETURN_ON_UNINTERESTING_SYMBOLS(ri, dd);
	// fine, add it to Menu, so we will load its references 
	for(rr=ri->refs; rr!=NULL; rr=rr->next) {
//&fprintf(dumpOut,"checking %d.%d ref of %s\n", rr->p.line,rr->p.coll,ri->name); fflush(dumpOut);
		if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, (&dd->mm))) {
			if (IS_DEFINITION_OR_DECL_USAGE(rr->usg.base)) {
				// definition inside class, default or private acces to be checked
				rstack = s_olcxCurrentUser->browserStack.top;
				olAddBrowsedSymbol(ri, &rstack->hkSelectedSym, 1, 1, 
								   0, UsageUsed, 0, &rr->p, rr->usg.base);
				break;
			}
		}
	}
}

static void tpCheckFutureAccessibilitiesOfSymbolsDefinedInsideMovedClass(
	S_tpCheckMoveClassData dd
	) {
	S_olcxReferences *rstack;
	S_olSymbolsMenu *ss, **sss, *mm;
	S_reference *rr;
	int foundOutsideReference;
	rstack = s_olcxCurrentUser->browserStack.top;
	rstack->hkSelectedSym = olCreateSpecialMenuItem(
		LINK_NAME_MOVE_CLASS_MISSED, s_noneFileIndex, StorageDefault);
	// push them into hkSelection, 
	refTabMap2(&s_cxrefTab, tpCheckMoveClassPutClassDefaultSymbols, &dd);
	// load all theirs references
	olCreateSelectionMenu(rstack->command);
	// mark all as unselected unvisible
	for(ss=rstack->hkSelectedSym; ss!=NULL; ss=ss->next) {
		ss->selected = ss->visible = 0;
	}
	// checks all references from within this file
	refTabMap2(&s_cxrefTab, tpCheckFutureAccOfLocalReferences, &dd);
	// check references from outside
	for(mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
		ss = javaGetRelevantHkSelectedItem(&mm->s);
		if (ss!=NULL && (! ss->selected)) {
			for(rr=mm->s.refs; rr!=NULL; rr=rr->next) {
				if (rr->p.file != s_input_file_number) {
					// yes there is a reference from outside to our symbol
					ss->selected = ss->visible = 1;
					goto nextsym;
				}
			}
		}
	nextsym:;
	}

	sss= &rstack->menuSym; 
	while (*sss!=NULL) {
		ss = javaGetRelevantHkSelectedItem(&(*sss)->s);
		if (ss!=NULL && ss->selected) {
			sss= &(*sss)->next;
		} else {
			*sss = olcxFreeSymbolMenuItem(*sss);
		}
	}
}

static void tpCheckDefaultAccessibilitiesMoveClass(S_symbolRefItem *ri, void *ddd) {
	S_reference				*rr;
	S_olcxReferences  		*rstack;
	S_tpCheckMoveClassData 	*dd;
	char					symclass[MAX_FILE_NAME_SIZE];
	int						sclen, symclen;
	dd = (S_tpCheckMoveClassData *) ddd;
//&fprintf(ccOut,"!mapping %s\n", ri->name);
	MOVE_CLASS_MAP_FUN_RETURN_ON_UNINTERESTING_SYMBOLS(ri, dd);
	// check that it is not from moved class
	javaGetClassNameFromFileNum(ri->vFunClass, symclass, 0);
	sclen = strlen(dd->sclass);
	symclen = strlen(symclass);
	if (sclen<=symclen && fnnCmp(dd->sclass, symclass, sclen)==0) return;
	// O.K. finally check if there is a reference
	for(rr=ri->refs; rr!=NULL; rr=rr->next) {
		if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, (&dd->mm))) {
			// O.K. there is a reference inside the moved class, add it to the list,
			assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
			rstack = s_olcxCurrentUser->browserStack.top;
			olcxAddReference(&rstack->r, rr, 0);
			break;
		}
	}
}

void tpCheckFillMoveClassData(S_tpCheckMoveClassData *dd, 
							  char *spack, char *tpack
	) {
	S_olcxReferences *rstack;
	S_olSymbolsMenu *sclass;
	char *targetfile, *srcfile;
	int transPackageMove;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	sclass = rstack->hkSelectedSym;
	assert(sclass);
	targetfile = s_opt.moveTargetFile;
	assert(targetfile);
	srcfile = s_input_file_name;
	assert(srcfile);
	// 
	javaGetPackageNameFromSourceFileName(srcfile, spack);
	javaGetPackageNameFromSourceFileName(targetfile, tpack);
	// O.K. moving from package spack to package tpack
	if (fnCmp(spack, tpack)==0) transPackageMove = 0;
	else transPackageMove = 1;

	FILLF_tpCheckMoveClassData(dd, s_cps.cxMemiAtClassBeginning, 
							   s_cps.cxMemiAtClassEnd, 
							   spack, tpack, transPackageMove, sclass->s.name);

}

int tpCheckMoveClassAccessibilities() {
	S_olcxReferences  		*rstack;
	S_olSymbolsMenu			*sclass, *ss, **sss;
	S_reference				*rr;
	char					*targetfile, *srcfile;
	S_tpCheckMoveClassData	dd;
	char					spack[MAX_FILE_NAME_SIZE];
	char					tpack[MAX_FILE_NAME_SIZE];
	int						transPackageMove;

	tpCheckFillMoveClassData(&dd, spack, tpack);
	olcxPushSpecialCheckMenuSym(s_opt.cxrefs,LINK_NAME_MOVE_CLASS_MISSED);
	refTabMap2(&s_cxrefTab, tpCheckDefaultAccessibilitiesMoveClass, &dd);

	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	if (rstack->r!=NULL) {
		ss = rstack->menuSym;
		assert(ss);
		ss->s.refs = olcxCopyRefList(rstack->r);
		rstack->act = rstack->r;
		if (s_ropt.refactoringRegime==RegimeRefactory) {
			int firstPassing = 1;
			refactoryDisplayResolutionDialog(
				"Those references inside moved class are refering to symbols which will be  inaccessible  at  new  class  location. You  should  adjust  their accessibilities first.  (Each symbol is listed only once)",
				PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
			refactoryAskForReallyContinueConfirmation();
#if ZERO
		} else {
			olcxPrintRefList(";", rstack);
			fprintf(ccOut, 
					" ** Those references inside moved class are refering to symbols which will  ** be  inaccessible  at  new  class  location. You  should  adjust  their  ** accessibilities first.  (Each symbol is listed only once)"
				);
#endif
		}
		return(0);
	} 
	olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
	// O.K. now check symbols defined inside the class
	olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
	tpCheckFutureAccessibilitiesOfSymbolsDefinedInsideMovedClass(dd);
	rstack = s_olcxCurrentUser->browserStack.top;
	if (rstack->menuSym!=NULL) {
		olcxRecomputeSelRefs(rstack);
		// TODO, synchronize this with emacs, but how?
		rstack->refsFilterLevel = RFilterDefinitions;
		if (s_ropt.refactoringRegime==RegimeRefactory) {
			int firstPassing = 1;
			refactoryDisplayResolutionDialog(
				"Those symbols  defined inside moved  class and used outside  the class will be inaccessible  at new class location.  You  should adjust their accessibilities first.",
				PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
			refactoryAskForReallyContinueConfirmation();
#if ZERO
		} else {
			olcxPrintRefList(";", rstack);
			fprintf(ccOut, 
					" ** Those symbols defined inside moved class and used outside the class will  ** be  inaccessible  at  new  class  location.  You  should  adjust  their  ** accessibilities first."
				);
#endif
		}
		return(0);
	}
	olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
	return(1);
}

int tpCheckSourceIsNotInnerClass() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	char				*target;
	int 				thisclassi,deii;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	// I can rely that it is a class
	thisclassi = getClassNumFromClassLinkName(ss->s.name, s_noneFileIndex);
	//& target = s_opt.moveTargetClass;
	//& assert(target!=NULL);
	assert(s_fileTab.tab[thisclassi]);
	deii = s_fileTab.tab[thisclassi]->directEnclosingInstance;
	if (deii != -1 && deii != s_noneFileIndex && (ss->s.b.accessFlags&ACC_INTERFACE)==0) {
		// exists direct enclosing instance, it is an inner class
		sprintf(tmpBuff, "This is  an inner class. Current  version of C-xrefactory  can only move nested classes declared  'static' and top level classes.  If the class does  not depend  on its  enclosing instances,  you should  declare it 'static' and move it after.");
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

static void tpCheckSpecialReferencesMapFun(S_symbolRefItem *ri,
														void *ddd
	) {
	S_reference 						*rr;
	S_tpCheckSpecialReferencesData 	*dd;
	S_chReference 						*cl;
	int									scl;
	dd = (S_tpCheckSpecialReferencesData *) ddd;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	// todo make supermethod symbol special type
//&fprintf(dumpOut,"! checking %s\n", ri->name);
	if (strcmp(ri->name, dd->symbolToTest)!=0) return;
//&fprintf(dumpOut,"comparing %d <-> %d; %s <-> %s\n", ri->vFunClass, scl,	s_fileTab.tab[ri->vFunClass]->name, s_fileTab.tab[scl]->name);
	for(rr=ri->refs; rr!=NULL; rr=rr->next) {
		if (DM_IS_BETWEEN(cxMemory, rr, dd->mm.minMemi, dd->mm.maxMemi) ){
			// a super method reference
			dd->foundSpecialRefItem = ri;
			dd->foundSpecialR = rr;
			if (ri->vFunClass == dd->classToTest) {
				// a super reference to direct superclass
				dd->foundRefToTestedClass = ri;
			} else {
				dd->foundRefNotToTestedClass = ri;
			}
			// TODO! check if it is reference to outer scope
			if (rr->usg.base == UsageMaybeQualifiedThis) {
				dd->foundOuterScopeRef = rr;
			}
		}				
	}
}

static int tpCheckSuperMethodReferencesInit(S_tpCheckSpecialReferencesData *rr) {
	S_olSymbolsMenu 					*ss;
	S_chReference 						*cl;
	int									scl;
	S_olcxReferences					*rstack;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	scl = javaGetSuperClassNumFromClassNum(ss->s.vApplClass);
	if (scl == s_noneFileIndex) {
		error(ERR_ST, "no super class, something is going wrong");
		return(0);
	}
	FILLF_tpCheckSpecialReferencesData(rr, s_cps.cxMemiAtMethodBeginning, 
										s_cps.cxMemiAtMethodEnd, 
										LINK_NAME_SUPER_METHOD_ITEM, scl,
										NULL, NULL, NULL, NULL, NULL);
	refTabMap2(&s_cxrefTab, tpCheckSpecialReferencesMapFun, rr);
	return(1);
}

int tpCheckSuperMethodReferencesForPullUp() {
	S_tpCheckSpecialReferencesData 		rr;
	S_olcxReferences					*rstack;
	S_olSymbolsMenu 					*ss;
	int									tmp;
	char								tt[TMP_STRING_SIZE];
	char								ttt[MAX_CX_SYMBOL_SIZE];
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	tmp = tpCheckSuperMethodReferencesInit(&rr);
	if (! tmp) return(0);
	// synthetize an answer
	if (rr.foundRefToTestedClass!=NULL) {
		linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
		javaGetClassNameFromFileNum(rr.foundRefToTestedClass->vFunClass, tt, DOTIFY_NAME);
		sprintf(tmpBuff,"'%s' invokes another method using the keyword \"super\" and this invocation is refering to class '%s', i.e. to the class where '%s' will be moved. In consequence, it is not possible to ensure behaviour preseving pulling-up of this method.", ttt, tt, ttt);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

int tpCheckSuperMethodReferencesAfterPushDown() {
	S_tpCheckSpecialReferencesData 		rr;
	S_olcxReferences					*rstack;
	S_olSymbolsMenu 					*ss;
	int									tmp;
	char								tt[TMP_STRING_SIZE];
	char								ttt[MAX_CX_SYMBOL_SIZE];
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	tmp = tpCheckSuperMethodReferencesInit(&rr);
	if (! tmp) return(0);
	// synthetize an answer
	if (rr.foundRefToTestedClass!=NULL) {
		linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
		sprintf(tmpBuff,"'%s' invokes another method using the keyword \"super\" and the invoked method is also defined in current class. After pushing down, the reference will be misrelated. In consequence, it is not possible to ensure behaviour preseving pushing-down of this method.", ttt);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		fprintf(ccOut,":[warning] %s", tmpBuff);
		//&error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

int tpCheckSuperMethodReferencesForDynToSt() {
	S_tpCheckSpecialReferencesData 		rr;
	int									tmp;
	tmp = tpCheckSuperMethodReferencesInit(&rr);
	if (! tmp) return(0);
	// synthetize an answer
	if (rr.foundSpecialRefItem!=NULL) {
		if (s_opt.xref2) ppcGenGotoPositionRecord(&rr.foundSpecialR->p);
		sprintf(tmpBuff,"This method invokes another method using the keyword \"super\". Current version of C-xrefactory does not know how to make it static.");
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

int tpCheckOuterScopeUsagesForDynToSt() {
	S_tpCheckSpecialReferencesData 	rr;
	S_olSymbolsMenu 					*ss;
	S_olcxReferences					*rstack;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	FILLF_tpCheckSpecialReferencesData(&rr, s_cps.cxMemiAtMethodBeginning, 
									   s_cps.cxMemiAtMethodEnd, 
									   LINK_NAME_MAYBE_THIS_ITEM, ss->s.vApplClass,
									   NULL, NULL, NULL, NULL, NULL);
	refTabMap2(&s_cxrefTab, tpCheckSpecialReferencesMapFun, &rr);
	if (rr.foundOuterScopeRef!=NULL) {
		sprintf(tmpBuff,"Inner class method is using symbols from outer scope. Current version of C-xrefactory does not know how to make it static.");
		// be soft, so that user can try it and see.
		if (s_opt.xref2) {
			ppcGenGotoPositionRecord(&rr.foundOuterScopeRef->p);
			formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
			ppcGenRecord(PPC_ERROR, tmpBuff, "\n");
		} else {
			fprintf(ccOut, ":[warning] %s", tmpBuff);
		}
		//& error(ERR_ST, tmpBuff);
		return(0);
	}
	return(1);
}

int tpCheckMethodReferencesWithApplOnSuperClassForPullUp() {
	// is there an application to original class or some of super types?
	// you should not consider any call from within the method, 
	// when the method is invoked as single name, am I right?
	// No. Do not consider only invocations of form super.method().
	S_olcxReferences  	*rstack;
	S_reference			*rr;
	S_olSymbolsMenu		*ss,*mm;
	S_symbol 			*target;
	int					targetcn, srccn;
	char				ttt[MAX_CX_SYMBOL_SIZE];
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	srccn = ss->s.vApplClass;
	target = getMoveTargetClass();
	assert(target!=NULL && target->u.s);
	targetcn = target->u.s->classFile;
	for (mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
		if (itIsSameCxSymbol(&ss->s, &mm->s)) {
			if (javaIsSuperClass(mm->s.vApplClass, srccn)) {
				// finally check there is some other reference than super.method()
				// and definition
				for (rr=mm->s.refs; rr!=NULL; rr=rr->next) {
					if ((! IS_DEFINITION_OR_DECL_USAGE(rr->usg.base))
						&& rr->usg.base != UsageMethodInvokedViaSuper) {
						// well there is, issue warning message and finish
						linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
						sprintf(tmpBuff,"%s is defined also in superclass and there are invocations syntactically refering to one of superclasses. Under some circumstances this may cause that pulling up of this method will not be behaviour preserving.", ttt);
						formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
						warning(ERR_ST, tmpBuff);
						return(0);
					}
				}
			}
		}
	}
	return(1);
}

int tpCheckTargetToBeDirectSubOrSupClass(int flag, char *subOrSuper) {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss;
	char				ttt[TMP_STRING_SIZE];
	char				tt[TMP_STRING_SIZE];
	char				*s;
	S_chReference		*cl;
	S_symbol 			*target;
	int 				res;
	res = 0;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	target = getMoveTargetClass();
	if (target==NULL) {
		error(ERR_ST, "moving to NULL target class?");
		return(0);
	}
	readOneAppropReferenceFile(NULL, s_cxScanFunTabForClassHierarchy);
	InternalCheck(target->u.s!=NULL&&target->u.s->classFile!=s_noneFileIndex);
	if (flag == REQ_SUBCLASS) cl=s_fileTab.tab[ss->s.vApplClass]->infs;
	else cl=s_fileTab.tab[ss->s.vApplClass]->sups;
	for(; cl!=NULL; cl=cl->next) {
//&sprintf(tmpBuff,"!checking %d(%s) <-> %d(%s) \n", cl->clas, s_fileTab.tab[cl->clas]->name, target->u.s->classFile, s_fileTab.tab[target->u.s->classFile]->name);ppcGenTmpBuff();
		if (cl->clas == target->u.s->classFile) {
			res = 1;
			break;
		}
	}
	if (res) return(res);
	javaGetClassNameFromFileNum(target->u.s->classFile, tt, DOTIFY_NAME);
	javaGetClassNameFromFileNum(ss->s.vApplClass, ttt, DOTIFY_NAME);
	sprintf(tmpBuff,"Class %s is not direct %s of %s. This refactoring provides moving to direct %ses only.", tt, subOrSuper, ttt, subOrSuper);
	formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
	error(ERR_ST,tmpBuff);
	return(0);
}

int tpPullUpFieldLastPreconditions() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss,*mm;
	char				ttt[TMP_STRING_SIZE];
	char				*s;
	S_chReference		*cl;
	S_symbol 			*target;
	int					pcharFlag;
	pcharFlag = 0;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	target = getMoveTargetClass();
	assert(target!=NULL);
	InternalCheck(target->u.s!=NULL&&target->u.s->classFile!=s_noneFileIndex);
	for(mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
		if (itIsSameCxSymbol(&ss->s,&mm->s) 
			&& mm->s.vApplClass == target->u.s->classFile) goto cont2;
	}
	// it is O.K. no item found
	return(1);
 cont2:
	// an item found, it must be empty
	if (mm->s.refs == NULL) return(1);
	javaGetClassNameFromFileNum(target->u.s->classFile, ttt, DOTIFY_NAME);
	if (IS_DEFINITION_OR_DECL_USAGE(mm->s.refs->usg.base) && mm->s.refs->next==NULL) {
		if (pcharFlag==0) {pcharFlag=1; fprintf(ccOut,":[warning] ");}
		sprintf(tmpBuff, "%s is yet defined in the superclass %s.  Pulling up will do nothing, but removing the definition from the subclass. You should be sure that both fields are initialized to the same value.", mm->s.name, ttt);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		warning(ERR_ST, tmpBuff);
		return(0);
	}
	sprintf(tmpBuff,"There are yet references of the field %s syntactically applied on the superclass %s, pulling up this field would cause confusion!", mm->s.name, ttt);
	formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
	error(ERR_ST, tmpBuff);
	return(0);
}

int tpPushDownFieldLastPreconditions() {
	S_olcxReferences  	*rstack;
	S_olSymbolsMenu		*ss,*mm, *sourcesm, *targetsm;
	char				ttt[TMP_STRING_SIZE];
	char				*s;
	S_reference			*rr;
	S_symbol 			*target;
	int					thisclassi, pcharFlag, res;
	pcharFlag = 0; res = 1;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	rstack = s_olcxCurrentUser->browserStack.top;
	ss = rstack->hkSelectedSym;
	assert(ss);
	thisclassi = ss->s.vApplClass;
	target = getMoveTargetClass();
	assert(target!=NULL);
	InternalCheck(target->u.s!=NULL&&target->u.s->classFile!=s_noneFileIndex);
	sourcesm = targetsm = NULL;
	for(mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
		if (itIsSameCxSymbol(&ss->s,&mm->s)) {
			if (mm->s.vApplClass == target->u.s->classFile) targetsm = mm;
			if (mm->s.vApplClass == thisclassi) sourcesm = mm;
		}
	}
	if (targetsm != NULL) {
		rr = getDefinitionRef(targetsm->s.refs);
		if (rr!=NULL && IS_DEFINITION_OR_DECL_USAGE(rr->usg.base)) {
			javaGetClassNameFromFileNum(target->u.s->classFile, ttt, DOTIFY_NAME);
			sprintf(tmpBuff,"The field %s is yet defined in %s!",
					targetsm->s.name, ttt);
			formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
			error(ERR_ST, tmpBuff);
			return(0);
		}
	}
	if (sourcesm != NULL) {
		if (sourcesm->s.refs!=NULL && sourcesm->s.refs->next!=NULL) {
			//& if (pcharFlag==0) {pcharFlag=1; fprintf(ccOut,":[warning] ");}
			javaGetClassNameFromFileNum(thisclassi, ttt, DOTIFY_NAME);
			sprintf(tmpBuff, "There are several references of %s syntactically applied on %s. This may cause that the refactoring will not be behaviour preserving!", sourcesm->s.name, ttt);
			formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
			warning(ERR_ST, tmpBuff);
			res = 0;
		}
	}
	return(res);
}

// ---------------------------------------------------------------------------------


static int refactoryHandleSafetyCheckDifferenceLists(
	S_editorMarkerList *diff1, S_editorMarkerList *diff2, S_olcxReferences *diffrefs
	) {
	int res, pbflag;
	S_olcxReferences *refs;
	S_olSymbolsMenu *mm;
	res = 1;
	if (diff1!=NULL || diff2!=NULL) {
		res = 0;
//&editorDumpMarkerList(diff1);
//&editorDumpMarkerList(diff2);
		for(mm=diffrefs->menuSym; mm!=NULL; mm=mm->next) {
			mm->selected = mm->visible = 1;
			mm->ooBits = 07777777;
			// hack, freeing now all diffs computed by old method
			olcxFreeReferences(mm->s.refs); mm->s.refs = NULL;
		}
//&editorDumpMarkerList(diff1);
		//& safetyCheckFailPrepareRefStack();
		//& refactoryPushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_LOST);
		//& refactoryPushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_FOUND);
		refactoryPushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
		refactoryPushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
		editorFreeMarkerListNotMarkers(diff1);
		editorFreeMarkerListNotMarkers(diff2);
		olcxPopOnly();
		if (s_ropt.theRefactoring == PPC_AVR_RENAME_PACKAGE) {
			refactoryDisplayResolutionDialog(
"The package exists and is referenced in the original project. Renaming will join two packages without possibility of inverse refactoring",
				PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
		} else {
			refactoryDisplayResolutionDialog(
				"Those references may be  misinterpreted after refactoring",
				PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
		}
	}
	return(res);
}


static int refactoryMakeSafetyCheckAndUndo(
	S_editorBuffer *buf, S_editorMarker *point, 
	S_editorMarkerList **occs, S_editorUndo *startPoint,
	S_editorUndo **redoTrack
	) {
	int 				res, pbflag;
	S_editorMarkerList 	*chks, *dd;
	S_editorMarker	 	*defin;
	S_reference 		*df1, *df2, *rr;
	S_editorMarkerList 	*diff1, *diff2;
	S_olcxReferences 	*refs, *origrefs, *newrefs, *diffrefs;
	S_olSymbolsMenu 	*mm;
	// safety check

	defin = point;
	// find definition reference? why this was there?
	//&for(dd= *occs; dd!=NULL; dd=dd->next) {
	//&	if (IS_DEFINITION_USAGE(dd->usg.base)) break;
	//&}
	//&if (dd != NULL) defin = dd->d;

	olcxPushSpecialCheckMenuSym(OLO_SAFETY_CHECK_INIT, LINK_NAME_SAFETY_CHECK_MISSED);
	refactorySafetyCheck( s_ropt.project, defin->buffer, defin);

	chks = editorReferencesToMarkers(s_olcxCurrentUser->browserStack.top->r,filter0, NULL);
//&sprintf(tmpBuff,"\nchecking\n");ppcGenTmpBuff();editorDumpMarkerList(*occs);sprintf(tmpBuff,"and\n");ppcGenTmpBuff;editorDumpMarkerList(chks);
	editorMarkersDifferences(occs, &chks, &diff1, &diff2);
//&fprintf(dumpOut,"\ndiff1==\n");editorDumpMarkerList(diff1);fprintf(dumpOut,"diff2==\n");editorDumpMarkerList(diff2);fflush(dumpOut);
	editorFreeMarkersAndMarkerList(chks);

//&fprintf(dumpOut,"\ndiff1==\n");editorDumpMarkerList(diff1);fprintf(dumpOut,"diff2==\n");editorDumpMarkerList(diff2);fflush(dumpOut);

	editorUndoUntil(startPoint, redoTrack);

//&fprintf(dumpOut,"\ndiff1==\n");editorDumpMarkerList(diff1);fprintf(dumpOut,"diff2==\n");editorDumpMarkerList(diff2);fflush(dumpOut);
	origrefs = newrefs = diffrefs = NULL;
	SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
	assert(origrefs!=NULL && newrefs!=NULL && diffrefs!=NULL);
	res = refactoryHandleSafetyCheckDifferenceLists(diff1, diff2, diffrefs);
	return(res);
}

void refactoryAskForReallyContinueConfirmation() {
	ppcGenRecord(PPC_ASK_CONFIRMATION,"The refactoring may change program behaviour, really continue?", "\n");
}

static void refactoryPreCheckThatSymbolRefsCorresponds(char *oldName, S_editorMarkerList *occs) {
	char 				*bVal, *cid;
	int					check, off1, off2;
	S_editorMarkerList 	*ll;
	S_editorMarker		*pos, *pp;
	for(ll=occs; ll!=NULL; ll=ll->next) {
		pos = ll->d;
		// first check that I have updated reference
		cid = refactoryGetIdentifierOnMarker_st(pos);
		if (strcmp(cid, oldName)!=0) {
			sprintf(tmpBuff, 
				   "something goes wrong: expecting %s\ninstead of %s at %s, offset:%d",
				   oldName, cid, simpleFileName(getRealFileNameStatic(pos->buffer->name)), 
				   pos->offset);
			error(ERR_INTERNAL, tmpBuff);
			return;
		}
		// O.K. check also few characters around
		off1 = pos->offset - RRF_CHARS_TO_PRE_CHECK_AROUND;
		off2 = pos->offset + strlen(oldName) + RRF_CHARS_TO_PRE_CHECK_AROUND;
		if (off1 < 0) off1 = 0;
		if (off2 >= pos->buffer->a.bufferSize) off2 = pos->buffer->a.bufferSize-1;
		pp = editorCrNewMarker(pos->buffer, off1);
		bVal = pos->buffer->a.text + off1;
		ppcGenPreCheckRecord(pp, off2-off1);
		editorFreeMarker(pp);
	}
}

static void refactoryMakeSyntaxPassOnSource(S_editorMarker *point) {
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, 
								   point, NULL, "-olcxsyntaxpass",NULL);
	olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
}

static S_editorMarker *refactoryCrNewMarkerForExpressionBegin(S_editorMarker *d, int kind) {
	S_editorMarker  *pp;
	S_editorBuffer  *bb;
	S_position		*pos;
	refactoryEditServerParseBuffer( s_ropt.project, d->buffer, 
								   d,NULL, "-olcxprimarystart",NULL);
	olStackDeleteSymbol(s_olcxCurrentUser->browserStack.top);
	if (kind == GET_PRIMARY_START) {
		pos = &s_primaryStartPosition;
	} else if (kind == GET_STATIC_PREFIX_START) {
		pos = &s_staticPrefixStartPosition;
	} else {
		pos = NULL;
		assert(0);
	}
	if (pos->file == s_noneFileIndex) {
		if (kind == GET_STATIC_PREFIX_START) {
			refactoryFatalErrorOnPosition(d, ERR_ST, "Can't determine static prefix. Maybe non-static reference to a static object? Make this invocation static before refactoring.");
		} else {
			refactoryFatalErrorOnPosition(d, ERR_INTERNAL, "Can't determine beginning of primary expression");
		}
		return(NULL);
	} else {
		bb = editorGetOpenedAndLoadedBuffer(s_fileTab.tab[pos->file]->name);
		pp = editorCrNewMarker(bb, 0);
		editorMoveMarkerToLineCol(pp, pos->line, pos->coll);
		assert(pp->buffer == d->buffer);
		assert(pp->offset <= d->offset);
		return(pp);
	}
}

static int refactoryShouldIMoveDirectoryNow(S_editorUndo *undoBase, char *dir) {
	S_editorUndo *uu;
	int dlen;
	dlen = strlen(dir);
	for(uu = s_editorUndo; uu!=NULL && uu!=undoBase; uu=uu->next) {
		if (uu->operation == UNDO_RENAME_BUFFER) {
//&sprintf(tmpBuff,"new checking\n%s to\n%s", uu->u.rename.name, dir);ppcGenRecord(PPC_WARNING, tmpBuff, "\n");
			if (fnnCmp(uu->u.rename.name, dir, dlen)==0
				&& (uu->u.rename.name[dlen] == '/'
					|| uu->u.rename.name[dlen] == '\\')) {
				return(0);
			}
		}
	}
	return(1);
}

static void refactoryCheckedRenameBuffer(
	S_editorBuffer *buff, char *newName, S_editorUndo **undo
	) {
	struct stat st;
	if (statb(newName, &st)==0) {
		sprintf(tmpBuff, "Renaming buffer %s to an existing file.\nCan I do this?", buff->name);
		ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff, "\n");
	}
	editorRenameBuffer(buff, newName, undo);
}

static void refactoryMoveFileAndDirForPackageRename(
	char *currentPath, S_editorUndo *undoBase, S_editorMarker *lld, 
	char *symLinkName
	) {
	char newfile[MAX_FILE_NAME_SIZE];
	char packdir[MAX_FILE_NAME_SIZE];
	char newpackdir[MAX_FILE_NAME_SIZE];
	char path[MAX_FILE_NAME_SIZE];
	int plen;
	strcpy(path, currentPath);
	plen = strlen(path);	
	if (plen>0 && (path[plen-1]=='/'||path[plen-1]=='\\')) {
		plen--;
		path[plen]=0;
	}
	sprintf(packdir, "%s%c%s", path, SLASH, symLinkName);
	sprintf(newpackdir, "%s%c%s", path, SLASH, s_ropt.renameTo);
	javaSlashifyDotName(newpackdir+strlen(path));
	sprintf(newfile, "%s%s", newpackdir, lld->buffer->name+strlen(packdir));
	refactoryCheckedRenameBuffer(lld->buffer, newfile, &s_editorUndo);
}


static int refactoryRenamePackageFileMove(char *currentPath, S_editorMarkerList *ll,
										  char *symLinkName, int slnlen,
										  S_editorUndo *rpundo) {
	int 				res, plen;
	res = 0;
	plen = strlen(currentPath);
//&sprintf(tmpBuff,"checking %s<->%s, %s<->%s\n",ll->d->buffer->name, currentPath,ll->d->buffer->name+plen+1, symLinkName);ppcGenRecord(PPC_WARNING,tmpBuff,"\n");
	if (fnnCmp(ll->d->buffer->name, currentPath, plen)==0
		&& ll->d->buffer->name[plen] == SLASH
		&& fnnCmp(ll->d->buffer->name+plen+1, symLinkName, slnlen)==0) {
		refactoryMoveFileAndDirForPackageRename( currentPath, rpundo, ll->d, symLinkName);
		res = 1;
		goto fini;
	}
 fini:
	return(res);
}
					
static void refactorySimplePackageRenaming(
	S_editorMarkerList *occs, S_editorMarker *point, 
	char *symname,char *symLinkName, int symtype
	) {
	char 				rtpack[MAX_FILE_NAME_SIZE];
	char 				rtprefix[MAX_FILE_NAME_SIZE];
	char				*ss;
	int					snlen, slnlen, mvfile;
	S_editorMarkerList 	*ll;
	S_editorMarker 		*pp;
	S_editorUndo		*rpundo;
	// get original and new directory, but how?
	snlen = strlen(symname);
	slnlen = strlen(symLinkName);
	strcpy(rtprefix, s_ropt.renameTo);
	ss = lastOccurenceInString(rtprefix, '.');
	if (ss == NULL) {
		strcpy(rtpack, rtprefix);
		rtprefix[0]  = 0;
	} else {
		strcpy(rtpack, ss+1);
		*(ss+1)=0;
	}
	for(ll=occs; ll!=NULL; ll=ll->next) {
		pp = refactoryCrNewMarkerForExpressionBegin(ll->d, GET_STATIC_PREFIX_START);
		if (pp!=NULL) {
			refactoryRemoveNonCommentCode(pp, ll->d->offset - pp->offset);
			// make attention here, so that markers still points 
			// to the package name, the best would be to replace 
			// package name per single names, ...
			refactoryCheckedReplaceString(pp, snlen, symname, rtpack);
			refactoryReplaceString(pp, 0, rtprefix);
		}
		editorFreeMarker(pp);
	}
	rpundo = s_editorUndo;
	for(ll=occs; ll!=NULL; ll=ll->next) {
		if (ll->next == NULL || ll->next->d->buffer!=ll->d->buffer) {
			// O.K. verify whether I should move the file
			JavaMapOnPaths(s_javaSourcePaths, {
				mvfile = refactoryRenamePackageFileMove(currentPath, ll, symLinkName, 
														slnlen, rpundo);
				if (mvfile) goto moved;
			});
#if ZERO
			// TODO! try to make this correct also for auto-inferred classpath !!!!!
			// well this is big hack, get inferred classpath of only 
			// the LAST file !!!!
			if (s_javaStat->namedPackageDir) {
				mvfile = refactoryRenamePackageFileMove(s_javaStat->namedPackageDir, ll, 
														symLinkName, slnlen, rpundo);
				if (mvfile) goto moved;
			}
#endif
		moved:;
		}
	}
}

static void refactorySimpleRenaming(S_editorMarkerList *occs, S_editorMarker *point, 
									char *symname,char *symLinkName, int symtype) {
	char 				nfile[MAX_FILE_NAME_SIZE];
	char 				*ss;
	S_editorMarkerList 	*ll;
	if (symtype == TypeStruct && LANGUAGE(LAN_JAVA)
		&& s_ropt.theRefactoring != PPC_AVR_RENAME_CLASS) {
		error(ERR_INTERNAL, "Use Rename Class to rename classes");
	}
	if (symtype == TypePackage && LANGUAGE(LAN_JAVA)
		&& s_ropt.theRefactoring != PPC_AVR_RENAME_PACKAGE) {
		error(ERR_INTERNAL, "Use Rename Package to rename packages");
	}

	if (s_ropt.theRefactoring == PPC_AVR_RENAME_PACKAGE) {
		refactorySimplePackageRenaming(occs, point, symname, symLinkName, symtype);
	} else {
		for(ll=occs; ll!=NULL; ll=ll->next) {
			refactoryRenameTo(ll->d, symname, s_ropt.renameTo);
		}
		ppcGenGotoMarkerRecord(point);
		if (s_ropt.theRefactoring == PPC_AVR_RENAME_CLASS) {
			if (strcmp(simpleFileNameWithoutSuffix_st(point->buffer->name), symname)==0) {
				// O.K. file name equals to class name, rename file
				strcpy(nfile, point->buffer->name);
				ss = lastOccurenceOfSlashOrAntiSlash(nfile);
				if (ss==NULL) ss=nfile;
				else ss++;
				sprintf(ss, "%s.java", s_ropt.renameTo);
				InternalCheck(strlen(nfile) < MAX_FILE_NAME_SIZE-1);
				if (strcmp(nfile, point->buffer->name)!=0) {
					// O.K. I should move file
					refactoryCheckedRenameBuffer(point->buffer, nfile, &s_editorUndo);
				}
			}
		}
	}
}

static S_editorMarkerList *refactoryGetReferences(
	S_editorBuffer *buf, S_editorMarker *point, 
	char *resolveMessage, int messageType
	) {
	S_editorMarkerList *occs;
	refactoryPushReferences( buf, point, "-olcxrename", resolveMessage,
							messageType);
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	occs = editorReferencesToMarkers(s_olcxCurrentUser->browserStack.top->r, filter0, NULL);
	return(occs);
}


static S_editorMarkerList *refactoryPushGetAndPreCheckReferences(
	S_editorBuffer *buf, S_editorMarker *point, char *nameOnPoint, 
	char *resolveMessage, int messageType
	) {
	S_editorMarkerList *occs;
	occs = refactoryGetReferences(buf, point, resolveMessage, messageType);
	refactoryPreCheckThatSymbolRefsCorresponds(nameOnPoint, occs);
	return(occs);
}


static S_editorMarker *refactoryFindModifierAndCrMarker(
	S_editorMarker *point, char *modifier, int limitIndex
	) {
	int 			i, mlen, blen, mini;
	char 			*text;
	S_editorMarker	*mm, *mb, *me;

	text = point->buffer->a.text;
	blen = point->buffer->a.bufferSize;
	mlen = strlen(modifier);
	refactoryMakeSyntaxPassOnSource(point);
	if (s_spp[limitIndex].file == s_noneFileIndex) {
		warning(ERR_INTERNAL, "cant get field declaration");
		mini = point->offset;
		while (mini>0 && text[mini]!='\n') mini --;
		i = point->offset; 
	} else {
		mb = editorCrNewMarkerForPosition(&s_spp[limitIndex]);
		// TODO, this limitIndex+2 should be done more semantically
		me = editorCrNewMarkerForPosition(&s_spp[limitIndex+2]);
		mini = mb->offset;
		i = me->offset;
		editorFreeMarker(mb);
		editorFreeMarker(me);
	}
	while (i>=mini) {
		if (strncmp(text+i, modifier, mlen)==0
			&& (i==0 || isspace(text[i-1]))
			&& (i+mlen==blen || isspace(text[i+mlen]))) {
			mm = editorCrNewMarker(point->buffer, i);
			return(mm);
		}
		i--;
	}
	return(NULL);
}

static void refactoryRemoveModifier(
	S_editorMarker *point, int limitIndex, char *modifier) {
	int 			i, j, mlen;
	char 			*text;
	S_editorMarker	*mm;
	mlen = strlen(modifier);
	text = point->buffer->a.text;
	mm = refactoryFindModifierAndCrMarker(point, modifier, limitIndex);
	if (mm!=NULL) {
		i = mm->offset;
		for(j=i+mlen; isspace(text[j]); j++) ;
		refactoryReplaceString(mm, j-i, "");
	}
	editorFreeMarker(mm);
}

static void refactoryAddModifier(S_editorMarker *point, int limit, char *modifier) {
	char			modifSpace[TMP_STRING_SIZE];
	S_editorMarker	*mm;
	refactoryMakeSyntaxPassOnSource(point);
	if (s_spp[limit].file == s_noneFileIndex) {
		error(ERR_INTERNAL, "cant find beginning of field declaration");
	}
	mm = editorCrNewMarkerForPosition(&s_spp[limit]);
	sprintf(modifSpace, "%s ", modifier);
	refactoryReplaceString(mm, 0, modifSpace);
	editorFreeMarker(mm);
}

static void refactoryChangeAccessModifier(
	S_editorMarker *point, int limitIndex, char *modifier
	) {
	S_editorMarker *mm;
	mm = refactoryFindModifierAndCrMarker(point, modifier, limitIndex);
	if (mm == NULL) {
		refactoryRemoveModifier(point, limitIndex, "private");
		refactoryRemoveModifier(point, limitIndex, "protected");
		refactoryRemoveModifier(point, limitIndex, "public");
		if (*modifier) refactoryAddModifier(point, limitIndex, modifier);
	} else {
		editorFreeMarker(mm);
	}
}

static void refactoryRestrictAccessibility(S_editorMarker *point, int limitIndex, int minAccess) {
	int 			accIndex, access;
	S_reference		*rr;

	minAccess &= ACC_PPP_MODIFER_MASK;
	for(accIndex=0; accIndex<MAX_REQUIRED_ACCESS; accIndex++) {
		if (s_javaRequiredeAccessibilitiesTable[accIndex] == minAccess) break;
	}

	// must update, because usualy they are out of date here
	refactoryUpdateReferences(s_ropt.project);

	refactoryPushReferences(point->buffer, point, "-olcxrename", NULL, 0);
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->menuSym);

	for(rr=s_olcxCurrentUser->browserStack.top->r; rr!=NULL; rr=rr->next) {
		if (! IS_DEFINITION_OR_DECL_USAGE(rr->usg.base)) {
			if (rr->usg.requiredAccess < accIndex) {
				accIndex = rr->usg.requiredAccess;
			}
		}
	}

	olcxPopOnly();

	access = s_javaRequiredeAccessibilitiesTable[accIndex];

	if (access == ACC_PUBLIC) refactoryChangeAccessModifier(point, limitIndex, "public");
	else if (access == ACC_PROTECTED) refactoryChangeAccessModifier(point, limitIndex, "protected");
	else if (access == ACC_DEFAULT) refactoryChangeAccessModifier(point, limitIndex, "");
	else if (access == ACC_PRIVATE) refactoryChangeAccessModifier(point, limitIndex, "private");
	else error(ERR_INTERNAL, "No access modifier computed");
}


static void refactoryCheckForMultipleReferencesOnSinglePlace2( S_symbolRefItem *p, 
															   S_olcxReferences *rstack, 
															   S_reference *r 
	) {
	S_reference *dr;
	int prefixchar;
	if (refOccursInRefs(r, rstack->r)) {
		ppcGenGotoPositionRecord(&r->p);
		sprintf(tmpBuff, "The reference at this place refers to multiple symbols. The refactoring will probably damage your program. Do you really want to continue?");
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff, "\n");
		//& olcxAppendReference(r, s_olcxCurrentUser->browserStack.top);
	}
}

static void refactoryCheckForMultipleReferencesOnSinglePlace(S_olcxReferences *rstack, S_olSymbolsMenu *ccms) {
	S_symbolRefItem  	*p, *sss;
	S_reference 		*r;
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
			refactoryCheckForMultipleReferencesOnSinglePlace2( p, rstack, r);
		}
	}
}

static void refactoryMultipleOccurencesSafetyCheck() {
	S_olcxReferences  	*rstack;

	assert( s_olcxCurrentUser!=NULL);
	rstack = s_olcxCurrentUser->browserStack.top;	
	olProcessSelectedReferences(rstack, refactoryCheckForMultipleReferencesOnSinglePlace);
}

// -------------------------------------------- Rename

static void refactoryRename(S_editorBuffer *buf, S_editorMarker *point) {
	char 				nameOnPoint[TMP_STRING_SIZE];
	char				*symLinkName, *message;
	int 				check, symtype;
	S_editorMarkerList 	*occs;
	S_editorUndo 		*undoStartPoint, *redoTrack;
	S_olSymbolsMenu		*csym;

	if (s_ropt.renameTo == NULL) {
		error(ERR_ST, "this refactoring requires -renameto=<new name> option");
	}

	refactoryUpdateReferences(s_ropt.project); 

	if (LANGUAGE(LAN_JAVA)) {
		message = STANDARD_SELECT_SYMBOLS_MESSAGE;
	} else {
		message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;
	}
	// rename
	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
	occs = refactoryPushGetAndPreCheckReferences(buf, point, nameOnPoint, message,PPCV_BROWSER_TYPE_INFO);
	csym =  s_olcxCurrentUser->browserStack.top->hkSelectedSym;
	symtype = csym->s.b.symType;
	symLinkName = csym->s.name;
	undoStartPoint = s_editorUndo;
	if (!LANGUAGE(LAN_JAVA)) {
		refactoryMultipleOccurencesSafetyCheck();
	}
	refactorySimpleRenaming( occs, point,nameOnPoint,symLinkName, symtype);
//&editorDumpBuffers();
	redoTrack = NULL;
	check = refactoryMakeSafetyCheckAndUndo(buf, point, &occs, undoStartPoint, &redoTrack);
	if (! check) {
		refactoryAskForReallyContinueConfirmation();
	}

	editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

	// finish where you have started
	ppcGenGotoMarkerRecord(point);

	// no messages, they make problem on extract method
	//&if (check) ppcGenRecord(PPC_INFORMATION,"Symbol has been safely renamed","\n");
	//&else ppcGenRecord(PPC_INFORMATION,"Symbol has been renamed","\n");
	editorFreeMarkersAndMarkerList(occs);  // O(n^2)!

	if (s_ropt.theRefactoring==PPC_AVR_RENAME_PACKAGE) {
		ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former package", "\n");
	} else if (s_ropt.theRefactoring==PPC_AVR_RENAME_CLASS
		&& strcmp(simpleFileNameWithoutSuffix_st(point->buffer->name), s_ropt.renameTo)==0) {
		ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class file of former class", "\n");
	}
}

void clearParamPositions() {
	s_paramPosition = s_noPos;
	s_paramBeginPosition = s_noPos;
	s_paramEndPosition = s_noPos;
}

static int refactoryGetParamNamePosition(S_editorMarker *pos, char *fname, int argn) {
	char			pushOpt[TMP_STRING_SIZE];
	char 			*actName;
	int 			res;
	actName = refactoryGetIdentifierOnMarker_st(pos);
	clearParamPositions();
	InternalCheck(strcmp(actName, fname)==0);
	sprintf(pushOpt, "-olcxgotoparname%d", argn);
	refactoryEditServerParseBuffer( s_ropt.project, pos->buffer, pos,NULL, pushOpt,NULL);
	olcxPopOnly();
	if (s_paramPosition.file != s_noneFileIndex) {
		res = RETURN_OK;
	} else {
		res = RETURN_ERROR;
	}
	return(res);
}

static int refactoryGetParamPosition(S_editorMarker *pos, char *fname, int argn) {
	char			pushOpt[TMP_STRING_SIZE];
	char 			*actName;
	int 			res;
	actName = refactoryGetIdentifierOnMarker_st(pos);
	if (! (strcmp(actName, fname)==0 
		   || strcmp(actName,"this")==0 
		   || strcmp(actName,"super")==0)) {
		ppcGenGotoMarkerRecord(pos);
		sprintf(tmpBuff, "This reference is not pointing to the function/method name. Maybe a composed symbol. Sorry, do not know how to handle this case.");
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
	}
	
	clearParamPositions();
	sprintf(pushOpt, "-olcxgetparamcoord%d", argn);
	refactoryEditServerParseBuffer( s_ropt.project, pos->buffer, pos,NULL, pushOpt,NULL);
	olcxPopOnly();

	res = RETURN_OK;
	if (s_paramBeginPosition.file == s_noneFileIndex 
		|| s_paramEndPosition.file == s_noneFileIndex
		|| s_paramBeginPosition.file == -1
		|| s_paramEndPosition.file == -1
		) {
		ppcGenGotoMarkerRecord(pos);
		error(ERR_INTERNAL, "Can't get end of parameter");
		res = RETURN_ERROR;
	}
	// check some logical preconditions, 
	if (s_paramBeginPosition.file != s_paramEndPosition.file
		|| s_paramBeginPosition.line > s_paramEndPosition.line
		|| (s_paramBeginPosition.line == s_paramEndPosition.line 
			&& s_paramBeginPosition.coll > s_paramEndPosition.coll)) {
		ppcGenGotoMarkerRecord(pos);
		sprintf(tmpBuff, "Something goes wrong at this occurence, can't get reasonable parameter limites");
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		error(ERR_ST, tmpBuff);
		res = RETURN_ERROR;
	}
	if (! LANGUAGE(LAN_JAVA)) {
		// check preconditions to avoid cases like
		// #define PAR1 toto,
		// ... lot of text
		// #define PAR2 tutu,
		//    function(PAR1 PAR2 0);
		// Hmmm, but how to do it? TODO!!!!
	}
	return(res);
}

static void dumpContext(S_editorMarker *mm) {
	int i, j, m;
	char tt[REFACTORING_TMP_STRING_SIZE];
	char *text;
	text = mm->buffer->a.text;
	sprintf(tt, "[dumping context at %s:%d]\n", mm->buffer->name, mm->offset);
	j = strlen(tt);
	i = mm->offset;
	i -= 50;
	if (i<0) i=0;
	while (i<mm->offset) {
		tt[j++] = text[i++];
	}
	sprintf(tt+j, "[MARKER]");
	j = strlen(tt);
	m = mm->offset+50;
	if (m>mm->buffer->a.bufferSize) m = mm->buffer->a.bufferSize;
	while (i<m) {
		tt[j++] = text[i++];
	}
	tt[j++]=0;
	ppcGenRecord(PPC_INFORMATION, tt, "\n");
}

// !!!!!!!!! pos and endm can be the same marker !!!!!!
static int refactoryAddStringAsParameter(S_editorMarker *pos, S_editorMarker *endm, 
										  char *fname, int argn, char *param) {
	char 			*actName, *text;
	char			pushOpt[TMP_STRING_SIZE];
	char			par[REFACTORING_TMP_STRING_SIZE];
	char			*sep1, *sep2;
	int				ipos,rr,insertionOffset;
	S_editorMarker	*mm;

	insertionOffset = -1;
	rr = refactoryGetParamPosition(pos, fname, argn);
	if (rr != RETURN_OK) {
		error(ERR_INTERNAL, "Problem while adding parameter");
		return(insertionOffset);
	}
	text = pos->buffer->a.text;

	if (endm==NULL) {
		mm = editorCrNewMarkerForPosition(&s_paramBeginPosition);
	} else {
		mm = endm;
		assert(mm->buffer->ftnum == s_paramBeginPosition.file);
		editorMoveMarkerToLineCol(mm, s_paramBeginPosition.line, s_paramBeginPosition.coll);
	}

	sep1=""; sep2="";
	if (POSITION_EQ(s_paramBeginPosition, s_paramEndPosition)) {
		if (text[mm->offset] == '(') {
			// function with no parameter
			mm->offset ++;
			sep1=""; sep2="";
		} else if (text[mm->offset] == ')') {
			// beyond limite
			sep1=", "; sep2="";
		} else {
			ppcGenGotoMarkerRecord(pos);
			sprintf(tmpBuff, "Something goes wrong, probably different parameter coordinates at different cpp passes.");
			formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
			fatalError(ERR_INTERNAL, tmpBuff, XREF_EXIT_ERR);
			assert(0);
		}
	} else {
		if (text[mm->offset] == '(') {
			sep1 = "";
			sep2 = ", ";
		} else {
			sep1 = " ";
			sep2 = ",";
		}
		mm->offset ++;
	}

	sprintf(par, "%s%s%s", sep1, param, sep2);
	InternalCheck(strlen(param) < REFACTORING_TMP_STRING_SIZE-1);

	insertionOffset = mm->offset;
	refactoryReplaceString(mm, 0, par);
	if (endm == NULL) {
		editorFreeMarker(mm);
	}
	// O.K. I hope that mm == endp is moved to
	// end of inserted parameter
	return(insertionOffset);
}

static int refactoryIsThisSymbolUsed(S_editorMarker *pos) {
	int refn;
	refactoryPushReferences( pos->buffer, pos, "-olcxpushforlm",STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
	LIST_LEN(refn, S_reference, s_olcxCurrentUser->browserStack.top->r);
	olcxPopOnly();
	return(refn > 1);
}

static int refactoryIsParameterUsedExceptRecursiveCalls(S_editorMarker *ppos, S_editorMarker *fpos, char *fname) {
	// for the moment
	return(refactoryIsThisSymbolUsed(ppos));
}

static void refactoryCheckThatParameterIsUnused(S_editorMarker *pos, char *fname, 
												int argn, int checkfor) {
	char 			*actName, *text;
	char			pname[TMP_STRING_SIZE];
	int				ipos,rr;
	S_editorMarker	*mm;

	rr = refactoryGetParamNamePosition(pos, fname, argn);
	if (rr != RETURN_OK) {
		ppcGenRecord(PPC_ASK_CONFIRMATION, "Can not parse parameter definition, continue anyway?", "\n");
		return;
	}

	mm = editorCrNewMarkerForPosition(&s_paramPosition);
	strncpy(pname, refactoryGetIdentifierOnMarker_st(mm), TMP_STRING_SIZE);
	pname[TMP_STRING_SIZE-1] = 0;
	text = pos->buffer->a.text;
	if (refactoryIsParameterUsedExceptRecursiveCalls(mm, pos, fname)) {
		if (checkfor==CHECK_FOR_ADD_PARAM) {
			sprintf(tmpBuff, "parameter '%s' clashes with an existing symbol, continue anyway?", pname);
			ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff, "\n");
		} else if (checkfor==CHECK_FOR_DEL_PARAM) {
			sprintf(tmpBuff, "parameter '%s' is used, delete it anyway?", pname);
			ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff, "\n");
		} else {
			assert(0);
		}
	}
	editorFreeMarker(mm);
}

static void refactoryAddParameter(S_editorMarker *pos, char *fname, 
								  int argn, int usage) {
	if (IS_DEFINITION_OR_DECL_USAGE(usage)) {
		refactoryAddStringAsParameter(pos,NULL, fname, argn, s_ropt.refpar1);
		// now check that there is no conflict
		if (IS_DEFINITION_USAGE(usage)) {
			refactoryCheckThatParameterIsUnused(pos, fname, argn, CHECK_FOR_ADD_PARAM);
		}
	} else {
		refactoryAddStringAsParameter(pos,NULL, fname, argn, s_ropt.refpar2);
	}
}

static void refactoryDeleteParameter(S_editorMarker *pos, char *fname, 
									 int argn, int usage) {
	char 			*text;
	char 			pname[TMP_STRING_SIZE];
	int				ipos, res;
	S_editorMarker	*m1, *m2;

	res = refactoryGetParamPosition(pos, fname, argn);
	if (res != RETURN_OK) return;

	m1 = editorCrNewMarkerForPosition(&s_paramBeginPosition);
	m2 = editorCrNewMarkerForPosition(&s_paramEndPosition);

	text = pos->buffer->a.text;

	if (POSITION_EQ(s_paramBeginPosition, s_paramEndPosition)) {
		if (text[m1->offset] == '(') {
			// function with no parameter
		} else if (text[m1->offset] == ')') {
			// beyond limite
		} else {
			fatalError(ERR_INTERNAL,"Something goes wrong, probably different parameter coordinates at different cpp passes.", XREF_EXIT_ERR);
			assert(0);
		}
		error(ERR_ST, "Parameter out of limit");
	} else {
		if (text[m1->offset] == '(') {
			m1->offset ++;
			if (text[m2->offset] == ',') {
				m2->offset ++;
				// here pass also blank symbols
			}
			editorMoveMarkerToNonBlank(m2, 1);
		}
		if (IS_DEFINITION_USAGE(usage)) {
			// this must be at the end, because it discards values 
			// of s_paramBeginPosition and s_paramEndPosition
			refactoryCheckThatParameterIsUnused(pos, fname, argn, CHECK_FOR_DEL_PARAM);		
		}

		assert(m1->offset <= m2->offset);
		refactoryReplaceString(m1, m2->offset - m1->offset, "");
	}
	editorFreeMarker(m1);
	editorFreeMarker(m2);
}

static void refactoryMoveParameter(S_editorMarker *pos, char *fname, 
								   int argFrom, int argTo) {
	char 			*actName, *text;
	char			par[REFACTORING_TMP_STRING_SIZE];
	int				ipos, res, plen;
	S_editorMarker	*m1, *m2;

	res = refactoryGetParamPosition(pos, fname, argFrom);
	if (res != RETURN_OK) return;

	m1 = editorCrNewMarkerForPosition(&s_paramBeginPosition);
	m2 = editorCrNewMarkerForPosition(&s_paramEndPosition);

	text = pos->buffer->a.text;
	plen = 0;
	par[plen]=0;

	if (POSITION_EQ(s_paramBeginPosition, s_paramEndPosition)) {
		if (text[m1->offset] == '(') {
			// function with no parameter
		} else if (text[m1->offset] == ')') {
			// beyond limite
		} else {
			fatalError(ERR_INTERNAL,"Something goes wrong, probably different parameter coordinates at different cpp passes.", XREF_EXIT_ERR);
			assert(0);
		}
		error(ERR_ST, "Parameter out of limit");
	} else {
		m1->offset++;
		editorMoveMarkerToNonBlank(m1, 1);
		m2->offset--;
		editorMoveMarkerToNonBlank(m2, -1);
		m2->offset++;
		assert(m1->offset <= m2->offset);
		plen = m2->offset - m1->offset;
		strncpy(par, MARKER_TO_POINTER(m1), plen);
		par[plen]=0;
		refactoryDeleteParameter(pos, fname, argFrom, UsageUsed);
		refactoryAddStringAsParameter(pos, NULL, fname, argTo, par);
	}
	editorFreeMarker(m1);
	editorFreeMarker(m2);
}

static void refactoryApplyParamManip(char *functionName, S_editorMarkerList *occs,
									 int manip, int argn1, int argn2
	) {
	S_editorMarkerList *ll;
	int progressi, progressn;
	LIST_LEN(progressn, S_editorMarkerList, occs); progressi=0;
	for(ll=occs; ll!=NULL; ll=ll->next) {
		if (ll->usg.base != UsageUndefinedMacro) {
			if (manip==PPC_AVR_ADD_PARAMETER) {
				refactoryAddParameter(ll->d, functionName, argn1, 
									  ll->usg.base);
			} else if (manip==PPC_AVR_DEL_PARAMETER) {
				refactoryDeleteParameter(ll->d, functionName, argn1, 
										 ll->usg.base);
			} else if (manip==PPC_AVR_MOVE_PARAMETER) {
				refactoryMoveParameter(ll->d, functionName, 
									   argn1, argn2
					);
			} else {
				error(ERR_INTERNAL, "unknown parameter manipulation");
			}
		}
		writeRelativeProgress((progressi++)*100/progressn);
	}
	writeRelativeProgress(100);
}


// -------------------------------------- ParameterManipulations

static void refactoryApplyParameterManipulation(S_editorBuffer *buf, S_editorMarker *point,
										   int manip, int argn1, int argn2) {
	char 				nameOnPoint[TMP_STRING_SIZE];
	int 				check;
	S_editorMarkerList 	*occs, *ll, *chks, *diff1, *diff2;
	S_reference 		*df1, *df2, *rr;
	S_editorUndo 		*startPoint, *redoTrack;

	refactoryUpdateReferences(s_ropt.project); 

	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	refactoryPushReferences( buf, point, "-olcxargmanip",STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
	occs = editorReferencesToMarkers(s_olcxCurrentUser->browserStack.top->r, filter0, NULL);
	startPoint = s_editorUndo;
	// first just check that loaded files are up to date
	//& refactoryPreCheckThatSymbolRefsCorresponds(nameOnPoint, occs);

//&editorDumpBuffer(occs->d->buffer);
//&editorDumpMarkerList(occs);
	// for some error mesages it is more natural that cursor does not move
	ppcGenGotoMarkerRecord(point);
	redoTrack = NULL;
	refactoryApplyParamManip(nameOnPoint, occs, 
							  manip, argn1, argn2
);
	if (LANGUAGE(LAN_JAVA)) {
		check = refactoryMakeSafetyCheckAndUndo(buf, point, &occs, startPoint, &redoTrack);
		if (! check) refactoryAskForReallyContinueConfirmation();
		editorApplyUndos(redoTrack, NULL, &s_editorUndo, GEN_NO_OUTPUT);
	}
	editorFreeMarkersAndMarkerList(occs);  // O(n^2)!
}

static void refactoryParameterManipulation(S_editorBuffer *buf, S_editorMarker *point,
										   int manip, int argn1, int argn2) {
	refactoryApplyParameterManipulation(buf, point, manip, argn1, argn2);
	// and generate output
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
}

static int createMarkersForAllReferencesInRegions(
	S_olSymbolsMenu *menu, S_editorRegionList **regions
	) {
	S_olSymbolsMenu *mm;
	int 			res, n, nn;
	res = 0;
	for(mm=menu; mm!=NULL; mm=mm->next) {
		InternalCheck(mm->markers==NULL);
		if (mm->selected && mm->visible) {
//&LIST_LEN(nn, S_reference, mm->s.refs);sprintf(tmpBuff,"there are %d refs for %s", nn, s_fileTab.tab[mm->s.vApplClass]->name);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
			mm->markers = editorReferencesToMarkers(mm->s.refs,filter0, NULL);
			if (regions != NULL) {
				editorRestrictMarkersToRegions(&mm->markers, regions);
			}
//&LIST_LEN(nn, S_editorMarkerList, mm->markers);sprintf(tmpBuff,"converted to %d refs for %s", nn, s_fileTab.tab[mm->s.vApplClass]->name);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
			LIST_MERGE_SORT(S_editorMarkerList, mm->markers, editorMarkerListLess);
			LIST_LEN(n, S_editorMarkerList, mm->markers);
			res += n;
		}
	}
	return(res);
}

// --------------------------------------- ExpandShortNames

static void refactoryApplyExpandShortNames(S_editorBuffer *buf, S_editorMarker *point) {
	S_olSymbolsMenu		*mm;
	S_editorMarkerList	*ppp;
	char				fqtName[MAX_FILE_NAME_SIZE];
	char				fqtNameDot[MAX_FILE_NAME_SIZE];
	char				*shortName;
	int					shortNameLen, n;

	refactoryEditServerParseBuffer( s_ropt.project, buf, point,NULL, "-olcxnotfqt",NULL);
	olcxPushSpecial(LINK_NAME_NOT_FQT_ITEM, OLO_NOT_FQT_REFS);

	// Do it in two steps because when changing file the references
	// are not updated while markers are, so first I need to change
	// everything to markers
	createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, NULL);
	// Hmm. what if one reference will be twice ? Is it possible?
	for(mm = s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			javaGetClassNameFromFileNum(mm->s.vApplClass, fqtName, DOTIFY_NAME);
			javaDotifyClassName(fqtName);
			sprintf(fqtNameDot, "%s.", fqtName);
			shortName = javaGetShortClassName(fqtName);
			shortNameLen = strlen(shortName);
			if (isdigit(shortName[0])) {
				goto cont;	// anonymous nested class, no expansion
			}
			for(ppp=mm->markers; ppp!=NULL; ppp=ppp->next) {
//&fprintf(dumpOut,"expanding at %s:%d\n", ppp->d->buffer->name, ppp->d->offset);
				if (ppp->usg.base == UsageNonExpandableNotFQTNameInClassOrMethod) {
					ppcGenGotoMarkerRecord(ppp->d);
					sprintf(tmpBuff, "This occurence of %s would be misinterpreted after expansion to %s.\nNo action made at this place.", shortName, fqtName);
					warning(ERR_ST, tmpBuff);
				} else if (ppp->usg.base == UsageNotFQFieldInClassOrMethod) {
					refactoryReplaceString(ppp->d, 0, fqtNameDot);
				} else if (ppp->usg.base == UsageNotFQTypeInClassOrMethod) {
					refactoryCheckedReplaceString(ppp->d, shortNameLen, shortName, fqtName);
				}
			}
		}
	cont:;
		editorFreeMarkerListNotMarkers(mm->markers);
		mm->markers = NULL;
 	}
//&editorDumpBuffer(buf);
}

static void refactoryExpandShortNames(S_editorBuffer *buf, S_editorMarker *point) {
	refactoryApplyExpandShortNames(buf, point);
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
}

static S_editorMarker *refactoryReplaceStaticPrefix(S_editorMarker *d, char *npref) {
	int 				ppoffset, npreflen;
	S_editorMarker 		*pp;
	char				pdot[MAX_FILE_NAME_SIZE];
	// For speed reasons maybe make special parser for static prefix?
	pp = refactoryCrNewMarkerForExpressionBegin(d, GET_STATIC_PREFIX_START);
	if (pp==NULL) {
		// this is an error, this is just to avoid possible core dump in the future
		pp = editorCrNewMarker(d->buffer, d->offset);
	} else {
		ppoffset = pp->offset;
		refactoryRemoveNonCommentCode(pp, d->offset-pp->offset);
		if (*npref!=0) {
			npreflen = strlen(npref);
			strcpy(pdot, npref);
			pdot[npreflen] = '.'; pdot[npreflen+1]=0;
			refactoryReplaceString(pp, 0, pdot);
		}
		// return it back to beginning of fqt
		pp->offset = ppoffset;
	}
	return(pp);
}

// -------------------------------------- ReduceLongNames

static void refactoryReduceLongReferencesInRegions(
	S_editorMarker 		*point, 
	S_editorRegionList 	**regions
	) {
	S_editorMarkerList 	*rli, *ri, *ro, *rr;
	S_editorRegionList 	*reg;
	S_editorMarker		*pp;
	int 				progressi, progressn;

	refactoryEditServerParseBuffer(s_ropt.project, point->buffer, point,NULL, 
									"-olcxuselesslongnames", "-olallchecks");
	olcxPushSpecial(LINK_NAME_IMPORTED_QUALIFIED_ITEM, OLO_USELESS_LONG_NAME);
 	rli = editorReferencesToMarkers(s_olcxCurrentUser->browserStack.top->r, filter0, NULL);
	editorSplitMarkersWithRespectToRegions(&rli, regions, &ri, &ro);
	editorFreeMarkersAndMarkerList(ro); ro = NULL;

	// a hack, as we cannot predict how many times this will be
	// invoked, adjust progress bar counter ratio

	s_progressFactor += 1;
	LIST_LEN(progressn, S_editorMarkerList, ri); progressi=0;
	for(rr=ri; rr!=NULL; rr=rr->next) {
		pp = refactoryReplaceStaticPrefix(rr->d, "");
		editorFreeMarker(pp);
		writeRelativeProgress((((progressi++)*100/progressn)/10)*10);
	}
	writeRelativeProgress(100);
}

// ------------------------------------------------------ Reduce Long Names In The File

static int refactoryIsTheImportUsed(S_editorMarker *point, int line, int coll) {
	char				isymName[TMP_STRING_SIZE];
	S_olSymbolsMenu 	*mm;
	int					res;

	strcpy(isymName, javaImportSymbolName_st(point->buffer->ftnum, line, coll));
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point,NULL,
									"-olcxpushfileunused", "-olallchecks");
	pushLocalUnusedSymbolsAction();
	res = 1;
	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->visible && strcmp(mm->s.name, isymName)==0) {
			res = 0;
			goto fini1;
		}
	}
 fini1:
	olcxPopOnly();
	return(res);
}

static int refactoryPushFileUnimportedFqts(S_editorMarker *point, S_editorRegionList **regions) {
	char				pushOpt[TMP_STRING_SIZE];
	int 				lastImportLine;
	sprintf(pushOpt, "-olcxpushspecialname=%s", LINK_NAME_UNIMPORTED_QUALIFIED_ITEM);
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point,NULL,pushOpt, "-olallchecks");
	lastImportLine = s_cps.lastImportLine;
	olcxPushSpecial(LINK_NAME_UNIMPORTED_QUALIFIED_ITEM, OLO_PUSH_SPECIAL_NAME);
	createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, regions);
	return(lastImportLine);
}

static int refactoryImportNeeded(S_editorMarker *point, S_editorRegionList **regions, int vApplCl) {
	S_olSymbolsMenu 	*mm;
	int					res;

	// check whether the symbol is reduced			
	refactoryPushFileUnimportedFqts(point, regions);
	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->s.vApplClass == vApplCl) {
			res = 1;
			goto fini;
		}
	}
	res = 0;
 fini:
	olcxPopOnly();
	return(res);
}

static int refactoryAddImport(S_editorMarker *point, S_editorRegionList **regions, 
							  char *iname, int line, int vApplCl, int interactive) {
	char 				istat[MAX_CX_SYMBOL_SIZE];
	char 				icoll;
	char				*ld1, *ld2, *ss;
	S_editorMarker 		*mm;
	int					res;
	S_editorUndo		*undoBase;
	S_editorRegionList 	*wholeBuffer;

	undoBase = s_editorUndo;
	sprintf(istat, "import %s;\n", iname);
	mm = editorCrNewMarker(point->buffer, 0);
	// a little hack, make one free line after 'package'
	if (line > 1) {
		editorMoveMarkerToLineCol(mm, line-1, 0);
		if (strncmp(MARKER_TO_POINTER(mm), "package", 7) == 0) {
			// insert newline
			editorMoveMarkerToLineCol(mm, line, 0);
			refactoryReplaceString(mm, 0, "\n");
			line ++;
		}
	}
	// add the import
	editorMoveMarkerToLineCol(mm, line, 0);
	refactoryReplaceString(mm, 0, istat);

	res = 1;

	// TODO verify that import is unused
	ld1 = NULL;
	ld2 = istat + strlen("import");
	for(ss=ld2; *ss; ss++) {
		if (*ss == '.') {
			ld1 = ld2;
			ld2 = ss;
		}
	}
	if (ld1 == NULL) {
		error(ERR_INTERNAL, "can't find imported package");
	} else {

		icoll = ld1 - istat + 1;
		if (refactoryIsTheImportUsed(mm, line, icoll)) {
			if (interactive == INTERACTIVE_YES) {
				ppcGenRecord(PPC_WARNING,
							 "Sorry, adding this import would cause misinterpretation of\nsome of classes used elsewhere it the file.",
							 "\n");
			}
			res = 0;
		} else {
			wholeBuffer = editorWholeBufferRegion(point->buffer);
			refactoryReduceLongReferencesInRegions(point, &wholeBuffer);
			editorFreeMarkersAndRegionList(wholeBuffer); wholeBuffer=NULL;
			if (refactoryImportNeeded(point, regions, vApplCl)) {
				if (interactive == INTERACTIVE_YES) {
					ppcGenRecord(PPC_WARNING,
								 "Sorry, this import will not help to reduce class references.",
								 "\n");
				}
				res = 0;
			}
		}
	}
	if (res == 0) editorUndoUntil(undoBase, NULL);

	editorFreeMarker(mm);
	return(res);
}

static int isInDisabledList(S_disabledList *list, int file, int vApplCl) {
	S_disabledList *ll;
	for(ll=list; ll!=NULL; ll=ll->next) {
		if (ll->file == file && ll->clas == vApplCl) return(1);
	}
	return(0);
}

static int translatePassToAddImportAction(int pass) {
	switch (pass) {
	case 0:	return(RC_IMPORT_ON_DEMAND);
	case 1:	return(RC_IMPORT_SINGLE_TYPE);
	case 2:	return(RC_CONTINUE);
	default: error(ERR_INTERNAL, "wrong code for noninteractive add import");
	}
	return(0);
}

static int refactoryInteractiveAskForAddImportAction(S_editorMarkerList *ppp, int defaultAction, 
													 char *fqtName
	) {
	int action;
	refactoryAplyWholeRefactoringFromUndo();  // make current state visible
	ppcGenGotoMarkerRecord(ppp->d);
	ppcGenNumericRecord(PPC_ADD_TO_IMPORTS_DIALOG,defaultAction,fqtName,"\n");
	refactoryBeInteractive();
	action = s_opt.continueRefactoring;
	return(action);
}

static void refactoryPerformReduceNamesAndAddImportsInSingleFile(
	S_editorMarker *point, S_editorRegionList **regions, int interactive
	) {
	S_olSymbolsMenu 	*mm;
	S_editorMarkerList 	*ppp;
	S_editorRegionList	*rl;
	S_editorBuffer		*b;
	int 				action, keepAdding, succ, lastImportLine;
	int					defaultAction, cfile;
	char				fqtName[MAX_FILE_NAME_SIZE];
	char				starName[MAX_FILE_NAME_SIZE];
	char				*dd;
	S_disabledList		*disabled, *dl;

	// just verify that all references are from single file
	if (regions!=NULL && *regions!=NULL) {
		b = (*regions)->r.b->buffer;
		for(rl= *regions; rl!=NULL; rl=rl->next) {
			if (rl->r.b->buffer != b || rl->r.e->buffer != b) {
				assert(0 && "[refactoryPerformAddImportsInSingleFile] region list contains multiple buffers!");
				break;
			}
		}
	}

	refactoryReduceLongReferencesInRegions(point, regions);

	disabled = NULL;
	keepAdding = 1;
	while (keepAdding) {
		keepAdding = 0;
		lastImportLine = refactoryPushFileUnimportedFqts(point, regions);
		for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
			ppp=mm->markers; 
			defaultAction = s_ropt.defaultAddImportStrategy;
			while (ppp!=NULL && keepAdding==0 
				   && !isInDisabledList(disabled, ppp->d->buffer->ftnum, mm->s.vApplClass)) {
				cfile = ppp->d->buffer->ftnum;
				javaGetClassNameFromFileNum(mm->s.vApplClass, fqtName, DOTIFY_NAME);
				javaDotifyClassName(fqtName);
				if (interactive == INTERACTIVE_YES) {
					action = refactoryInteractiveAskForAddImportAction(ppp,defaultAction,fqtName);
				} else {
					action = translatePassToAddImportAction(defaultAction);
				}
//&sprintf(tmpBuff,"%s, %s, %d", simpleFileNameFromFileNum(ppp->d->buffer->ftnum), fqtName, action); ppcGenTmpBuff();
				switch (action) {
				case RC_IMPORT_ON_DEMAND:
					strcpy(starName, fqtName);
					dd = lastOccurenceInString(starName, '.');
					if (dd!=NULL) {
						sprintf(dd, ".*");
						keepAdding = refactoryAddImport(point, regions, starName, 
														lastImportLine+1, mm->s.vApplClass, 
														interactive);
					}
					defaultAction = NID_IMPORT_ON_DEMAND;
					break;
				case RC_IMPORT_SINGLE_TYPE:
					keepAdding = refactoryAddImport(point, regions, fqtName, 
													lastImportLine+1, mm->s.vApplClass, 
													interactive);
					defaultAction = NID_SINGLE_TYPE_IMPORT;
					break;
				case RC_CONTINUE:
					ED_ALLOC(dl, S_disabledList);
					FILL_disabledList(dl, cfile, mm->s.vApplClass, disabled);
					disabled = dl;
					defaultAction = NID_KEPP_FQT_NAME;
					break;
				default:
					fatalError(ERR_INTERNAL, "wrong continuation code", XREF_EXIT_ERR);
				}
				if (defaultAction <= 1)  defaultAction ++;
			}
			editorFreeMarkersAndMarkerList(mm->markers);
			mm->markers = NULL;
		}
		olcxPopOnly();
	}
}

static void refactoryPerformReduceNamesAndAddImports(S_editorRegionList **regions, int interactive) {
	S_editorBuffer 			*cb;
	S_editorRegionList		**cr, **cl, *ncr, **ocr;
	LIST_MERGE_SORT(S_editorRegionList, *regions, editorRegionListLess);
	cr = regions;
	// split regions per files and add imports
	while (*cr!=NULL) {
		cl = cr; 
		cb = (*cr)->r.b->buffer;
		while (*cr!=NULL && (*cr)->r.b->buffer == cb) cr = &(*cr)->next;
		ncr = *cr;
		*cr = NULL; 
		refactoryPerformReduceNamesAndAddImportsInSingleFile((*cl)->r.b, cl, interactive);
		// following line this was big bug, regions may be sortes, some may even be
		// even removed due to overlaps
		//& *cr = ncr;  
		cr = cl;
		while (*cr!=NULL) cr = &(*cr)->next;
		*cr = ncr;
	}
}

// ------------------------------------------- ReduceLongReferencesAddImports

// this is reduction of all names within file
static void refactoryReduceLongNamesInTheFile(S_editorBuffer *buf, S_editorMarker *point) {
	S_editorRegionList *wholeBuffer;
	wholeBuffer = editorWholeBufferRegion(buf);
	// don't be interactive, I am too lazy to write jEdit interface
	// for <add-import-dialog>
	refactoryPerformReduceNamesAndAddImportsInSingleFile(point, &wholeBuffer, INTERACTIVE_NO);
	editorFreeMarkersAndRegionList(wholeBuffer);wholeBuffer=NULL;
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
}

// this is reduction of a single fqt, problem is with detection of applicable context
static void refactoryAddToImports(S_editorBuffer *buf, S_editorMarker *point) {
	S_editorMarker 			*bb, *ee;
	S_editorRegionList 		*reg;
	bb = editorDuplicateMarker(point);
	ee = editorDuplicateMarker(point);
	editorMoveMarkerBeyondIdentifier(bb, -1);
	editorMoveMarkerBeyondIdentifier(ee, 1);
	ED_ALLOC(reg, S_editorRegionList);
	FILLF_editorRegionList(reg, bb, ee, NULL);
	refactoryPerformReduceNamesAndAddImportsInSingleFile(point, &reg, INTERACTIVE_YES);
	editorFreeMarkersAndRegionList(reg); reg=NULL;
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
}


static void refactoryPushAllReferencesOfMethod(S_editorMarker *m1, char *specialOption) {
	refactoryEditServerParseBuffer( s_ropt.project, m1->buffer, m1,NULL, "-olcxpushallinmethod", specialOption);
	olPushAllReferencesInBetween(s_cps.cxMemiAtMethodBeginning, s_cps.cxMemiAtMethodEnd);
}


static void moveFirstElementOfMarkerList(S_editorMarkerList **l1, S_editorMarkerList **l2) {
	S_editorMarkerList	*mm;
	if (*l1!=NULL) {
		mm = *l1;
		*l1 = (*l1)->next;
		mm->next = NULL;
		LIST_APPEND(S_editorMarkerList, *l2, mm);
	}
}

static void refactoryShowSafetyCheckFailingDialog(S_editorMarkerList **totalDiff, char *message) {
	S_editorUndo *redo;
	redo = NULL;
	editorUndoUntil(s_refactoringStartPoint, &redo);
	olcxPushSpecialCheckMenuSym(OLO_SAFETY_CHECK_INIT, LINK_NAME_SAFETY_CHECK_MISSED);
	refactoryPushMarkersAsReferences(totalDiff, s_olcxCurrentUser->browserStack.top, 
									 LINK_NAME_SAFETY_CHECK_MISSED);
	refactoryDisplayResolutionDialog(message, PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
	editorApplyUndos(redo, NULL, &s_editorUndo, GEN_NO_OUTPUT);
}


#define EACH_SYMBOL_ONCE 1

static void refactoryStaticMoveCheckCorrespondance(
	S_olSymbolsMenu *menu1, S_olSymbolsMenu *menu2, S_symbolRefItem *theMethod
	) {
	S_olSymbolsMenu		*mm1, *mm2;
	S_editorMarkerList	*diff1, *diff2, *totalDiff, *mmm;
	int					problem;
	problem = 0;
	totalDiff = NULL;
	mm1=menu1;
	while (mm1!=NULL) {
		// do not check recursive calls
		if (itIsSameCxSymbolIncludingFunClass(&mm1->s, theMethod)) goto cont;
		// nor local variables
		if (mm1->s.b.storage == StorageAuto) goto cont;
		// nor labels 
		if (mm1->s.b.symType == TypeLabel) goto cont;
		// do not check also any symbols from classes defined in inner scope
		if (isStrictlyEnclosingClass(mm1->s.vFunClass, theMethod->vFunClass)) goto cont;
		// (maybe I should not test any local symbols ???)
		// O.K. something to be checked, find correspondance in mm2
//&fprintf(dumpOut, "Looking for correspondance to %s\n", mm1->s.name);
		for(mm2=menu2; mm2!=NULL; mm2=mm2->next) {
//&fprintf(dumpOut, "Checking %s\n", mm2->s.name);
			if (itIsSameCxSymbolIncludingApplClass(&mm1->s, &mm2->s)) break;
		}
		if (mm2==NULL) {
			// O(n^2) !!!
//&fprintf(dumpOut, "Did not find correspondance to %s\n", mm1->s.name);
#			ifdef EACH_SYMBOL_ONCE
			// if each symbol reported only once
			moveFirstElementOfMarkerList(&mm1->markers, &totalDiff);
#			else
			LIST_APPEND(S_editorMarkerList, totalDiff, mm1->markers);
			// hack!
			mm1->markers = NULL;
#			endif
		} else {
			editorMarkersDifferences(&mm1->markers, &mm2->markers, &diff1, &diff2);
#			ifdef EACH_SYMBOL_ONCE
			// if each symbol reported only once
			if (diff1!=NULL) {
				moveFirstElementOfMarkerList(&diff1, &totalDiff);
//&fprintf(dumpOut, "problem with symbol %s corr %s\n", mm1->s.name, mm2->s.name);
			} else {
				// no, do not put there new symbols, only lost are problems
				moveFirstElementOfMarkerList(&diff2, &totalDiff);
			}
			editorFreeMarkersAndMarkerList(diff1);
			editorFreeMarkersAndMarkerList(diff2);
#			else
			LIST_APPEND(S_editorMarkerList, totalDiff, diff1);
			LIST_APPEND(S_editorMarkerList, totalDiff, diff2);
#			endif
		}
		editorFreeMarkersAndMarkerList(mm1->markers);
		mm1->markers = NULL;
	cont:
		mm1=mm1->next;
	}
	for(mm2=menu2; mm2!=NULL; mm2=mm2->next) {
		editorFreeMarkersAndMarkerList(mm2->markers);
		mm2->markers = NULL;
	}
	if (totalDiff!=NULL) {
#		ifdef EACH_SYMBOL_ONCE
			refactoryShowSafetyCheckFailingDialog(&totalDiff,"Those references will be  misinterpreted after refactoring. Fix them first. (each symbol is reported only once)");
#		else
			refactoryShowSafetyCheckFailingDialog(&totalDiff, "Those references will be  misinterpreted after refactoring");
#		endif
		editorFreeMarkersAndMarkerList(totalDiff); totalDiff=NULL;
		refactoryAskForReallyContinueConfirmation();
	}
}

// make it public, because you update references after and some references can
// be lost, later you can restrict accessibility

static void refactoryPerformMovingOfStaticObjectAndMakeItPublic(
	S_editorMarker *mstart, S_editorMarker *point, S_editorMarker *mend, 
	S_editorMarker *target, char fqtname[], unsigned *outAccessFlags, 
	int check, int limitIndex
	) {
	char 				nameOnPoint[TMP_STRING_SIZE];
	int 				size, ppoffset;
	S_olSymbolsMenu		*mm1, *mm2;
	S_editorMarker 		*pp, *ppp, *movedEnd;
	S_editorMarkerList 	*occs, *ll, *ll2, *rli, *rll;
	S_editorRegionList	*regions, *lll;
	S_symbolRefItem		*theMethod;
	S_editorUndo 		*undoStartPoint;
	int 				progressi, progressn;


	movedEnd = editorDuplicateMarker(mend);
	movedEnd->offset --;

//&editorDumpMarker(mstart);
//&editorDumpMarker(movedEnd);

	size = mend->offset - mstart->offset;
	if (target->buffer == mstart->buffer 
		&& target->offset > mstart->offset
		&& target->offset < mstart->offset+size) {
		ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.", "\n");
		return;
	}

	// O.K. move
	refactoryApplyExpandShortNames(point->buffer, point);
	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
	occs = refactoryGetReferences(point->buffer, point,STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	if (outAccessFlags!=NULL) {
		*outAccessFlags = s_olcxCurrentUser->browserStack.top->hkSelectedSym->s.b.accessFlags;
	}
	//&refactoryEditServerParseBuffer(s_ropt.project, point->buffer, point, "-olcxrename");

	undoStartPoint = s_editorUndo;
	LIST_MERGE_SORT(S_editorMarkerList, occs, editorMarkerListLess);
	LIST_LEN(progressn, S_editorMarkerList, occs); progressi=0;
	regions = NULL; 
	for(ll=occs; ll!=NULL; ll=ll->next) {
		if ((! IS_DEFINITION_OR_DECL_USAGE(ll->usg.base))
			&& ll->usg.base!=UsageConstructorDefinition) {
			pp = refactoryReplaceStaticPrefix(ll->d, fqtname);
			ppp = editorCrNewMarker(ll->d->buffer, ll->d->offset);
			editorMoveMarkerBeyondIdentifier(ppp, 1);
			ED_ALLOC(lll, S_editorRegionList);
			FILLF_editorRegionList(lll, pp, ppp, regions);
			regions = lll;
		}
		writeRelativeProgress((progressi++)*100/progressn);
	}
	writeRelativeProgress(100);

	size = mend->offset - mstart->offset;
	if (check==NO_CHECKS) {
		editorMoveBlock(target, mstart, size, &s_editorUndo);
		refactoryChangeAccessModifier(point, limitIndex, "public");
	} else {
		assert(s_olcxCurrentUser->browserStack.top!=NULL && s_olcxCurrentUser->browserStack.top->hkSelectedSym!=NULL);
		theMethod = &s_olcxCurrentUser->browserStack.top->hkSelectedSym->s;
		refactoryPushAllReferencesOfMethod(point, "-olallchecks");
		createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, NULL);
		editorMoveBlock(target, mstart, size, &s_editorUndo);
		refactoryChangeAccessModifier(point, limitIndex, "public");
		refactoryPushAllReferencesOfMethod(point, "-olallchecks");
		createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, NULL);
		assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->previous);
		mm1 = s_olcxCurrentUser->browserStack.top->previous->menuSym; 
		mm2 = s_olcxCurrentUser->browserStack.top->menuSym; 
		refactoryStaticMoveCheckCorrespondance(mm1, mm2, theMethod);
	}

//&editorDumpMarker(mstart);
//&editorDumpMarker(movedEnd);

	// reduce long names in the method
	pp = editorDuplicateMarker(mstart);
	ppp = editorDuplicateMarker(movedEnd);
	ED_ALLOC(lll, S_editorRegionList);
	FILLF_editorRegionList(lll, pp, ppp, regions);
	regions = lll;

	refactoryPerformReduceNamesAndAddImports(&regions, INTERACTIVE_NO);

}

static S_editorMarker *getTargetFromOptions() {
	S_editorMarker 	*target;
	S_editorBuffer 	*tb;
	int 			tline;
	tb = editorFindFileCreate(normalizeFileName(s_ropt.moveTargetFile, s_cwd));
	target = editorCrNewMarker(tb, 0);
	sscanf(s_ropt.refpar1, "%d", &tline);
	editorMoveMarkerToLineCol(target, tline, 0);
	return(target);
}

static void refactoryGetMethodLimitsForMoving(S_editorMarker *point, 
											  S_editorMarker **_mstart, 
											  S_editorMarker **_mend,
											  int limitIndex
	) {
	S_editorMarker *mstart, *mend;
	// get method limites
	refactoryMakeSyntaxPassOnSource(point);
	if (s_spp[limitIndex].file==s_noneFileIndex || s_spp[limitIndex+1].file==s_noneFileIndex) {
		fatalError(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
	}
	mstart = editorCrNewMarkerForPosition(&s_spp[limitIndex]);
	mend = editorCrNewMarkerForPosition(&s_spp[limitIndex+1]);
	refactoryMoveMarkerToTheBeginOfDefinitionScope(mstart);
	refactoryMoveMarkerToTheEndOfDefinitionScope(mend);
	assert(mstart->buffer == mend->buffer);
	*_mstart = mstart;
	*_mend = mend;
}

static void refactoryGetNameOfTheClassAndSuperClass(S_editorMarker *point, char *ccname, char *supercname) {
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, 
									point,NULL, "-olcxcurrentclass",NULL);
	if (ccname != NULL) {
		if (s_cps.currentClassAnswer[0] == 0) {
			fatalError(ERR_ST, "can't get class on point", XREF_EXIT_ERR);
		}
		strcpy(ccname, s_cps.currentClassAnswer);
		javaDotifyClassName(ccname);
	}
	if (supercname != NULL) {
		if (s_cps.currentSuperClassAnswer[0] == 0) {
			fatalError(ERR_ST, "can't get superclass of class on point", XREF_EXIT_ERR);
		}
		strcpy(supercname, s_cps.currentSuperClassAnswer);
		javaDotifyClassName(supercname);	
	}
}

// ---------------------------------------------------- MoveStaticMethod

static void refactoryMoveStaticFieldOrMethod(S_editorMarker *point, int limitIndex) {
	char 				targetFqtName[MAX_FILE_NAME_SIZE];
	int					lines;
	unsigned			accFlags;
	S_editorMarker	 	*target, *mstart, *mend;

	target = getTargetFromOptions();

	if (! validTargetPlace(target, "-olcxmmtarget")) return;
	refactoryUpdateReferences(s_ropt.project);
	refactoryGetNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
	refactoryGetMethodLimitsForMoving(point, &mstart, &mend, limitIndex);
	lines = editorCountLinesBetweenMarkers(mstart, mend);

	// O.K. Now STARTING!
	refactoryPerformMovingOfStaticObjectAndMakeItPublic(mstart, point, mend, target, 
														targetFqtName, 
														&accFlags, APPLY_CHECKS, limitIndex);
//&sprintf(tmpBuff,"original acc == %d", accFlags); ppcGenTmpBuff();
	refactoryRestrictAccessibility(point, limitIndex, accFlags);

	// and generate output
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
	ppcGenNumericRecord(PPC_INDENT, lines, "", "\n");
}


static void refactoryMoveStaticMethod(S_editorMarker *point) {
	refactoryMoveStaticFieldOrMethod(point, SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

static void refactoryMoveStaticField(S_editorMarker *point) {
	refactoryMoveStaticFieldOrMethod(point, SPP_FIELD_DECLARATION_BEGIN_POSITION);
}


// ---------------------------------------------------------- MoveField

static void refactoryMoveField(S_editorMarker *point) {
	char 				targetFqtName[MAX_FILE_NAME_SIZE];
	char 				nameOnPoint[TMP_STRING_SIZE];
	char				prefixDot[TMP_STRING_SIZE];
	int					lines, size, check, accessFlags;
	S_editorMarker	 	*target, *mstart, *mend, *movedEnd;
	S_editorMarkerList 	*occs, *noccs, *ll;
	S_editorMarker 		*pp, *ppp;
	S_editorRegionList	*regions, *lll;
	S_editorUndo 		*undoStartPoint, *redoTrack;
	int 				progressi, progressn;


	target = getTargetFromOptions();
	if (s_ropt.refpar2!=NULL && *s_ropt.refpar2!=0) {
		sprintf(prefixDot, "%s.", s_ropt.refpar2);
	} else {
		sprintf(prefixDot, "");
	}

	if (! validTargetPlace(target, "-olcxmmtarget")) return;
	refactoryUpdateReferences(s_ropt.project);
	refactoryGetNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
	refactoryGetMethodLimitsForMoving(point, &mstart, &mend, SPP_FIELD_DECLARATION_BEGIN_POSITION);
	lines = editorCountLinesBetweenMarkers(mstart, mend);

	// O.K. Now STARTING!
	movedEnd = editorDuplicateMarker(mend);
	movedEnd->offset --;

//&editorDumpMarker(mstart);
//&editorDumpMarker(movedEnd);

	size = mend->offset - mstart->offset;
	if (target->buffer == mstart->buffer 
		&& target->offset > mstart->offset
		&& target->offset < mstart->offset+size) {
		ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.", "\n");
		return;
	}

	// O.K. move
	refactoryApplyExpandShortNames(point->buffer, point);
	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
	occs = refactoryGetReferences(point->buffer, point,STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	accessFlags = s_olcxCurrentUser->browserStack.top->hkSelectedSym->s.b.accessFlags;
	
	undoStartPoint = s_editorUndo;
	LIST_MERGE_SORT(S_editorMarkerList, occs, editorMarkerListLess);
	LIST_LEN(progressn, S_editorMarkerList, occs); progressi=0;
	regions = NULL; 
	for(ll=occs; ll!=NULL; ll=ll->next) {
		if (! IS_DEFINITION_OR_DECL_USAGE(ll->usg.base)) {
			if (*prefixDot != 0) {
				refactoryReplaceString(ll->d, 0, prefixDot);
			}
		}
		writeRelativeProgress((progressi++)*100/progressn);
	}
	writeRelativeProgress(100);

	size = mend->offset - mstart->offset;
	editorMoveBlock(target, mstart, size, &s_editorUndo);

	// reduce long names in the method
	pp = editorDuplicateMarker(mstart);
	ppp = editorDuplicateMarker(movedEnd);
	ED_ALLOC(lll, S_editorRegionList);
	FILLF_editorRegionList(lll, pp, ppp, regions);
	regions = lll;

	refactoryPerformReduceNamesAndAddImports(&regions, INTERACTIVE_NO);

	refactoryChangeAccessModifier(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, "public");
	refactoryRestrictAccessibility(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, accessFlags);

	redoTrack = NULL;
	check = refactoryMakeSafetyCheckAndUndo(point->buffer, point, &occs, 
											undoStartPoint, &redoTrack);
	if (! check) {
		refactoryAskForReallyContinueConfirmation();
	}
	// and generate output
	editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);


	ppcGenGotoMarkerRecord(point);
	ppcGenNumericRecord(PPC_INDENT, lines, "", "\n");
}

// ---------------------------------------------------------- MoveClass

static void refactorySetMovingPrecheckStandardEnvironment(S_editorMarker *point, char *targetFqtName) {
	S_olSymbolsMenu *ss;
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, 
									point, NULL, "-olcxtrivialprecheck",NULL);
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	olCreateSelectionMenu(s_olcxCurrentUser->browserStack.top->command);
	s_opt.moveTargetFile = s_ropt.moveTargetFile;
	s_opt.moveTargetClass = targetFqtName;
	assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
	ss = s_olcxCurrentUser->browserStack.top->hkSelectedSym;
	assert(ss);
}

static void refactoryPerformMoveClass(S_editorMarker *point, S_editorMarker *target,
									  S_editorMarker **outstart, S_editorMarker **outend
	) {
	char					spack[MAX_FILE_NAME_SIZE];
	char					tpack[MAX_FILE_NAME_SIZE];
	char 					targetFqtName[MAX_FILE_NAME_SIZE];
	int						i, targetIsNestedInClass;
	S_editorMarker	 		*mm, *mstart, *mend;
	S_editorBuffer			*tb;
	S_olSymbolsMenu			*ss;
	S_tpCheckMoveClassData	dd;

	*outstart = *outend = NULL;
	// get target place
	refactoryEditServerParseBuffer( s_ropt.project, target->buffer,
								   target,NULL, "-olcxcurrentclass",NULL);
	if (s_cps.currentPackageAnswer[0] == 0) {
		error(ERR_ST, "Can't get target class or package");
		return;
	}
	if (s_cps.currentClassAnswer[0] == 0) {
		sprintf(targetFqtName, "%s", s_cps.currentPackageAnswer);
		targetIsNestedInClass = 0;
	} else {
		sprintf(targetFqtName, "%s", s_cps.currentClassAnswer);
		targetIsNestedInClass = 1;
	}
	javaDotifyClassName(targetFqtName);

	// get limites
	refactoryMakeSyntaxPassOnSource(point);
	if (s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file==s_noneFileIndex
		||s_spp[SPP_CLASS_DECLARATION_END_POSITION].file==s_noneFileIndex) {
		fatalError(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
	}
	mstart = editorCrNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION]);
	mend = editorCrNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_END_POSITION]);
	refactoryMoveMarkerToTheBeginOfDefinitionScope(mstart);
	refactoryMoveMarkerToTheEndOfDefinitionScope(mend);

	assert(mstart->buffer == mend->buffer);

	*outstart = mstart;
	// put outend -1 to be updated during moving
	*outend = editorCrNewMarker(mend->buffer, mend->offset-1);

	// prechecks
	refactorySetMovingPrecheckStandardEnvironment(point, targetFqtName);
	ss = s_olcxCurrentUser->browserStack.top->hkSelectedSym;
	tpCheckFillMoveClassData(&dd, spack, tpack);
	tpCheckSourceIsNotInnerClass();
	tpCheckMoveClassAccessibilities();

	// O.K. Now STARTING!
	refactoryPerformMovingOfStaticObjectAndMakeItPublic(mstart, point, mend, target, targetFqtName, NULL, NO_CHECKS, SPP_CLASS_DECLARATION_BEGIN_POSITION);

	// recover end marker
	(*outend)->offset++;

	// finally fiddle modifiers
	if (ss->s.b.accessFlags & ACC_STATIC) {
		if (! targetIsNestedInClass) {
			// nested -> top level
//&sprintf(tmpBuff,"removing modifier"); ppcGenTmpBuff();
			refactoryRemoveModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "static");
		}
	} else {
		if (targetIsNestedInClass) {
			// top level -> nested
			refactoryAddModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "static");
		}
	}
	if (dd.transPackageMove) {
		// add public
		refactoryChangeAccessModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "public");
	}
}

static void refactoryMoveClass(S_editorMarker *point) {
	S_editorMarker	 		*target, *mstart, *mend;
	int						linenum;

	target = getTargetFromOptions();
	if (! validTargetPlace(target, "-olcxmctarget")) return;

	refactoryUpdateReferences(s_ropt.project);

	refactoryPerformMoveClass(point, target, &mstart, &mend);
	linenum = editorCountLinesBetweenMarkers(mstart, mend);

	// and generate output
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
	ppcGenNumericRecord(PPC_INDENT, linenum, "", "\n");

	ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former class.", "\n");
}

static void refactoryGetPackageNameFromMarkerFileName(S_editorMarker *target, char *tclass) {
	char *dd;
	strcpy(tclass, javaCutSourcePathFromFileName(target->buffer->name));
	dd = lastOccurenceInString(tclass, '.');
	if (dd!=NULL) *dd=0;
	dd = lastOccurenceOfSlashOrAntiSlash(tclass);
	if (dd!=NULL) {
		*dd=0;
		javaDotifyFileName(tclass);
	} else {
		*tclass = 0;
	}
}


static void refactoryInsertPackageStatToNewFile(S_editorMarker *src, S_editorMarker *target) {
	char					tclass[MAX_FILE_NAME_SIZE];
	char					sclass[MAX_FILE_NAME_SIZE];
	char					pack[MAX_FILE_NAME_SIZE];
	char 					*dd, *sdd;
	refactoryGetPackageNameFromMarkerFileName(target, tclass);
	if (tclass[0] == 0) {
		sprintf(pack, "\n");
	} else {
		sprintf(pack, "package %s;\n", tclass);
	}
	sprintf(pack+strlen(pack), "\n\n");
#ifdef ZERO
	refactoryGetPackageNameFromMarkerFileName(src, sclass);
	// add default import, it is strong probable that will be used
	if (sclass[0] == 0) {
		sprintf(pack+strlen(pack), "\n");
	} else {
		sprintf(pack+strlen(pack), "import %s.*;\n\n\n", sclass);
	}
#endif
	refactoryReplaceString(target, 0, pack);
	target->offset --;
}


static void refactoryMoveClassToNewFile(S_editorMarker *point) {
	S_editorMarker	 		*target, *mstart, *mend, *npoint;
	S_editorBuffer			*buff, *buf;
	int						linenum;
	char					*dd;

	buff = point->buffer;
	refactoryUpdateReferences(s_ropt.project);
	target = getTargetFromOptions();

	// insert package statement
	refactoryInsertPackageStatToNewFile(point, target);

	refactoryPerformMoveClass(point, target, &mstart, &mend);

	if (mstart==NULL || mend==NULL) return;
//&editorDumpMarker(mstart);
//&editorDumpMarker(mend);
	linenum = editorCountLinesBetweenMarkers(mstart, mend);

	// and generate output
	refactoryAplyWholeRefactoringFromUndo();

	// indentation must be at the end (undo, redo does not work with)
	ppcGenGotoMarkerRecord(point);
	ppcGenNumericRecord(PPC_INDENT, linenum, "", "\n");

	// TODO check whether the original class was the only class in the file
	npoint = editorCrNewMarker(buff, 0);
	// just to parse the file
	refactoryEditServerParseBuffer(s_ropt.project, npoint->buffer, npoint,NULL, "-olcxpushspecialname=", NULL);
	if (s_spp[SPP_LAST_TOP_LEVEL_CLASS_POSITION].file == s_noneFileIndex) {
		ppcGenGotoMarkerRecord(npoint);
		ppcGenRecord(PPC_KILL_BUFFER_REMOVE_FILE, "This file does not contain classes anymore, can I remove it?", "\n");
	}
	ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former class.", "\n");
}

static void refactoryMoveAllClassesToNewFile(S_editorMarker *point) {
	// this should really copy whole file, including commentaries
	// between classes, etc... Then update all references

}

static void refactoryAddCopyOfMarkerToList(S_editorMarkerList **ll, S_editorMarker *mm, S_usageBits *usage) {
	S_editorMarker 			*nn;
	S_editorMarkerList 		*lll;
	nn = editorCrNewMarker(mm->buffer, mm->offset);
	ED_ALLOC(lll, S_editorMarkerList);
	FILL_editorMarkerList(lll, nn, *usage, *ll);
	*ll = lll;
}

// ------------------------------------------ TurnDynamicToStatic

static void refactoryTurnDynamicToStatic(S_editorMarker *point) {
	char				nameOnPoint[TMP_STRING_SIZE];
	char				primary[REFACTORING_TMP_STRING_SIZE];
	char				fqstaticname[REFACTORING_TMP_STRING_SIZE];
	char				fqthis[REFACTORING_TMP_STRING_SIZE];
	char				pardecl[REFACTORING_TMP_STRING_SIZE];
	char				parusage[REFACTORING_TMP_STRING_SIZE];
	char				cid[TMP_STRING_SIZE];
	int					plen, ppoffset, poffset;
	int 				progressi, progressj, progressn;
	S_editorMarker		*pp, *ppp, *nparamdefpos;
	S_editorMarkerList	*ll, *markers, *occs, *allrefs;
	S_editorMarkerList	*npoccs, *npadded, *diff1, *diff2;
	S_editorRegionList	*regions, **reglast, *lll;
	S_olSymbolsMenu		*csym, *mm;
	S_editorUndo		*undoStartPoint;
	S_usageBits			defusage;

	nparamdefpos = NULL;
	refactoryUpdateReferences(s_ropt.project);
	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
	occs = refactoryPushGetAndPreCheckReferences(point->buffer, point, nameOnPoint, 
												 "If  you see  this  message it  is  highly probable  that turning  this virtual  method into  static will  not be  behaviour  preserving! This refactoring is  behaviour preserving only  if the method does  not use mechanism of virtual invocations. In this dialog you should select the application classes which are refering to the method which will become static. If  you can't unambiguously determine those  references do not continue in this refactoring!",
												 PPCV_BROWSER_TYPE_WARNING);
	editorFreeMarkersAndMarkerList(occs);

	if (! tpCheckOuterScopeUsagesForDynToSt()) return;
	if (! tpCheckSuperMethodReferencesForDynToSt()) return;

	// Pass over all references and move primary prefix to first parameter
	// also insert new first parameter on definition
	csym =  s_olcxCurrentUser->browserStack.top->hkSelectedSym;
	javaGetClassNameFromFileNum(csym->s.vFunClass, fqstaticname, DOTIFY_NAME);
	javaDotifyClassName(fqstaticname);
	sprintf(pardecl, "%s %s", fqstaticname, s_ropt.refpar1);
	sprintf(fqstaticname+strlen(fqstaticname), ".");

	progressn = progressi = 0;
	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			mm->markers = editorReferencesToMarkers(mm->s.refs, filter0, NULL);
			LIST_MERGE_SORT(S_editorMarkerList, mm->markers, editorMarkerListLess);
			LIST_LEN(progressj, S_editorMarkerList, mm->markers); 
			progressn += progressj;
		}
	}
	undoStartPoint = s_editorUndo;
	regions = NULL; reglast = &regions; allrefs = NULL;
	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			javaGetClassNameFromFileNum(mm->s.vApplClass, fqthis, DOTIFY_NAME);
			javaDotifyClassName(fqthis);
			sprintf(fqthis+strlen(fqthis), ".this");
			for(ll=mm->markers; ll!=NULL; ll=ll->next) {
				refactoryAddCopyOfMarkerToList(&allrefs, ll->d, &ll->usg);
				pp = NULL;
				if (IS_DEFINITION_OR_DECL_USAGE(ll->usg.base)) {
					pp = editorCrNewMarker(ll->d->buffer, ll->d->offset);
					pp->offset = refactoryAddStringAsParameter(ll->d, ll->d, nameOnPoint, 
															   1, pardecl);
					// remember definition position of new parameter
					nparamdefpos = editorCrNewMarker(pp->buffer, pp->offset+strlen(pardecl)-strlen(s_ropt.refpar1));
				} else {
					pp = refactoryCrNewMarkerForExpressionBegin(ll->d, GET_PRIMARY_START);
					assert(pp!=NULL);
					ppoffset = pp->offset;
					plen = ll->d->offset - pp->offset;
					strncpy(primary, MARKER_TO_POINTER(pp), plen);
					// delete pending dot
					while (plen>0 && primary[plen-1]!='.') plen--;
					if (plen>0) {
						primary[plen-1] = 0;
					} else {
						if (javaLinkNameIsAnnonymousClass(mm->s.name)) {
							strcpy(primary, "this");
						} else {
							strcpy(primary, fqthis);
						}
					}
					refactoryReplaceString(pp, ll->d->offset-pp->offset, fqstaticname);
					refactoryAddStringAsParameter(ll->d, ll->d, nameOnPoint, 1, primary);
					// return offset back to beginning of fqt
					pp->offset = ppoffset;
				}
				ppp = editorCrNewMarker(ll->d->buffer, ll->d->offset);
				editorMoveMarkerBeyondIdentifier(ppp, 1);
				ED_ALLOC(lll, S_editorRegionList);
				FILLF_editorRegionList(lll, pp, ppp, NULL);
				*reglast = lll;
				reglast = &lll->next;
				writeRelativeProgress((progressi++)*100/progressn);
			}
			editorFreeMarkersAndMarkerList(mm->markers);
			mm->markers = NULL;
		}
	}
	writeRelativeProgress(100);
	
	// pop references
	s_olcxCurrentUser->browserStack.top = s_olcxCurrentUser->browserStack.top->previous;

	sprintf(parusage, "%s.", s_ropt.refpar1);

	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point,NULL, "-olcxmaybethis",NULL);
	olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);

	progressn = progressi = 0;
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			mm->markers = editorReferencesToMarkers(mm->s.refs, filter0, NULL);
			LIST_MERGE_SORT(S_editorMarkerList, mm->markers, editorMarkerListLess);
			LIST_LEN(progressj, S_editorMarkerList, mm->markers); 
			progressn += progressj;
		}
	}
	
	// passing references inside method and change them to the new parameter
	npadded = NULL;
	FILL_usageBits(&defusage, UsageDefined, 0, 0);
	refactoryAddCopyOfMarkerToList(&npadded, nparamdefpos, &defusage);

	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			for(ll=mm->markers; ll!=NULL; ll=ll->next) {
				if (ll->usg.base == UsageMaybeQualifThisInClassOrMethod) {
					editorUndoUntil(undoStartPoint,NULL);
					ppcGenGotoMarkerRecord(ll->d);
					error(ERR_ST, "The method is using qualified this to access enclosed instance. Do not know how to make it static.");
					return;
				} else if (ll->usg.base == UsageMaybeThisInClassOrMethod) {
					strncpy(cid, refactoryGetIdentifierOnMarker_st(ll->d), TMP_STRING_SIZE);
					cid[TMP_STRING_SIZE-1]=0;
					poffset = ll->d->offset;
//&sprintf(tmpBuff, "Checking %s", cid); ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
					if (strcmp(cid, "this")==0 || strcmp(cid, "super")==0) {
						pp = refactoryReplaceStaticPrefix(ll->d, "");
						poffset = pp->offset;
						editorFreeMarker(pp);
						refactoryCheckedReplaceString(ll->d, 4, cid, s_ropt.refpar1);
					} else {
						refactoryReplaceString(ll->d, 0, parusage);
					}
					ll->d->offset = poffset;
					refactoryAddCopyOfMarkerToList(&npadded, ll->d, &ll->usg);
				}
				writeRelativeProgress((progressi++)*100/progressn);
			}
		}
	}
	writeRelativeProgress(100);

	refactoryAddModifier(point, SPP_METHOD_DECLARATION_BEGIN_POSITION, "static");

	// reduce long names at the end because of recursive calls
	refactoryPerformReduceNamesAndAddImports(&regions, INTERACTIVE_NO);
	editorFreeMarkersAndRegionList(regions); regions=NULL;

	// safety check checking that new parameter has exactly
	// those references as expected (not hidden by a local variable and no 
	// occurence of extra variable is resolved to parameter)
	npoccs = refactoryGetReferences(
		nparamdefpos->buffer, nparamdefpos, 
		"Internal problem, during new parameter resolution",
		PPCV_BROWSER_TYPE_WARNING);
	editorMarkersDifferences(&npoccs, &npadded, &diff1, &diff2);
	LIST_APPEND(S_editorMarkerList, diff1, diff2); diff2=NULL;
	if (diff1!=NULL) {
		ppcGenGotoMarkerRecord(point);
		refactoryShowSafetyCheckFailingDialog( &diff1, "The new parameter conflicts with existing symbols");
	}

	if (npoccs != NULL && npoccs->next == NULL) {
		// only one occurence, this must be the definition
		// but check it for being sure
		// maybe you should update references and delete the parameter
		// after, but for now, use computed references, it should work.
		if (IS_DEFINITION_USAGE(npoccs->usg.base)) {
			refactoryApplyParamManip(nameOnPoint, allrefs, PPC_AVR_DEL_PARAMETER, 1, 1);
			//& refactoryDeleteParameter(point, nameOnPoint, 1, UsageDefined);
		}
		
	}

	// TODO!!! add safety checks, as changing the profile of the method
	// can introduce new conflicts

	editorFreeMarkersAndMarkerList(allrefs); allrefs=NULL;

	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);
}

static int noSpaceChar(int c) {return(! isspace(c));}

static void dumpRefernces(S_reference *rr) {
	S_reference *r;
	fprintf(dumpOut, "[dump]");
	for(r=rr; r!=NULL; r=r->next) {
		fprintf(dumpOut,"%s:%d:%d\n", s_fileTab.tab[r->p.file]->name, r->p.line, r->p.coll);
	}
	fprintf(dumpOut, "\n\n");
}

static void refactoryPushMethodSymbolsPlusThoseWithClearedRegion(
	S_editorMarker *m1, S_editorMarker *m2
	) {
	char				spaces[REFACTORING_TMP_STRING_SIZE];
	S_editorUndo *undoMark;
	int slen;
	assert(m1->buffer == m2->buffer);
	undoMark = s_editorUndo;
	refactoryPushAllReferencesOfMethod(m1,NULL);
	slen = m2->offset-m1->offset;
	assert(slen>=0 && slen<REFACTORING_TMP_STRING_SIZE);
	memset(spaces, ' ', slen);
	spaces[slen]=0;
	refactoryReplaceString(m1, slen, spaces);
	refactoryPushAllReferencesOfMethod(m1,NULL);
	editorUndoUntil(undoMark,NULL);
}

static int refactoryIsMethodPartRedundant(
	S_editorMarker *m1, S_editorMarker *m2
	) {
	S_olSymbolsMenu 	*mm1, *mm2;
	S_editorUndo		*undoMark;
	S_reference			*diff;
	S_editorMarkerList	*lll, *ll;
	int					slen, res;

	refactoryPushMethodSymbolsPlusThoseWithClearedRegion(m1, m2);
	res = 1;
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->previous);
	mm1 = s_olcxCurrentUser->browserStack.top->menuSym; 
	mm2 = s_olcxCurrentUser->browserStack.top->previous->menuSym; 
	while (mm1!=NULL && mm2!=NULL && res!=0) {
//&symbolRefItemDump(&mm1->s); dumpRefernces(mm1->s.refs);
//&symbolRefItemDump(&mm2->s); dumpRefernces(mm2->s.refs);
		olcxReferencesDiff(&mm1->s.refs, &mm2->s.refs, &diff);
		if (diff!=NULL) {
			lll = editorReferencesToMarkers(diff, filter0, NULL);
			LIST_MERGE_SORT(S_editorMarkerList, lll, editorMarkerListLess);
			for (ll=lll; ll!=NULL; ll=ll->next) {
				assert(ll->d->buffer == m1->buffer);
//&sprintf(tmpBuff, "checking diff %d", ll->d->offset); ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
				if (editorMarkerLess(ll->d, m1) || editorMarkerLessOrEq(m2, ll->d)) {
					res=0;
				}
			}
			editorFreeMarkersAndMarkerList(lll);
			olcxFreeReferences(diff);
		}
		mm1 = mm1->next;
		mm2 = mm2->next;
	}
	olcxPopOnly();
	olcxPopOnly();

	return(res);
}

static void refactoryRemoveMethodPartIfRedundant(S_editorMarker *m, int len) {
	S_editorMarker *mm;
	mm = editorCrNewMarker(m->buffer, m->offset+len);
	if (refactoryIsMethodPartRedundant(m, mm)) {
		refactoryReplaceString(m, len, "");
	}
	editorFreeMarker(mm);
}

static int isMethodBeg(int c) {return(c=='{');}

static int staticToDynCanBeThisOccurence(S_editorMarker *pp, char *param, int *rlen) {
	char 			*pp2;
	S_editorMarker 	*mm;
	int 			res = 0;
	mm = editorCrNewMarker(pp->buffer, pp->offset);
	pp2 = strchr(param, '.');
	if (pp2==NULL) {
		*rlen = strlen(param);
		res = strcmp(refactoryGetIdentifierOnMarker_st(pp), param)==0;
		goto fini;
	}
	// param.field so parse it
	if (strncmp(refactoryGetIdentifierOnMarker_st(mm), param, pp2-param)!=0) goto fini;
	mm->offset += (pp2-param);
	editorMoveMarkerToNonBlank(mm, 1);
	if (*(MARKER_TO_POINTER(mm)) != '.') goto fini;
	mm->offset ++;
	editorMoveMarkerToNonBlank(mm, 1);
	if (strcmp(refactoryGetIdentifierOnMarker_st(mm), pp2+1)!=0) goto fini;
	*rlen = mm->offset - pp->offset + strlen(pp2+1);
	res = 1;
 fini:
	editorFreeMarker(mm);
	return(res);
}

// ----------------------------------------------- TurnStaticToDynamic

static void refactoryTurnStaticToDynamic(S_editorMarker *point) {
	char				nameOnPoint[TMP_STRING_SIZE];
	char				param[REFACTORING_TMP_STRING_SIZE];
	char				tparam[REFACTORING_TMP_STRING_SIZE];
	char				testi[REFACTORING_TMP_STRING_SIZE];
	int					plen, tplen, rlen, res, argn, bi;
	int					classnum, parclassnum;
	int 				progressi, progressn;
	S_editorMarker		*mm, *m1, *m2, *pp;
	S_editorMarkerList	*ll, *occs, *poccs;
	S_editorUndo		*checkPoint;

	refactoryUpdateReferences(s_ropt.project);

	argn = 0;
	sscanf(s_ropt.refpar1, "%d", &argn);

	assert(argn!=0);

	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	res = refactoryGetParamNamePosition(point, nameOnPoint, argn);
	if (res != RETURN_OK) {
		ppcGenGotoMarkerRecord(point);
		error(ERR_INTERNAL, "Can't determine position of parameter");
		return;
	}
	mm = editorCrNewMarkerForPosition(&s_paramPosition);
	if (s_ropt.refpar2[0]!=0) {
		sprintf(param, "%s.%s", refactoryGetIdentifierOnMarker_st(mm), s_ropt.refpar2);
	} else {
		sprintf(param, "%s", refactoryGetIdentifierOnMarker_st(mm));
	}
	plen = strlen(param);

	// TODO!!! precheck
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer,
								   point,NULL, "-olcxcurrentclass",NULL);
	if (s_cps.currentClassAnswer[0] == 0) {
		error(ERR_INTERNAL, "Can't get current class");
		return;
	}
	classnum = getClassNumFromClassLinkName(s_cps.currentClassAnswer, s_noneFileIndex);
	if (classnum==s_noneFileIndex) {
		error(ERR_INTERNAL, "Problem when getting current class");
		return;
	}
	
	checkPoint = s_editorUndo;
	pp = editorCrNewMarker(point->buffer, point->offset);
	res = editorRunWithMarkerUntil(pp, isMethodBeg, 1);
	if (! res) {
		error(ERR_INTERNAL, "Can't find beginning of method");
		return;
	}
	pp->offset ++;
	sprintf(testi, "xxx(%s)", param);
	bi = pp->offset + 3 + plen;
	editorReplaceString(pp->buffer, pp->offset, 0, testi, strlen(testi), &s_editorUndo);
	pp->offset = bi;
	refactoryEditServerParseBuffer( s_ropt.project, pp->buffer,
								   pp,NULL, "-olcxgetsymboltype","-noerrors");
	// -noerrors is basically very dangerous in this context, recover it in s_opt
	s_opt.noErrors = 0;
	if (! s_olstringServed) {
		error(ERR_ST, "Can't infer type for parameter/field");
		return;
	}
	parclassnum = getClassNumFromClassLinkName(s_olSymbolClassType, s_noneFileIndex);
	if (parclassnum==s_noneFileIndex) {
		error(ERR_INTERNAL, "Problem when getting parameter/field class");
		return;
	}
	if (! isSmallerOrEqClass(parclassnum, classnum)) {
		error(ERR_ST, "Type of parameter.field must be current class or its subclass");
		return;
	}

	editorUndoUntil(checkPoint, NULL);
	editorFreeMarker(pp);

	// O.K. turn it virtual

	// STEP 1) inspect all references and copy the parameter to application object
	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
	occs = refactoryPushGetAndPreCheckReferences(point->buffer, point, nameOnPoint,STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);

	LIST_LEN(progressn, S_editorMarkerList, occs); progressi=0;
	for(ll=occs; ll!=NULL; ll=ll->next) {
		if (! IS_DEFINITION_OR_DECL_USAGE(ll->usg.base)) {
			res = refactoryGetParamPosition(ll->d, nameOnPoint, argn);
			if (res == RETURN_OK) {
				m1 = editorCrNewMarkerForPosition(&s_paramBeginPosition);
				m1->offset ++;
				editorRunWithMarkerUntil(m1, noSpaceChar, 1);
				m2 = editorCrNewMarkerForPosition(&s_paramEndPosition);
				m2->offset --;
				editorRunWithMarkerUntil(m2, noSpaceChar, -1);
				m2->offset ++;
				tplen = m2->offset - m1->offset;
				assert(tplen < REFACTORING_TMP_STRING_SIZE-1);
				strncpy(tparam, MARKER_TO_POINTER(m1), tplen);
				tparam[tplen] = 0;
				if (strcmp(tparam, "this")!=0) {
					if (s_ropt.refpar2[0]!=0) {
						sprintf(tparam+strlen(tparam), ".%s", s_ropt.refpar2);
					}
					pp = refactoryReplaceStaticPrefix(ll->d, tparam);
					editorFreeMarker(pp);
				}
				editorFreeMarker(m2);
				editorFreeMarker(m1);
			}
		}
		writeRelativeProgress((progressi++)*100/progressn);
	}
	writeRelativeProgress(100);
	// you can remove 'static' now, hope it is not virtual symbol,
	refactoryRemoveModifier(point, SPP_METHOD_DECLARATION_BEGIN_POSITION, "static");

	// TODO verify that new profile does not make clash


	// STEP 2) inspect all usages of parameter and replace them by 'this', 
	// remove this this if useless
	poccs = refactoryGetReferences(
		mm->buffer, mm, 
		STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO
		);
	for(ll=poccs; ll!=NULL; ll=ll->next) {
		if (! IS_DEFINITION_OR_DECL_USAGE(ll->usg.base)) {
			if (ll->d->offset+plen <= ll->d->buffer->a.bufferSize
				// TODO! do this at least little bit better, by skipping spaces, etc.
				&& staticToDynCanBeThisOccurence(ll->d, param, &rlen)) {
				refactoryReplaceString(ll->d, rlen, "this");
				refactoryRemoveMethodPartIfRedundant(ll->d, strlen("this."));
			}
		}
	}

	// STEP3) remove the parameter if not used anymore
	if (! refactoryIsThisSymbolUsed(mm)) {
		refactoryApplyParameterManipulation(point->buffer, point, PPC_AVR_DEL_PARAMETER, argn, 0);
	} else {
		// at least update the progress
		writeRelativeProgress(100);
	}

	// and generate output
	refactoryAplyWholeRefactoringFromUndo();
	ppcGenGotoMarkerRecord(point);

	// DONE!
}


// ------------------------------------------------------ ExtractMethod

static void refactoryExtractMethod(S_editorMarker *point, S_editorMarker *mark) {
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point, mark, 
									"-olcxextract", NULL);
}

static void refactoryExtractMacro(S_editorMarker *point, S_editorMarker *mark) {
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point, mark, 
									"-olcxextract", "-olexmacro");
}

// ------------------------------------------------------- Encapsulate

static S_reference *refactoryCheckEncapsulateGetterSetterForExistingMethods(char *mname) {
	S_olSymbolsMenu 	*mm, *hk;
	char 				clist[REFACTORING_TMP_STRING_SIZE];
	char 				cn[TMP_STRING_SIZE];
	S_reference			*rr, *anotherDefinition;
	clist[0] = 0;
	assert(s_olcxCurrentUser->browserStack.top);
	assert(s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	assert(s_olcxCurrentUser->browserStack.top->menuSym);
	anotherDefinition = NULL;
	hk = s_olcxCurrentUser->browserStack.top->hkSelectedSym;
	for(mm=s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (itIsSameCxSymbol(&mm->s, &hk->s) && mm->defRefn != 0) {
			if (mm->s.vFunClass==hk->s.vFunClass) {
				// find definition of another function
				for(rr=mm->s.refs; rr!=NULL; rr=rr->next) {
					if (IS_DEFINITION_USAGE(rr->usg.base)) {
						if (POSITION_NEQ(rr->p, hk->defpos)) {
							anotherDefinition = rr;
							goto refbreak;
						}
					}
				}
			refbreak:;
			} else {
				if (isSmallerOrEqClass(mm->s.vFunClass, hk->s.vFunClass)
					|| isSmallerOrEqClass(hk->s.vFunClass, mm->s.vFunClass)) {
					linkNamePrettyPrint(cn, 
										getShortClassNameFromClassNum_st(mm->s.vFunClass),
										TMP_STRING_SIZE,
										SHORT_NAME);
					if (substringIndex(clist, cn) == -1) {
						sprintf(clist+strlen(clist), " %s", cn);
					}
				}
			}
		}
	}
	// O.K. now I have list of classes in clist
	if (clist[0] != 0) {
		sprintf(tmpBuff,
				"The method %s is also defined in following related classes: %s. Its definition in  current class may  (under some  circumstance) change  your program behaviour. Do you really  want to continue in this refactoring?", mname, clist);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff, "\n");
	}
	if (anotherDefinition!=NULL) {
		sprintf(tmpBuff, "The method %s is yet defined in this class. C-xrefactory will not generate new method. Continue anyway?", mname);
		formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
		ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff, "\n");
	}
	return(anotherDefinition);
}

static void refactoryAddMethodToForbiddenRegions(S_reference *methodRef, 
												 S_editorRegionList **forbiddenRegions
	) {
	S_editorMarker *mm, *mb, *me;
	S_editorRegionList *reg;

	mm = editorCrNewMarkerForPosition(&methodRef->p);
	refactoryMakeSyntaxPassOnSource(mm);
	mb = editorCrNewMarkerForPosition(&s_spp[SPP_METHOD_DECLARATION_BEGIN_POSITION]);
	me = editorCrNewMarkerForPosition(&s_spp[SPP_METHOD_DECLARATION_END_POSITION]);
	ED_ALLOC(reg, S_editorRegionList);
	FILLF_editorRegionList(reg, mb, me, *forbiddenRegions);
	*forbiddenRegions = reg;
	editorFreeMarker(mm);
}

static void refactoryPerformEncapsulateField(S_editorMarker *point, 
											 S_editorRegionList **forbiddenRegions
	) {
	char				nameOnPoint[TMP_STRING_SIZE];
	char				upcasedName[TMP_STRING_SIZE];
	char				getter[TMP_STRING_SIZE];
	char				setter[TMP_STRING_SIZE];
	char				cclass[TMP_STRING_SIZE];
	char				getterBody[REFACTORING_TMP_STRING_SIZE];
	char				setterBody[REFACTORING_TMP_STRING_SIZE];
	char				declarator[REFACTORING_TMP_STRING_SIZE];
	char				*scclass;
	int					nameOnPointLen, declLen, indlines, indoffset;
	S_reference			*anotherGetter, *anotherSetter;
	unsigned			accFlags;
	S_editorMarkerList	*ll, *occs, *insiders, *outsiders;
	S_editorMarker		*mm, *eqm, *ee, *db, *dte, *dtb, *de, *mb, *me;
	S_editorMarker		*getterm, *setterm, *tbeg, *tend;
	S_editorUndo		*beforeInsertionUndo;
	S_editorRegionList	*reg;

	strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
	nameOnPointLen = strlen(nameOnPoint);
	assert(nameOnPointLen < TMP_STRING_SIZE-1);
	occs = refactoryPushGetAndPreCheckReferences(point->buffer, point, nameOnPoint, 
												 ERROR_SELECT_SYMBOLS_MESSAGE,
												 PPCV_BROWSER_TYPE_WARNING);
	for(ll=occs; ll!=NULL; ll=ll->next) {
		if (ll->usg.base == UsageAddrUsed) {
			ppcGenGotoMarkerRecord(ll->d);
			sprintf(tmpBuff, "There is a combined l-value reference of the field. Current version of C-xrefactory doesn't  know how  to encapsulate such  assignment. Please, turn it into simple assignment (i.e. field = field 'op' ...;) first.");
			formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
			error(ERR_ST, tmpBuff);
			return;
		}
	}

	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->hkSelectedSym);
	accFlags = s_olcxCurrentUser->browserStack.top->hkSelectedSym->s.b.accessFlags;

	cclass[0] = 0; scclass = cclass;
	if (accFlags&ACC_STATIC) {
		refactoryGetNameOfTheClassAndSuperClass(point, cclass, NULL);
		scclass = lastOccurenceInString(cclass, '.');
		if (scclass == NULL) scclass = cclass;
		else scclass ++;
	}

	strcpy(upcasedName, nameOnPoint);
	upcasedName[0] = toupper(upcasedName[0]);
	sprintf(getter, "get%s", upcasedName);
	sprintf(setter, "set%s", upcasedName);

	// generate getter and setter bodies
	refactoryMakeSyntaxPassOnSource(point);
	db = editorCrNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_BEGIN_POSITION]);
	dtb = editorCrNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION]);
	dte = editorCrNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_TYPE_END_POSITION]);
	de = editorCrNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_END_POSITION]);
	refactoryMoveMarkerToTheEndOfDefinitionScope(de);
	assert(dtb->buffer == dte->buffer);
	assert(dtb->offset <= dte->offset);
	declLen = dte->offset - dtb->offset;
	strncpy(declarator, MARKER_TO_POINTER(dtb), declLen);
	declarator[declLen] = 0;

	sprintf(getterBody, "public %s%s %s() {\nreturn(%s);\n}\n", 
			((accFlags&ACC_STATIC)?"static ":""),
			declarator, getter, nameOnPoint);
	sprintf(setterBody, "public %s%s %s(%s %s) {\n%s.%s = %s;\nreturn(%s);\n}\n", 
			((accFlags&ACC_STATIC)?"static ":""),
			declarator, setter, declarator, nameOnPoint,
			((accFlags&ACC_STATIC)?scclass:"this"), 
			nameOnPoint, nameOnPoint, nameOnPoint);

	beforeInsertionUndo = s_editorUndo;
	if (CHAR_BEFORE_MARKER(de) != '\n') refactoryReplaceString(de, 0, "\n");
	tbeg = editorDuplicateMarker(de);
	tbeg->offset --;

	getterm = setterm = NULL;
	getterm = editorCrNewMarker(de->buffer, de->offset-1);
	refactoryReplaceString(de, 0, getterBody);
	getterm->offset += substringIndex(getterBody, getter)+1;

	if ((accFlags & ACC_FINAL) ==  0) {
		setterm = editorCrNewMarker(de->buffer, de->offset-1);
		refactoryReplaceString(de, 0, setterBody);
		setterm->offset += substringIndex(setterBody, setter)+1;
	}
	tbeg->offset ++;
	tend = editorDuplicateMarker(de);

	// check if not yet defined or used
	anotherGetter = anotherSetter = NULL;
	refactoryPushReferences(getterm->buffer, getterm, "-olcxrename", NULL, 0);
	anotherGetter = refactoryCheckEncapsulateGetterSetterForExistingMethods(getter);
	editorFreeMarker(getterm);
	if ((accFlags & ACC_FINAL) ==  0) {
		refactoryPushReferences(setterm->buffer, setterm, "-olcxrename", NULL, 0);
		anotherSetter = refactoryCheckEncapsulateGetterSetterForExistingMethods(setter);
		editorFreeMarker(setterm);
	}
	if (anotherGetter!=NULL || anotherSetter!=NULL) {
		if (anotherGetter!=NULL) {
			refactoryAddMethodToForbiddenRegions(anotherGetter, forbiddenRegions);
		}
		if (anotherSetter!=NULL) {
			refactoryAddMethodToForbiddenRegions(anotherSetter, forbiddenRegions);
		}
		editorUndoUntil(beforeInsertionUndo, &s_editorUndo);
		de->offset = tbeg->offset;
		if (CHAR_BEFORE_MARKER(de) != '\n') refactoryReplaceString(de, 0, "\n");
		tbeg->offset --;
		if (! anotherGetter) {
			refactoryReplaceString(de, 0, getterBody);
		}
		if ((accFlags & ACC_FINAL) ==  0 && ! anotherSetter) {
			refactoryReplaceString(de, 0, setterBody);
		}
		tend->offset = de->offset;
		tbeg->offset ++;
	}
	// do not move this before, as anotherdef reference would be freed!
	if ((accFlags & ACC_FINAL) ==  0) olcxPopOnly();
	olcxPopOnly();

	// generate getter and setter invocations
	editorSplitMarkersWithRespectToRegions(&occs, forbiddenRegions, &insiders, &outsiders);
	for(ll=outsiders; ll!=NULL; ll=ll->next) {
		if (ll->usg.base == UsageLvalUsed) {
			refactoryMakeSyntaxPassOnSource(ll->d);
			if (s_spp[SPP_ASSIGNMENT_OPERATOR_POSITION].file == s_noneFileIndex) {
				error(ERR_INTERNAL, "Can't get assignment coordinates");
			} else {
				eqm = editorCrNewMarkerForPosition(&s_spp[SPP_ASSIGNMENT_OPERATOR_POSITION]);
				ee = editorCrNewMarkerForPosition(&s_spp[SPP_ASSIGNMENT_END_POSITION]);
				// make it in two steps to move the ll->d marker to the end
				refactoryCheckedReplaceString(ll->d, nameOnPointLen, nameOnPoint, "");
				refactoryReplaceString(ll->d, 0, setter);
				refactoryReplaceString(ll->d, 0, "(");
				editorRemoveBlanks(ll->d, 1, &s_editorUndo);
				refactoryCheckedReplaceString(eqm, 1, "=", "");
				editorRemoveBlanks(eqm, 0, &s_editorUndo);
				refactoryReplaceString(ee, 0, ")");
				ee->offset --;
				editorRemoveBlanks(ee, -1, &s_editorUndo);
				editorFreeMarker(eqm);
				editorFreeMarker(ee);
			}
		} else if (! IS_DEFINITION_OR_DECL_USAGE(ll->usg.base)) {
			refactoryCheckedReplaceString(ll->d, nameOnPointLen, nameOnPoint, "");
			refactoryReplaceString(ll->d, 0, getter);
			refactoryReplaceString(ll->d, 0, "()");
		}
	}

	refactoryRestrictAccessibility(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, ACC_PRIVATE);

	indoffset = tbeg->offset;
	indlines = editorCountLinesBetweenMarkers(tbeg, tend);

	// and generate output
	refactoryAplyWholeRefactoringFromUndo();

	// put it here, undo-redo sometimes shifts markers
	de->offset = indoffset;
	ppcGenGotoMarkerRecord(de);
	ppcGenNumericRecord(PPC_INDENT, indlines, "", "\n");

	ppcGenGotoMarkerRecord(point);	
}

static void refactorySelfEncapsulateField(S_editorMarker *point) {
	S_editorRegionList	*forbiddenRegions;
	forbiddenRegions = NULL;
	refactoryUpdateReferences(s_ropt.project);
	refactoryPerformEncapsulateField(point, &forbiddenRegions);
}

static void refactoryEncapsulateField(S_editorMarker *point) {
	S_editorRegionList	*forbiddenRegions;
	S_editorMarker		*cb, *ce;

	refactoryUpdateReferences(s_ropt.project);

//&editorDumpMarker(point);
	refactoryMakeSyntaxPassOnSource(point);
//&editorDumpMarker(point);
	if (s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == s_noneFileIndex
		|| s_spp[SPP_CLASS_DECLARATION_END_POSITION].file == s_noneFileIndex) {
		fatalError(ERR_INTERNAL, "can't deetrmine class coordinates", XREF_EXIT_ERR);
	}

	cb = editorCrNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION]);
	ce = editorCrNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_END_POSITION]);

	ED_ALLOC(forbiddenRegions, S_editorRegionList);
	FILLF_editorRegionList(forbiddenRegions, cb, ce, NULL);

//&editorDumpMarker(point);
	refactoryPerformEncapsulateField(point, &forbiddenRegions);
}

// -------------------------------------------------- pulling-up/pushing-down


static S_olSymbolsMenu *refactoryFindSymbolCorrespondingToReferenceWrtPullUpPushDown(
	S_olSymbolsMenu *menu2, S_olSymbolsMenu *mm1, S_editorMarkerList *rr1
	) {
	S_olSymbolsMenu *mm2;
	S_editorMarkerList *rr2;
	// find corresponding reference
	for(mm2=menu2; mm2!=NULL; mm2=mm2->next) {
		if (mm1->s.b.symType!=mm2->s.b.symType && mm2->s.b.symType!=TypeInducedError) continue;
		for(rr2=mm2->markers; rr2!=NULL; rr2=rr2->next) {
			if (MARKER_EQ(rr1->d, rr2->d)) goto breakrr2;
		}
	breakrr2:
		// check if symbols corresponds
		if (rr2!=NULL && symbolsCorrespondWrtMoving(mm1, mm2, OLO_PP_PRE_CHECK)) {
			goto breakmm2;;
		}
//&fprintf(dumpOut, "Checking %s\n", mm2->s.name);
	}
 breakmm2:
	return(mm2);
}

static int refactoryIsMethodPartRedundantWrtPullUpPushDown(
	S_editorMarker *m1, S_editorMarker *m2
	) {
	S_olSymbolsMenu 	*mm1, *mm2;
	S_editorUndo		*undoMark;
	S_reference			*diff;
	S_editorMarkerList	*rr1;
	S_editorMarker		*mmin, *mmax;
	int					slen, res;
	S_editorRegionList	*regions, *reg;
	S_editorBuffer		*buf;

	assert(m1->buffer == m2->buffer);

	regions = NULL;
	buf = m1->buffer;
	ED_ALLOC(reg, S_editorRegionList);
	FILLF_editorRegionList(reg, 
						   editorCrMarkerForBufferBegin(buf),
						   editorDuplicateMarker(m1),
						   regions);
	regions = reg;
	ED_ALLOC(reg, S_editorRegionList);
	FILLF_editorRegionList(reg, 
						   editorDuplicateMarker(m2),
						   editorCrMarkerForBufferEnd(buf), 
						   regions);
	regions = reg;

	refactoryPushMethodSymbolsPlusThoseWithClearedRegion(m1, m2);
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->previous);
	mm1 = s_olcxCurrentUser->browserStack.top->menuSym; 
	mm2 = s_olcxCurrentUser->browserStack.top->previous->menuSym; 
	createMarkersForAllReferencesInRegions(mm1, &regions);
	createMarkersForAllReferencesInRegions(mm2, &regions);

	res = 1;
	while (mm1!=NULL) {
		for(rr1=mm1->markers; rr1!=NULL; rr1=rr1->next) {
			if (refactoryFindSymbolCorrespondingToReferenceWrtPullUpPushDown(mm2, mm1, rr1)==NULL) {
				res = 0;
				goto fini;
			}
		}
		mm1=mm1->next;
	}
 fini:

	editorFreeMarkersAndRegionList(regions);
	olcxPopOnly();
	olcxPopOnly();
	return(res);
}

static S_editorMarkerList *refactoryPullUpPushDownDifferences(
	S_olSymbolsMenu *menu1, S_olSymbolsMenu *menu2, S_symbolRefItem *theMethod
	) {
	S_olSymbolsMenu *mm1, *mm2;
	S_editorMarkerList *rr, *rr1, *rr2, *diff;
	diff = NULL;
	mm1 = menu1;
	while (mm1!=NULL) {
		// do not check recursive calls
		if (itIsSameCxSymbolIncludingFunClass(&mm1->s, theMethod)) goto cont;
		// nor local variables
		if (mm1->s.b.storage == StorageAuto) goto cont;
		// nor labels 
		if (mm1->s.b.symType == TypeLabel) goto cont;
		// do not check also any symbols from classes defined in inner scope
		if (isStrictlyEnclosingClass(mm1->s.vFunClass, theMethod->vFunClass)) goto cont;
		// (maybe I should not test any local symbols ???)
		// O.K. something to be checked, find correspondance in mm2
//&fprintf(dumpOut, "Looking for correspondance to %s\n", mm1->s.name);
		for(rr1=mm1->markers; rr1!=NULL; rr1=rr1->next) {
			mm2 = refactoryFindSymbolCorrespondingToReferenceWrtPullUpPushDown(menu2, mm1, rr1);
			if (mm2==NULL) {
				ED_ALLOC(rr, S_editorMarkerList);
				FILL_editorMarkerList(rr, editorDuplicateMarker(rr1->d), rr1->usg, diff);
				diff = rr;
			}
		}
	cont:
		mm1=mm1->next;
	}
	return(diff);
}

static void refactoryPullUpPushDownCheckCorrespondance(
	S_olSymbolsMenu *menu1, S_olSymbolsMenu *menu2, S_symbolRefItem *theMethod
	) {
	S_olSymbolsMenu		*mm1, *mm2;
	S_editorMarkerList	*rr, *rr1, *rr2, *diff;
	diff = refactoryPullUpPushDownDifferences(menu1, menu2, theMethod);
	if (diff!=NULL) {
		refactoryShowSafetyCheckFailingDialog(&diff, "Those references will be  misinterpreted after refactoring");
		editorFreeMarkersAndMarkerList(diff); diff=NULL;
		refactoryAskForReallyContinueConfirmation();
	}
}

static void refactoryReduceParenthesesAroundExpression(S_editorMarker *mm, char *expression) {
	S_editorMarker 	*lp, *rp, *eb, *ee;
	int				elen;
	refactoryMakeSyntaxPassOnSource(mm);
	if (s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION].file != s_noneFileIndex) {
		assert(s_spp[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION].file != s_noneFileIndex);
		assert(s_spp[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION].file != s_noneFileIndex);
		assert(s_spp[SPP_PARENTHESED_EXPRESSION_END_POSITION].file != s_noneFileIndex);
		elen = strlen(expression);
		lp = editorCrNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION]);
		rp = editorCrNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION]);
		eb = editorCrNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION]);
		ee = editorCrNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_END_POSITION]);
		if (ee->offset - eb->offset == elen && strncmp(MARKER_TO_POINTER(eb), expression, elen)==0) {
			refactoryReplaceString(lp, 1, "");
			refactoryReplaceString(rp, 1, "");
		}
		editorFreeMarker(lp);
		editorFreeMarker(rp);
		editorFreeMarker(eb);
		editorFreeMarker(ee);
	}
}

static void refactoryRemoveRedundantParenthesesAroundThisOrSuper(S_editorMarker *mm, char *keyword) {
	S_editorMarker 	*lp, *rp, *eb, *ee;
	char			*ss;
	ss = refactoryGetIdentifierOnMarker_st(mm);
	if (strcmp(ss, keyword)==0) {
		refactoryReduceParenthesesAroundExpression(mm, keyword);
	}
}

static void refactoryReduceCastedThis(S_editorMarker *mm, char *superFqtName) {
	S_editorMarker 	*lp, *rp, *eb, *ee, *tb, *te, *rr, *dd;
	char			*ss;
	int				superFqtLen, castExprLen;
	char			castExpr[MAX_FILE_NAME_SIZE];
	superFqtLen = strlen(superFqtName);
	ss = refactoryGetIdentifierOnMarker_st(mm);
	if (strcmp(ss,"this")==0) {
		refactoryMakeSyntaxPassOnSource(mm);
		if (s_spp[SPP_CAST_LPAR_POSITION].file != s_noneFileIndex) {
			assert(s_spp[SPP_CAST_RPAR_POSITION].file != s_noneFileIndex);
			assert(s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION].file != s_noneFileIndex);
			assert(s_spp[SPP_CAST_EXPRESSION_END_POSITION].file != s_noneFileIndex);
			lp = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_LPAR_POSITION]);
			rp = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_RPAR_POSITION]);
			tb = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_TYPE_BEGIN_POSITION]);
			te = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_TYPE_END_POSITION]);
			eb = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION]);
			ee = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_END_POSITION]);
			rp->offset ++;
			if (ee->offset - eb->offset == 4 /*strlen("this")*/) {
				if (refactoryIsMethodPartRedundantWrtPullUpPushDown(lp, rp)) {
					refactoryReplaceString(lp, rp->offset-lp->offset, "");
				} else if (te->offset - tb->offset == superFqtLen
						   && strncmp(MARKER_TO_POINTER(tb), superFqtName, superFqtLen) == 0) {
					// a little bit hacked:  ((superfqt)this).  -> super
					rr = editorDuplicateMarker(ee);
					editorMoveMarkerToNonBlank(rr, 1);
					if (CHAR_ON_MARKER(rr) == ')') {
						dd = editorDuplicateMarker(rr);
						dd->offset ++;
						editorMoveMarkerToNonBlank(dd, 1);
						if (CHAR_ON_MARKER(dd) == '.') {
							castExprLen = ee->offset - lp->offset;
							strncpy(castExpr, MARKER_TO_POINTER(lp), castExprLen);
							castExpr[castExprLen]=0;
							refactoryReduceParenthesesAroundExpression(mm, castExpr);
							refactoryReplaceString(lp, ee->offset-lp->offset, "super");
						}
						editorFreeMarker(dd);
					}
					editorFreeMarker(rr);
				}
			}
			editorFreeMarker(lp);
			editorFreeMarker(rp);
			editorFreeMarker(tb);
			editorFreeMarker(te);
			editorFreeMarker(eb);
			editorFreeMarker(ee);
		}
	}
}

static int refactoryIsThereACastOfThis(S_editorMarker *mm) {
	S_editorMarker 	*eb, *ee;
	char			*ss;
	int				res;
	res = 0;
	ss = refactoryGetIdentifierOnMarker_st(mm);
	if (strcmp(ss,"this")==0) {
		refactoryMakeSyntaxPassOnSource(mm);
		if (s_spp[SPP_CAST_LPAR_POSITION].file != s_noneFileIndex) {
			assert(s_spp[SPP_CAST_RPAR_POSITION].file != s_noneFileIndex);
			assert(s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION].file != s_noneFileIndex);
			assert(s_spp[SPP_CAST_EXPRESSION_END_POSITION].file != s_noneFileIndex);
			eb = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION]);
			ee = editorCrNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_END_POSITION]);
			if (ee->offset - eb->offset == 4 /*strlen("this")*/) {
				res = 1;
			}
			editorFreeMarker(eb);
			editorFreeMarker(ee);
		}
	}
	return(res);
}

static void refactoryReduceRedundantCastedThissInMethod(
	S_editorMarker *point, S_editorRegionList **methodreg
	) {
	S_editorMarkerList	*ll;
	S_editorMarker		*lp, *rp, *eb, *ee;
	S_olSymbolsMenu		*mm;
	char				superFqtName[MAX_FILE_NAME_SIZE];
	char				*ss;

	refactoryGetNameOfTheClassAndSuperClass(point, NULL, superFqtName);
	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point,NULL, "-olcxmaybethis",NULL);
	olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);

	createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, methodreg);
	for(mm = s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			for(ll=mm->markers; ll!=NULL; ll=ll->next) {
				// casted expression "((cast)this) -> this"
				// casted expression "((cast)this) -> super"
				ss = refactoryGetIdentifierOnMarker_st(ll->d);
				if (strcmp(ss,"this")==0) {
					refactoryReduceCastedThis(ll->d, superFqtName);
					refactoryRemoveRedundantParenthesesAroundThisOrSuper(ll->d, "this");
					//&refactoryRemoveRedundantParenthesesAroundThisOrSuper(ll->d, "super");
				}
			}
		}
	}
}

static void refactoryExpandThissToCastedThisInTheMethod(
	S_editorMarker *point, 
	char *thiscFqtName, char *supercFqtName, 
	S_editorRegionList *methodreg
	) {
	char 				thisCast[MAX_FILE_NAME_SIZE];
	char 				superCast[MAX_FILE_NAME_SIZE];
	char 				*ss;
	S_editorMarkerList 	*ll;
	S_olSymbolsMenu 	*mm;
	sprintf(thisCast, "((%s)this)", thiscFqtName);
	sprintf(superCast, "((%s)this)", supercFqtName);

	refactoryEditServerParseBuffer( s_ropt.project, point->buffer, point,NULL, "-olcxmaybethis",NULL);
	olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);
	createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, &methodreg);
	for(mm = s_olcxCurrentUser->browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
		if (mm->selected && mm->visible) {
			for(ll=mm->markers; ll!=NULL; ll=ll->next) {
				ss = refactoryGetIdentifierOnMarker_st(ll->d);
				// add casts only if there is yet this or super
				if (strcmp(ss,"this")==0) {
					// check whether there is yet a casted this
					if (! refactoryIsThereACastOfThis(ll->d)) {
						refactoryCheckedReplaceString(ll->d, 4, "this", "");
						refactoryReplaceString(ll->d, 0, thisCast);
					}
				} else if (strcmp(ss,"super")==0) {
					refactoryCheckedReplaceString(ll->d, 5, "super", "");
					refactoryReplaceString(ll->d, 0, superCast);
				}
			}
		}
	}
}

static void refactoryPushDownPullUp(S_editorMarker *point, int direction, int limitIndex) {
	char 				sourceFqtName[MAX_FILE_NAME_SIZE];
	char 				superFqtName[MAX_FILE_NAME_SIZE];
	char 				targetFqtName[MAX_FILE_NAME_SIZE];
	char				*ss;
	int					lines, size;
	S_editorMarker	 	*target, *mstart, *mend, *movedEnd, *pp, *ppp;
	S_editorMarkerList	*ll;
	S_editorRegionList 	*methodreg, *lll;
	S_olSymbolsMenu		*mm, *mm1, *mm2;
	S_symbolRefItem		*theMethod;

	target = getTargetFromOptions();
	if (! validTargetPlace(target, "-olcxmmtarget")) return;

	refactoryUpdateReferences(s_ropt.project);

	refactoryGetNameOfTheClassAndSuperClass(point, sourceFqtName, superFqtName);
	refactoryGetNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
	refactoryGetMethodLimitsForMoving(point, &mstart, &mend, limitIndex);
	lines = editorCountLinesBetweenMarkers(mstart, mend);

	// prechecks
	refactorySetMovingPrecheckStandardEnvironment(point, targetFqtName);
	if (limitIndex == SPP_METHOD_DECLARATION_BEGIN_POSITION) {
		// method
		if (direction == PULLING_UP) {
			if (! (tpCheckTargetToBeDirectSubOrSupClass(REQ_SUPERCLASS, "superclass")
				   && tpCheckSuperMethodReferencesForPullUp()
				   && tpCheckMethodReferencesWithApplOnSuperClassForPullUp())) {
				fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
			}
		} else {
			if (! (tpCheckTargetToBeDirectSubOrSupClass(REQ_SUBCLASS, "subclass"))) {
				fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
			}
		}
	} else {
		// field
		if (direction == PULLING_UP) {
			if (! (tpCheckTargetToBeDirectSubOrSupClass(REQ_SUPERCLASS, "superclass")
				   && tpPullUpFieldLastPreconditions())) {
				fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
			}
		} else {
			if (! (tpCheckTargetToBeDirectSubOrSupClass(REQ_SUBCLASS, "subclass")
				&& tpPushDownFieldLastPreconditions())) {
				fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
			}
		}
	}

	ED_ALLOC(methodreg, S_editorRegionList);
	FILLF_editorRegionList(methodreg, mstart, mend, NULL);

	refactoryExpandThissToCastedThisInTheMethod(point, sourceFqtName, superFqtName, methodreg);

	movedEnd = editorDuplicateMarker(mend);
	movedEnd->offset --;

	// perform moving
	refactoryApplyExpandShortNames(point->buffer, point);
	size = mend->offset - mstart->offset;
	refactoryPushAllReferencesOfMethod(point, "-olallchecks");
	createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, NULL);
	assert(s_olcxCurrentUser->browserStack.top!=NULL && s_olcxCurrentUser->browserStack.top->hkSelectedSym!=NULL);
	theMethod = &s_olcxCurrentUser->browserStack.top->hkSelectedSym->s;
	editorMoveBlock(target, mstart, size, &s_editorUndo);

	// recompute methodregion, maybe free old methodreg before!!
	pp = editorDuplicateMarker(mstart);
	ppp = editorDuplicateMarker(movedEnd);
	ppp->offset ++;
	ED_ALLOC(methodreg, S_editorRegionList);
	FILLF_editorRegionList(methodreg, pp, ppp, NULL);

	// checks correspondance
	refactoryPushAllReferencesOfMethod(point, "-olallchecks");
	createMarkersForAllReferencesInRegions(s_olcxCurrentUser->browserStack.top->menuSym, NULL);
	assert(s_olcxCurrentUser->browserStack.top && s_olcxCurrentUser->browserStack.top->previous);
	mm1 = s_olcxCurrentUser->browserStack.top->previous->menuSym; 
	mm2 = s_olcxCurrentUser->browserStack.top->menuSym; 
	
	refactoryPullUpPushDownCheckCorrespondance(mm1, mm2, theMethod);
	// push down super.method() check
	if (limitIndex == SPP_METHOD_DECLARATION_BEGIN_POSITION) {
		if (direction == PUSHING_DOWN) {
			refactorySetMovingPrecheckStandardEnvironment(point, targetFqtName);
			if (! tpCheckSuperMethodReferencesAfterPushDown()) {
				fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
			}
		}
	}

	// O.K. now repass maybethis and reduce casts on this
	refactoryReduceRedundantCastedThissInMethod(point, &methodreg);

	// reduce long names in the method
	refactoryPerformReduceNamesAndAddImports(&methodreg, INTERACTIVE_NO);

	// and generate output
	refactoryAplyWholeRefactoringFromUndo();

#if ZERO
	ppcGenGotoMarkerRecord(point);
	ppcGenNumericRecord(PPC_INDENT, lines, "", "\n");
#endif

}


static void refactoryPullUpField(S_editorMarker *point) {
	refactoryPushDownPullUp(point, PULLING_UP , SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

static void refactoryPullUpMethod(S_editorMarker *point) {
	refactoryPushDownPullUp(point, PULLING_UP , SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

static void refactoryPushDownField(S_editorMarker *point) {
	refactoryPushDownPullUp(point, PUSHING_DOWN , SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

static void refactoryPushDownMethod(S_editorMarker *point) {
	refactoryPushDownPullUp(point, PUSHING_DOWN , SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

// --------------------------------------------------------------------


static char * refactoryComputeUpdateOptionForSymbol(S_editorMarker *point) {	
	S_editorMarkerList 	*occs, *o;
	S_olSymbolsMenu		*csym;
	int 				hasHeaderReferenceFlag, scope, cat, multiFileRefsFlag, fn;
	int					symtype, storage, accflags;
	char				*res;

	assert(point!=NULL && point->buffer!=NULL);
	mainSetLanguage(point->buffer->name, &s_language);

	hasHeaderReferenceFlag = 0;
	multiFileRefsFlag = 0;
	occs = refactoryGetReferences(point->buffer, point, NULL, PPCV_BROWSER_TYPE_WARNING);
	csym =  s_olcxCurrentUser->browserStack.top->hkSelectedSym;
	scope = csym->s.b.scope;
	cat = csym->s.b.category;
	symtype = csym->s.b.symType;
	storage = csym->s.b.storage;
	accflags = csym->s.b.accessFlags;
	if (occs == NULL) {
		fn = s_noneFileIndex;
	} else {
		assert(occs->d!=NULL && occs->d->buffer!=NULL);
		fn = occs->d->buffer->ftnum;
	}
	for(o=occs; o!=NULL; o=o->next) {
		assert(o->d!=NULL && o->d->buffer!=NULL);
		assert(s_fileTab.tab[o->d->buffer->ftnum]!=NULL);
		if (fn != o->d->buffer->ftnum) {
			multiFileRefsFlag = 1;
		}
		if (! s_fileTab.tab[o->d->buffer->ftnum]->b.commandLineEntered) {
			hasHeaderReferenceFlag = 1;
		}
	}

	if (LANGUAGE(LAN_JAVA)) {
		if (cat == CatLocal) {
			// useless to update when there is nothing about the symbol in Tags
			res = "";
		} else if (symtype==TypeDefault 
				   && (storage == StorageMethod || storage == StorageField)
				   && ((accflags & ACC_PRIVATE) != 0)
			) {
			// private field or method, 
			// no update makes renaming after extract method much faster
			res = "";
		} else {
			res = "-fastupdate";
		}
	} else {
		if (cat == CatLocal) {
			// useless to update when there is nothing about the symbol in Tags
			res = "";
		} else if (hasHeaderReferenceFlag) {
			// once it is in a header, full update is required
			res = "-update";
		} else if (scope==ScopeAuto || scope==ScopeFile) {
			// for example a local var or a static function not used in any header
			if (multiFileRefsFlag) {
				error(ERR_INTERNAL, "something goes wrong, a local symbol is used in several files");
				res = "-update";
			} else {
				res = "";
			}
		} else if (! multiFileRefsFlag) {
			// this is a little bit tricky. It may provoke a bug when
			// a new external function is not yet indexed, but used in another file.
			// But it is so practical, so take the risk.
			res = "";
		} else {
			// may seems too strong, but implicitly linked global functions
			// requires this (for example).
			res = "-fastupdate";
		}
	}

	editorFreeMarkersAndMarkerList(occs);
	occs = NULL;
	olcxPopOnly();

 fini:
	return(res);
}

// --------------------------------------------------------------------


void mainRefactory(int argc, char **argv) {
	int 				fArgCount;
	char				*file, *fff;
	char 				ifile[MAX_FILE_NAME_SIZE];
	S_editorBuffer		*buf;
	S_editorMarker		*point, *mark;


	//
	copyOptions(&s_ropt, &s_opt);		// save command line options !!!!
	// in general in this file:
	//   s_ropt are options passed to c-xrefactory
	//   s_opt are options valid for interactive edit-server 'sub-task'
	// this was commented out, but 
	copyOptions(&s_cachedOptions, &s_opt);
	// MAGIC, fill something to restrict to browsing
	s_ropt.cxrefs = OLO_LIST;

	mainOpenOutputFile(s_ropt.outputFileName);
	editorLoadAllOpenedBufferFiles();
	// initialise lastQuasySaveTime
	editorQuasySaveModifiedBuffers();

	if (s_ropt.project==NULL) {
		fatalError(ERR_ST, "You have to specify active project with -p option", 
				   XREF_EXIT_ERR);
	}

	fArgCount = 0; fff = getInputFile(&fArgCount);
	if (fff==NULL) {
		file = NULL;
	} else {
		strcpy(ifile, fff);
		file = ifile;
	}

	buf = NULL;
	if (file==NULL) fatalError(ERR_ST, "no input file", XREF_EXIT_ERR);
	if (file!=NULL) buf = editorFindFile(file);

	point = refactoryGetPointFromRefactoryOptions(buf);
	mark = refactoryGetMarkFromRefactoryOptions(buf);

	s_refactoringStartPoint = s_editorUndo;

	// init subtask
	mainTaskEntryInitialisations(argnum(s_refactoryEditSrvInitOptions), 
								 s_refactoryEditSrvInitOptions);
	s_refactoryXrefEditSrvSubTaskFirstPassing = 1;
	// ------------

	s_progressFactor = 1;
	if (s_ropt.theRefactoring==PPC_AVR_RENAME_SYMBOL
		|| s_ropt.theRefactoring==PPC_AVR_RENAME_CLASS
		|| s_ropt.theRefactoring==PPC_AVR_RENAME_PACKAGE) {
		s_progressFactor = 3;
		s_refactoryUpdateOption = refactoryComputeUpdateOptionForSymbol(point);
		refactoryRename(buf, point);
	} else if (s_ropt.theRefactoring==PPC_AVR_EXPAND_NAMES) {
		s_progressFactor = 1;
		refactoryExpandShortNames(buf, point);
	} else if (s_ropt.theRefactoring==PPC_AVR_REDUCE_NAMES) {
		s_progressFactor = 1;
		refactoryReduceLongNamesInTheFile(buf, point);
	} else if (s_ropt.theRefactoring==PPC_AVR_ADD_ALL_POSSIBLE_IMPORTS) {
		s_progressFactor = 2;
		refactoryReduceLongNamesInTheFile(buf, point);
	} else if (s_ropt.theRefactoring==PPC_AVR_ADD_TO_IMPORT) {
		s_progressFactor = 2;
		refactoryAddToImports(buf, point);
	} else if (s_ropt.theRefactoring==PPC_AVR_ADD_PARAMETER
			   || s_ropt.theRefactoring==PPC_AVR_DEL_PARAMETER
			   || s_ropt.theRefactoring==PPC_AVR_MOVE_PARAMETER) {
		s_progressFactor = 3;
		s_refactoryUpdateOption = refactoryComputeUpdateOptionForSymbol(point);
		mainSetLanguage(file, &s_language);
		if (LANGUAGE(LAN_JAVA)) s_progressFactor ++;
		refactoryParameterManipulation(buf, point, s_ropt.theRefactoring,
									   s_ropt.olcxGotoVal, s_ropt.parnum2);
	} else if (s_ropt.theRefactoring==PPC_AVR_MOVE_FIELD) {
		s_progressFactor = 6;
		refactoryMoveField(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_MOVE_STATIC_FIELD) {
		s_progressFactor = 4;
		refactoryMoveStaticField(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_MOVE_STATIC_METHOD) {
		s_progressFactor = 4;
		refactoryMoveStaticMethod(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_MOVE_CLASS) {
		s_progressFactor = 3;
		refactoryMoveClass(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_MOVE_CLASS_TO_NEW_FILE) {
		s_progressFactor = 3;
		refactoryMoveClassToNewFile(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_MOVE_ALL_CLASSES_TO_NEW_FILE) {
		s_progressFactor = 3;
		refactoryMoveAllClassesToNewFile(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_PULL_UP_METHOD) {
		s_progressFactor = 2;
		refactoryPullUpMethod(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_PULL_UP_FIELD) {
		s_progressFactor = 2;
		refactoryPullUpField(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_PUSH_DOWN_METHOD) {
		s_progressFactor = 2;
		refactoryPushDownMethod(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_PUSH_DOWN_FIELD) {
		s_progressFactor = 2;
		refactoryPushDownField(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC) {
		s_progressFactor = 6;
		refactoryTurnStaticToDynamic(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC) {
		s_progressFactor = 4;
		refactoryTurnDynamicToStatic(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_EXTRACT_METHOD) {
		s_progressFactor = 1;
		refactoryExtractMethod(point, mark);
	} else if (s_ropt.theRefactoring==PPC_AVR_EXTRACT_MACRO) {
		s_progressFactor = 1;
		refactoryExtractMacro(point, mark);
	} else if (s_ropt.theRefactoring==PPC_AVR_SELF_ENCAPSULATE_FIELD) {
		s_progressFactor = 3;
		refactorySelfEncapsulateField(point);
	} else if (s_ropt.theRefactoring==PPC_AVR_ENCAPSULATE_FIELD) {
		s_progressFactor = 3;
		refactoryEncapsulateField(point);
	} else {
		error(ERR_INTERNAL, "unknown refactoring");
	}

	// always finish once more time
	writeRelativeProgress(0);
	writeRelativeProgress(100);

	if (s_progressOffset != s_progressFactor) {
		sprintf(tmpBuff, "s_progressOffset (%d) != s_progressFactor (%d)", s_progressOffset, s_progressFactor);
		ppcGenRecord(PPC_DEBUG_INFORMATION, tmpBuff, "\n");
	}

	// synchronisation, wait so files can not be saved with the same time
	editorQuasySaveModifiedBuffers();

	mainCloseOutputFile();
	ppcGenSynchroRecord();

	// exiting, pu undefined, so that main will finish
	s_opt.taskRegime = RegimeUndefined;
}

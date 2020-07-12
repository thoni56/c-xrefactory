package com.xrefactory.jedit;

import org.gjt.sp.jedit.*;

public class Opt {

/*

  movePointBackAfterRefactoring


 */

	// Not really options, but saved configuration variables

	public static boolean optionsEditingDisplaysFile = false;
	public static int browserTreeFilter = 2;
	public static int browserRefListFilter = 0;
	

	// options implementation

	private static String pnCompleteFullyQualifiedNames = "xrefactory.completes-fqt-names.option";
	public static boolean completeFullyQualifiedNames() {
		return(jEdit.getProperty(pnCompleteFullyQualifiedNames).equals("true"));
	}
	public static void setCompleteFullyQualifiedNames(boolean value) {
		jEdit.setProperty(pnCompleteFullyQualifiedNames, String.valueOf(value));
	}


	private static String pnCompletionDialogMaxWidth = "xrefactory.completions-max-width.option";
	public static int completionDialogMaxWidth() {
		int res = 600;
		try {res = Integer.parseInt(jEdit.getProperty(pnCompletionDialogMaxWidth));} catch(Exception e){}
		return(res);
	}
	public static void setCompletionDialogMaxWidth(int value) {
		jEdit.setProperty(pnCompletionDialogMaxWidth, String.valueOf(value));
	}


	private static String pnCompletionDialogMaxHeight = "xrefactory.completions-max-height.option";
	public static int completionDialogMaxHeight() {
		int res = 300;
		try {res = Integer.parseInt(jEdit.getProperty(pnCompletionDialogMaxHeight));} catch(Exception e){}
		return(res);
	}
	public static void setCompletionDialogMaxHeight(int value) {
		jEdit.setProperty(pnCompletionDialogMaxHeight, String.valueOf(value));
	}


	private static String pnUpdateIsFast = "xrefactory.full-auto-update.option";
	public static boolean fullAutoUpdate() {
		return(jEdit.getProperty(pnUpdateIsFast).equals("true"));
	}
	public static void fullAutoUpdate(boolean value) {
		jEdit.setProperty(pnUpdateIsFast, String.valueOf(value));
	}


	private static String pnUpdateBeforeRefactorings = "xrefactory.update-before-refactorings.option";
	public static boolean updateBeforeRefactorings() {
		return(jEdit.getProperty(pnUpdateBeforeRefactorings).equals("true"));
	}
	public static void setUpdateBeforeRefactorings(boolean value) {
		jEdit.setProperty(pnUpdateBeforeRefactorings, String.valueOf(value));
	}


	private static String pnUpdateBeforePush = "xrefactory.update-before-push.option";
	public static boolean updateBeforePush() {
		return(jEdit.getProperty(pnUpdateBeforePush).equals("true"));
	}
	public static void setUpdateBeforePush(boolean value) {
		jEdit.setProperty(pnUpdateBeforePush, String.valueOf(value));
	}


	private static String pnSaveFilesBeforeRefactorings = "xrefactory.save-before-refactorings.option";
	public static boolean saveFilesBeforeRefactorings() {
		return(jEdit.getProperty(pnSaveFilesBeforeRefactorings).equals("true"));
	}
	public static void setSaveFilesBeforeRefactorings(boolean value) {
		jEdit.setProperty(pnSaveFilesBeforeRefactorings, String.valueOf(value));
	}


	private static String pnSaveFilesAfterRefactorings = "xrefactory.save-after-refactorings.option";
	public static boolean saveFilesAfterRefactorings() {
		return(jEdit.getProperty(pnSaveFilesAfterRefactorings).equals("true"));
	}
	public static void setSaveFilesAfterRefactorings(boolean value) {
		jEdit.setProperty(pnSaveFilesAfterRefactorings, String.valueOf(value));
	}


	private static String pnSaveFilesAskForConfirmation = "xrefactory.save-asks-confirmation.option";
	public static boolean saveFilesAskForConfirmation() {
		return(jEdit.getProperty(pnSaveFilesAskForConfirmation).equals("true"));
	}
	public static void setSaveFilesAskForConfirmation(boolean value) {
		jEdit.setProperty(pnSaveFilesAskForConfirmation, String.valueOf(value));
	}


	private static String pnReferencePushingsKeepBrowserFilter = "xrefactory.pushing-keeps-filter.option";
	public static boolean referencePushingsKeepBrowserFilter() {
		return(jEdit.getProperty(pnReferencePushingsKeepBrowserFilter).equals("true"));
	}
	public static void setReferencePushingsKeepBrowserFilter(boolean value) {
		jEdit.setProperty(pnReferencePushingsKeepBrowserFilter, String.valueOf(value));
	}

	private static String pnActiveProject = "xrefactory.active-project.option";
	public static String activeProject() {
		return(jEdit.getProperty(pnActiveProject));
	}
	public static void setActiveProject(String value) {
		jEdit.setProperty(pnActiveProject, value);
	}

	private static String pnAskBeforeBrowsingJavadoc = "xrefactory.ask-before-javadoc.option";
	public static boolean askBeforeBrowsingJavadoc() {
		return(jEdit.getProperty(pnAskBeforeBrowsingJavadoc).equals("true"));
	}
	public static void setAskBeforeBrowsingJavadoc(boolean value) {
		jEdit.setProperty(pnAskBeforeBrowsingJavadoc, String.valueOf(value));
	}

	private static String pnCompletionAccessCheck = "xrefactory.completion-access-check.option";
	public static boolean completionAccessCheck() {
		return(jEdit.getProperty(pnCompletionAccessCheck).equals("true"));
	}
	public static void setCompletionAccessCheck(boolean value) {
		jEdit.setProperty(pnCompletionAccessCheck, String.valueOf(value));
	}

	private static String pnCompletionLinkageCheck = "xrefactory.completion-linkage-check.option";
	public static boolean completionLinkageCheck() {
		return(jEdit.getProperty(pnCompletionLinkageCheck).equals("true"));
	}
	public static void setCompletionLinkageCheck(boolean value) {
		jEdit.setProperty(pnCompletionLinkageCheck, String.valueOf(value));
	}

	private static String pnMaxCompletions = "xrefactory.max-completions.option";
	public static int maxCompletions() {
		int res = 600;
		try {res = Integer.parseInt(jEdit.getProperty(pnMaxCompletions));} catch(Exception e){}
		return(res);
	}
	public static void setMaxCompletions(int value) {
		jEdit.setProperty(pnMaxCompletions, String.valueOf(value));
	}

	// ------------------------ SAVED CONFIGURATION VARIABLES-------------------------

	private static String pnBrowserTreeDividerPosition = "xrefactory.browser-divider.prop";
	public static int browserTreeDividerPosition() {
		int res = 600;
		try {res = Integer.parseInt(jEdit.getProperty(pnBrowserTreeDividerPosition));} catch(Exception e){}
		return(res);
	}
	public static void setBrowserTreeDividerPosition(int value) {
		jEdit.setProperty(pnBrowserTreeDividerPosition, String.valueOf(value));
	}

	// ------------------- PATTERNS DO NOT REMOVE!

	// boolean pattern
	private static String pnXxb = "xrefactory.option";
	public static boolean xxb() {
		return(jEdit.getProperty(pnXxb).equals("true"));
	}
	public static void setXxb(boolean value) {
		jEdit.setProperty(pnXxb, String.valueOf(value));
	}
	// String pattern
	private static String pnXxs = "xrefactory.option";
	public static String xxs() {
		return(jEdit.getProperty(pnXxs));
	}
	public static void setXxs(String value) {
		jEdit.setProperty(pnXxs, value);
	}
	// int pattern
	private static String pnXxi = "xrefactory.option";
	public static int xxi() {
		int res = 600;
		try {res = Integer.parseInt(jEdit.getProperty(pnXxi));} catch(Exception e){}
		return(res);
	}
	public static void setXxi(int value) {
		jEdit.setProperty(pnXxi, String.valueOf(value));
	}



}

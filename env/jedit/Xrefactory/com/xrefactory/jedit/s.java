package com.xrefactory.jedit;

import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.textarea.*;
import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.io.*;
import org.gjt.sp.jedit.gui.*;
import org.gjt.sp.jedit.io.*;
import org.gjt.sp.jedit.help.*;
import java.net.*;
import java.awt.event.*;
import javax.swing.border.*;

public class s {

	public static boolean debug = false;

	public static class Position {
		View 		view;
		String 		file;
		int			offset;
		Position(View view, String file, int offset) {
			this.view=view; this.file=file; this.offset=offset;
		}
	}

	public static class FocusRequester implements Runnable {
		Component component;
		public void run() {
			component.requestFocus();
		}
		FocusRequester(Component component) {
			this.component = component;
		}
	}

	public static class MessageDisplayer implements Runnable {
		String message;
		boolean temporary;
		public void run() {
			if (view!=null && view.getStatus()!=null) {
				if (temporary) {
					view.getStatus().setMessageAndClear(message);
				} else {
					view.getStatus().setMessage(message);
				}
			}
		}
		MessageDisplayer(String message, boolean temporary) {
			this.message = message;
			this.temporary = temporary;
		}
	}

	public static class TagReportDisplayer implements Runnable {
		String 		report;
		Component 	caller;
		public void run() {
			JDialog pp = getParentDialog(caller);
			if (pp!=null) {
				new TagReportDialog(report, pp);
			} else {
				JFrame ff = getParentFrame(caller);
				new TagReportDialog(report, ff);
			}
		}
		TagReportDisplayer(String report, Component caller) {
			this.report = report;
			this.caller = caller;
		}
	}

	public static final String xrefRegistrationUrl = "http://www.xref-tech.com/license.html";
	public static final String xrefTaskDirUrl = "http://www.xref-tech.com/xrefactory/downloads/jedit/";
	public static final String dockableClassTreeWindowName = "xrefactory.class-tree-viewer";
	public static final String dockableUrlBrowserWindowName = "xrefactory.urlviewer";
	public static final String dockableBrowserWindowName = "xrefactory.browser";
	public static final String dockableRetrieverWindowName = "xrefactory.retriever";

	public static final String tryInstallXrefOption = "xrefactory.try-install-xref.prop";
	public static final String configurationFileOption = "xrefactory.configuration-file.prop";
	// options
	public static final String optCompletionFgColor = "xrefactory.completion-fgcolor";
	public static final String optCompletionSymbolFgColor = "xrefactory.completion-symbol-fgcolor";
	public static final String optCompletionBgColor = "xrefactory.completion-bgcolor";
	public static final String optCompletionBgColor2 = "xrefactory.completion-bgcolor2";
	public static final String optCompletionFont = "xrefactory.completion-font";
	public static final String optCompletionSymbolFont = "xrefactory.completion-symbol-font";
	public static final String optCompletionFqtLevel = "xrefactory.completion-fqtlevel";
	public static final String optCompletionCaseSensitive = "xrefactory.completion-cases";
	public static final String optCompletionDelPendingId = "xrefactory.completion-del-pending-id";
	public static final String optCompletionInsParenthesis = "xrefactory.completion-ins-parenthesis";
	public static final String optCompletionOldDialog = "xrefactory.completion-old-dialog";
	public static final String optBrowserFgColor = "xrefactory.browser-fgcolor";
	public static final String optBrowserBgColor = "xrefactory.browser-bgcolor";
	public static final String optBrowserSelectionColor = "xrefactory.browser-selcolor";
	public static final String optBrowserSymbolColor = "xrefactory.browser-symcolor";
	public static final String optBrowserNmSymbolColor = "xrefactory.browser-nmsymcolor";
	public static final String optBrowserFont = "xrefactory.browser-font";
	public static final String optBrowserToolBarPos = "xrefactory.browser-toolbar-position";
	public static final String optBrowserSrcWithRefs = "xrefactory.browser-src-code-in-references";
	public static final String optBrowserPickButton = "xrefactory.browser-pick-button";
	public static final String optBrowserRetrieveButton = "xrefactory.browser-retrieve-button";
	public static final String optRetrieverFgColor = "xrefactory.retriever-fgcolor";
	public static final String optRetrieverBgColor = "xrefactory.retriever-bgcolor";
	public static final String optRetrieverFont = "xrefactory.retriever-font";
	public static final String optRetrieverSearchButton = "xrefactory.retriever-search";
	public static final String optRetrieverBackButton = "xrefactory.retriever-back";
	public static final String optRetrieverForwardButton = "xrefactory.retriever-forward";
	public static final String optRefactoryCommentMovingLevel = "xrefactory.refactoring-comment-move";
	public static final String optRefactoryPreferImportOnDemand = "xrefactory.refactoring-imports-on-demand";


	//
	public static final String projectAutoDetectionName = "None (Automatic Project Detection)";
	public static final String answerNoAndDoNotAskAnymore ="No and do not display this dialog anymore";
	public static final String xrefTmpFilesPrefix = "xrefactory";
	public static final String xrefTmpFilesSuffix = ".tmp";
	public static final String xrefLogFileSingleName = "xrefactory.log";

	public static String tagFilesDirectory = "/";
	public static String tagProcessingReportFile = null;

	public static final int MAX_COMPLETION_HISTORY = 100;
	public static final int MAX_FILE_SIZE_FOR_READ =   20000000;
	public static final int MAX_FILE_SIZE_FOR_REPORT =  2000000;
	public static final int XREF_MAX_TREE_DEEP = 500;
	public static final int XREF_DOWNLOAD_BUFFER_SIZE = 1024;
	public static final String XREF_NON_MEMBER_SYMBOL_NAME = "NOT MEMBER SYMBOLS";
	// this is the only entry point to jEdit!
	public static View view;
	public static String activeProject;

	// this is the entry point on currently opened browsing dialog
	public static BrowserTopPanel 							xbrowser=null;
	public static HelpViewer 								xHelpViewer=null;
	public static ExtractMethodDialog.ExtractMethodPanel 	xExtractMethod=null;

	public static String			completionIdAfterCaret;
	public static String			completionIdBeforeCaret;

	
	public static String targetFile = "";
	public static int targetLine = 0;

	public static boolean panic = false;
	public static boolean xrefTaskYetChecked = false;

	public static BrowserTask xbTask = null;
	
	public static String xrefTaskUrl;
	public static String rootDir;
	public static String slash;
	public static String classPathSeparator;
	public static String pluginInstallationPath;
	public static String tmpDirectoryPath;
	public static String osName;
	public static String archName;
	public static String jdkClassPath;
	public static String javaHome;
	public static String homeDir;
	public static String configurationFile;
	public static String xrefTaskPath;
	public static String javaVersion;

	public static int currentTmpFileCounter = 0;
	public static int tmpFileStep = 1;

	public static final int OS_OTHER 		= 0;
	public static final int OS_WINDOWS 		= 1;
	public static final int OS_OS2	 		= 2;
	public static int osCode = OS_OTHER;

	public static Color light_gray = new Color(0xd0, 0xd0, 0xd0);
	public static Color completionBgDefaultColor = new Color(255,240,200);
	public static Color completionBgDefaultColor2 = new Color(235,220,180);
	public static Color completionSelectionColor = new Color(200,200,255);
	

	public static Font defaultFont = new Font("Monospaced", Font.PLAIN, 12);
	public static Font defaultComplSymFont = new Font("Monospaced", Font.BOLD, 12);
	public static Font browserDefaultFont = new Font(null, Font.PLAIN, 10);
	public static Font optionsBrowseButtonFont = new Font(null, Font.PLAIN, 10);

	// clas tree fonts and colors
	public static Font ctTopLevelFont = new Font(null, Font.PLAIN, 12);
	public static Font ctItalicBoldFont = new Font(null, Font.BOLD|Font.ITALIC, 10);
	public static Font ctItalicFont = new Font(null, Font.ITALIC, 10);
	public static Font ctBoldFont = new Font(null, Font.BOLD, 10);
	public static Font ctNormalFont = new Font(null, Font.PLAIN, 10);

	public static Color ctBgColor = Color.white;
	public static Color ctFgColor = Color.black;
	public static Color ctSelectionColor = Color.blue;
	public static Color ctSymbolColor = Color.red;
	public static Color ctNmSymbolColor = s.light_gray;


	public static EmptyBorder emptyBorder = new EmptyBorder(0,0,0,0);

	//
	public static void setupCtreeFontsAndColors() {
		Font res = jEdit.getFontProperty(s.optBrowserFont,  ctNormalFont);
		ctNormalFont = new Font(res.getName(), Font.PLAIN, res.getSize());
		ctBoldFont = new Font(res.getName(), Font.BOLD, res.getSize());
		ctItalicFont = new Font(res.getName(), Font.ITALIC, res.getSize());
		ctItalicBoldFont = new Font(res.getName(), Font.ITALIC | Font.BOLD, res.getSize());
		ctTopLevelFont = new Font(res.getName(), Font.BOLD, res.getSize()+2);

		ctBgColor = jEdit.getColorProperty(s.optBrowserBgColor, ctBgColor);
		ctFgColor = jEdit.getColorProperty(s.optBrowserFgColor, ctFgColor);
		ctSelectionColor = jEdit.getColorProperty(s.optBrowserSelectionColor, ctSelectionColor);
		ctSymbolColor = jEdit.getColorProperty(s.optBrowserSymbolColor, ctSymbolColor);
		ctNmSymbolColor = jEdit.getColorProperty(s.optBrowserNmSymbolColor, ctNmSymbolColor);
	}

	public static void updateBrowserVisage() {
		View [] vs = jEdit.getViews();
		for(int i=0; i<vs.length; i++) {
			BrowserTopPanel bb = getBrowser(vs[i]);
			if (bb!=null) {
				bb.referencesPanel.reflist.setBackground(ctBgColor);
				bb.referencesPanel.reflist.setForeground(ctFgColor);
				bb.referencesPanel.reflist.setFont(ctNormalFont);
				bb.treePanel.xtree.setBackground(ctBgColor);
			}
			DockableBrowser db = getDockableBrowser(vs[i]);
			if (db!=null) {
				db.repositionToolBar();
			}
			DockableClassTree ct = getClassTreeViewer(vs[i]);
			if (ct!=null) {
				ct.repositionToolBar();
				ct.tree.setBackground(ctBgColor);
			}
		}
	}

	//
	static public File getNewTmpFileName() {
		File res;
		assertt(tmpFileStep!=0);
		do {
			res = new File(tmpDirectoryPath 
						   + xrefTmpFilesPrefix 
						   + currentTmpFileCounter
						   + xrefTmpFilesSuffix);
			currentTmpFileCounter += tmpFileStep;
		} while (res.exists());
		return(res);
	}

	static class tmpFileNameFilter implements FileFilter {
		long offset;
		public boolean accept(File f) {
			String name = f.getName();
			int len = name.length();
			//&System.err.println("checking filername " + name + " == " + f.getAbsolutePath());
			if (len>10 
				&& name.substring(0, xrefTmpFilesPrefix.length()).equals(xrefTmpFilesPrefix)
				&& name.substring(len-xrefTmpFilesSuffix.length()).equals(xrefTmpFilesSuffix)) {
				long testtime = System.currentTimeMillis()-offset;
				//&System.err.println("checking " + f.lastModified() + " to " + testtime);
				if (f.lastModified() < testtime) {
					return(true);
				}
			}
			return(false);
		}
		tmpFileNameFilter(long offset) {
			this.offset = offset;
		}
	}
	static public void cleanOldTmpFiles() {
		File tmp = new File(tmpDirectoryPath);
		File[] files = tmp.listFiles(new tmpFileNameFilter(48L*60*60*1000));
		if (files.length > 0) {
			int confirm = JOptionPane.showConfirmDialog(null, "There are some old temporary files generated by Xrefactory, can I delete them?", "Xrefactory temporary files", JOptionPane.YES_NO_OPTION);
			if (confirm == JOptionPane.YES_OPTION) {
				files = tmp.listFiles(new tmpFileNameFilter(24L*60*60*1000));
				for(int i=0; i<files.length; i++) {
					//&System.err.println("deleting " + files[i].getAbsolutePath());
					files[i].delete();
				}
			}
		}
	}

	public static boolean browserIsDisplayed() {
		return(view.getDockableWindowManager().isDockableWindowVisible(
			dockableBrowserWindowName));
		//&return(xbrowser!=null && xbrowser.isShowing());
	}
	public static boolean browserIsDisplayed(Component cc) {
		View v = getParentView(cc);
		return(v.getDockableWindowManager().isDockableWindowVisible(
			dockableBrowserWindowName));
		//&return(xbrowser!=null && xbrowser.isShowing());
	}

	public static DockableClassTree getClassTreeViewer(View view) {
	    return((DockableClassTree)view.getDockableWindowManager().getDockable(dockableClassTreeWindowName));
	}

	public static DockableBrowser getDockableBrowser(View view) {
	    DockableBrowser db = (DockableBrowser) view.getDockableWindowManager().getDockable(dockableBrowserWindowName);
		return db;
	}
	
	public static BrowserTopPanel getBrowser(View view) {
	    DockableBrowser db = (DockableBrowser) view.getDockableWindowManager().getDockable(dockableBrowserWindowName);
	    if (db != null) return(db.browser); 
		return xbrowser;
	}
	
	public static void beforePushBrowserFiltersUpdates() {
		Opt.browserTreeFilter = 2;
		if (! Opt.referencePushingsKeepBrowserFilter()) {
			Opt.browserRefListFilter = 0;
		}
	}
	
	public static void browserNeedsToUpdate(View view) {
		BrowserTopPanel browser = getBrowser(view);
		if (browser!=null) browser.needToUpdate();
	}

	public static void showAndUpdateBrowser(View view) {
		view.getDockableWindowManager().showDockableWindow(dockableBrowserWindowName);
		browserNeedsToUpdate(view);
	}

	// TODO all this stuff should be parametrized by view
	public static String getFileName() {
		return(view.getBuffer().getPath());
	}

	public static int getCaretPosition() {
		return(view.getTextArea().getCaretPosition());
	}

	public static JEditTextArea getTextArea() {
		return(view.getTextArea());
	}

	public static Buffer getBuffer() {
		return(view.getBuffer());
	}

	// TODO, you should skip commentaries here!
	public static String inferDefaultSourcePath() {
		int i,j, count;
		char cc;
		File ff = new File(getBuffer().getPath()).getParentFile();
		String path = ff.getAbsolutePath();
		String pp = getTextArea().getText();
		if (pp.length()>7 && pp.substring(0,7).equals("package")) {
			i = "package".length();
		} else {
			i = pp.indexOf("\npackage");
			if (i== -1) return(path);
			i += "\npackage".length();
		}
		while (Character.isSpaceChar(pp.charAt(i))) i++;
		j=i; count=0;
		while (Character.isLetterOrDigit((cc=pp.charAt(j)))
			   || Character.isSpaceChar(cc)
			   || cc=='.') {
			if (Character.isLetterOrDigit(cc) || cc=='.') count++;
			j++;
		}
		if (count >= path.length()) return(path);
		int ressize = path.length()-count;
		while (ff.getAbsolutePath().length() > ressize) {
			ff = ff.getParentFile();
		}
		return(ff.getAbsolutePath());
	}

	public static void fileSetExecPermission(String fname) {
		if (osName.toLowerCase().indexOf("linux") != -1
			|| osName.toLowerCase().indexOf("sunos") != -1
			|| osName.toLowerCase().indexOf("mac-os-x") != -1
			|| osName.toLowerCase().indexOf("unix") != -1) {
			String[] chmodcmd = {"chmod", "a+x", fname};
			try {
				Runtime.getRuntime().exec(chmodcmd);
			} catch (Exception e) {}
		}
	}

	public static void downloadXrefTask(String url, String file) {
		int n, progressi, progressn;
		if (debug) System.err.println("Downloading "+url+" into "+file);
		Progress progress = Progress.crNew(null, "Downloading xref task");
		FileOutputStream oo = null;
		InputStream ii = null;
		try {
			URLConnection con = new URL(url).openConnection();
			ii = con.getInputStream();
			File of = new File(file);
			if (! of.getParentFile().exists()) {
				of.getParentFile().mkdir();
			}
			oo = new FileOutputStream(of);
			byte buffer[] = new byte[XREF_DOWNLOAD_BUFFER_SIZE];
			progressi = 0; progressn = con.getContentLength();
			n = 1;
			while (n >= 0) {
				n = ii.read(buffer, 0, XREF_DOWNLOAD_BUFFER_SIZE);
				if (n > 0) {
					oo.write(buffer, 0, n);
					progressi += n;
				}
				if (! progress.setProgress(progressi*100/progressn)) {
					// cancel
					n = -2;
				}
			}
			ii.close();
			oo.close();
			fileSetExecPermission(file);
			if (n == -2) {
				// canceled
				of.delete();
			}
		} catch(Exception e) {
			try {
				if (oo!=null) {
					oo.close();
					new File(file).delete();
				}
				if (ii!=null) ii.close();
			} catch (Exception ee) {}
			progress.setVisible(false);
			JOptionPane.showMessageDialog(
				null,
				e.toString()+"\nWhile downloading "+url+".\nMaybe wrong proxy configuration?", 
				"Xrefactory Error",
				JOptionPane.ERROR_MESSAGE);
		}
		progress.setVisible(false);
	}

	public static void setGlobalValues() {
		String deftmp, defslash, defcpsep, defcfile, deftask, downloadtask, instdir;

		// non settable property, should be always false in normal installations
		debug = jEdit.getBooleanProperty("xrefactory.debug-mode", debug);

		javaVersion = System.getProperty("java.version","1.3.0");

		osName = System.getProperty("os.name").toLowerCase();
		archName = System.getProperty("os.arch").toLowerCase();

		if (osName.equals("mac os x")) osName = "mac-os-x";
		if (osName.indexOf("windows") != -1) osName = "windows";
		if (osName.indexOf("os/2") != -1) osName = "os2";
		if (archName.indexOf("86") != -1) archName = "x86";

		if (debug) {
			System.err.println("[Xrefactory] os == " + osName);
			System.err.println("[Xrefactory] arch == " + archName);
		}
		if (osName.toLowerCase().indexOf("windows") != -1) {
			osCode = OS_WINDOWS;
			rootDir = "C:\\";
			deftmp = "C:\\";
			defslash = "\\";
			defcpsep = ";";
			defcfile = "_xrefrc";
			deftask = "xref.exe";
			homeDir = System.getProperty("user.home","c:\\");
		} else if (osName.equals("os2")) {
			osCode = OS_OS2;
			rootDir = "C:\\";
			deftmp = "C:\\";
			defslash = "\\";
			defcpsep = ";";
			defcfile = ".xrefrc";
			deftask = "xref.exe";
			homeDir = System.getProperty("user.home","c:\\");
		} else {
			osCode = OS_OTHER;
			rootDir = "/";
			deftmp = "/tmp/";
			defslash = "/";
			defcpsep = ":";
			defcfile = ".xrefrc";	
			deftask = "xref";
			homeDir = System.getProperty("user.home","~/");
		}

		downloadtask = "xref-" + Protocol.XREF_VERSION_NUMBER + "-" + osName + "-" + archName + ".exe";

		xrefTaskUrl = xrefTaskDirUrl + downloadtask;
		slash = System.getProperty("file.separator", defslash);
		classPathSeparator = System.getProperty("path.separator", defcpsep);

   		tmpDirectoryPath = System.getProperty("java.io.tmpdir", deftmp);

		if (tmpDirectoryPath.substring(tmpDirectoryPath.length()) != slash) {
			tmpDirectoryPath = tmpDirectoryPath + slash;
		}
		if (homeDir.substring(homeDir.length()) != slash) {
			homeDir = homeDir + slash;
		}

		javaHome = System.getProperty("java.home", rootDir);
		if (javaHome.substring(javaHome.length()) != slash) {
			javaHome = javaHome + slash;
		}
		
		instdir = "/";
 		String cp = System.getProperty("java.class.path");
		if (cp!=null) {
			//& JOptionPane.showMessageDialog(this, cp);
			int i = cp.toLowerCase().indexOf("jedit.jar");
			//&if (i == -1) i = cp.toLowerCase().indexOf("xrefac~1.jar");
			if (i != -1) {
				String ss = cp.substring(0, i);
				int j = ss.lastIndexOf(classPathSeparator);
				if (j != -1) ss = ss.substring(j+1);
				instdir = ss + "jars" + slash;
			}
		}
		pluginInstallationPath = "";
		xrefTaskPath = "xref";
		String path0 = jEdit.getSettingsDirectory();
		if (path0!=null) path0 = path0+slash+"jars"+slash;
		String path1 = homeDir+".jedit"+slash+"jars"+slash;
		String path2 = homeDir+"jedit"+slash+"jars"+slash;
		if (path0!=null && new File(path0+"Xrefactory.jar").exists()) {
			pluginInstallationPath = path0;
		} else if (new File(path1+"Xrefactory.jar").exists()) {
			pluginInstallationPath = path1;
		} else if (new File(path2+"Xrefactory.jar").exists()) {
			pluginInstallationPath = path2;
		} else if (new File(instdir+"Xrefactory.jar").exists()) {
			pluginInstallationPath = instdir;
		} else {
			System.err.println("[Xrefactory] can't find installation directory.");
			System.err.println("[Xrefactory] neither "+path0+", "+path1+", ");
			System.err.println("[Xrefactory] "+path2+" or "+instdir+" contains Xrefactory.jar");
		}
		xrefTaskPath = pluginInstallationPath+"Xrefactory"+slash+deftask;

		jdkClassPath = javaHome + "lib" + slash + "rt.jar";
		configurationFile = jEdit.getProperty(configurationFileOption);
		if (configurationFile==null || configurationFile.equals("none")) {
			configurationFile = new File(homeDir + defcfile).getAbsolutePath();
			jEdit.setProperty(configurationFileOption, configurationFile);
		}
		tagFilesDirectory = homeDir + "Xrefs" + slash;
		tagProcessingReportFile = tmpDirectoryPath + xrefLogFileSingleName;
		activeProject = null;
	}

	public static void checkIfXrefTaskExists() {
		if (xrefTaskYetChecked) return;
		xrefTaskYetChecked = true;
		File taskfile = new File(xrefTaskPath);
		if ((! taskfile.exists()) && ! jEdit.getProperty(tryInstallXrefOption).equals(Protocol.XREF_VERSION_NUMBER)){
			Object confirm = JOptionPane.showInputDialog(
				null,
				"You have  installed Xrefactory plugin. It requires shareware 'xref'\nnot  distributed with  the  plugin. However, xref can be  freely\ndownloaded   and   evaluated.   Can   I   download   xref   into\nfile: "+xrefTaskPath+"?",
				"Xrefactory installation",
				JOptionPane.QUESTION_MESSAGE,
				null,
				new String[] {"Yes", "No", "No and do not display this dialog anymore"},
				"Yes");
			if ("Yes".equals(confirm)) {
				downloadXrefTask(xrefTaskUrl, xrefTaskPath);
			} else if (answerNoAndDoNotAskAnymore.equals (confirm)) {
				// never
				jEdit.setProperty(tryInstallXrefOption, Protocol.XREF_VERSION_NUMBER);
			}
		}
		if (! taskfile.exists()) xrefTaskPath = "xref";
	}

	public static void checkVersionCorrespondance(String taskVersion, DispatchData data) {
		if (! taskVersion.equals(Protocol.XREF_VERSION_NUMBER)) {
			int confirm = JOptionPane.showConfirmDialog(
				getProbableParent(data.callerComponent),
				"You have installed Xrefactory version "+Protocol.XREF_VERSION_NUMBER+" while using\nxref  task version  "+taskVersion+". Version mismatch  may cause\nproblems during  execution. Can I download correct  version of\nxref task?",
				"Xrefactory Problem",
				JOptionPane.YES_NO_OPTION,
				JOptionPane.QUESTION_MESSAGE);
			if (confirm == JOptionPane.YES_OPTION) {
				xbTask.killThis(false);
				downloadXrefTask(xrefTaskUrl, xrefTaskPath);
				data.panic = true;		// restart task
			}
		}
	}

	public static void checkBrowserTask() {
		if (xbTask==null) {
			// this is the first invocation of Xrefactory
			cleanOldTmpFiles();
			checkIfXrefTaskExists();
			xbTask = new BrowserTask();
			// check version
			DispatchData ndata = new DispatchData(xbTask, view);
			XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt(
				"-olcheckversion=" + Protocol.XREF_VERSION_NUMBER, ndata);
			Dispatch.dispatch(receipt, ndata);
			if (ndata.panic) {
				xbTask = new BrowserTask();
			}
		}
	}

	public static void setGlobalValuesNoActiveProject(View view) {
		s.view = view;
		panic = false;
		checkBrowserTask();
		//& setGlobalValues();
	}

	public static void setGlobalValues(View view, boolean projectCreationAllowed) {
		setGlobalValuesNoActiveProject(view);
		//&if (activeProject == null) {
		activeProject = computeActiveProject(s.view, projectCreationAllowed);
		//&}
	}

	public static Position getPosition(View v) {
		Position res = new Position(
			v, 
			v.getBuffer().getPath(),
			v.getTextArea().getCaretPosition()
			);
		return(res);
	}

	public static void moveToPosition(View view, String file, int off) {
		Buffer buf = jEdit.openFile(view, file);
		// wait until operation completes
		VFSManager.waitForRequests();
		JEditTextArea texta = view.getTextArea();
		texta.setCaretPosition(off);
		VFSManager.waitForRequests();
	}

	public static void moveToPosition(Position p) {
		moveToPosition(p.view, p.file, p.offset);
	}

	public static void moveToPosition(View view,String file, int line, int col) {
		Buffer buf = jEdit.openFile(view, file);
		VFSManager.waitForRequests();
		JEditTextArea texta = view.getTextArea();
		int off = 0;
		int offmax = 0;
		try {
			off = texta.getLineStartOffset(line-1);
			offmax = texta.getLineEndOffset(line-1);
			off = off + col;
			if (off > offmax) off = offmax;
			texta.setCaretPosition(off);
			VFSManager.waitForRequests();
		} catch (Exception eee) {
			if (debug) System.err.println("Problem while moving to "+file+" "+line+" "+col);
		}
	}

	public static void assertt(boolean b) {
		if (! b) {
			(new Exception()).printStackTrace();
			JOptionPane.showMessageDialog(view, 
										  "An assertion Failed, an internal problem occurs.", 
										  "Xrefactory Error", 
										  JOptionPane.ERROR_MESSAGE);
		}
	}

	public static int countLines(String s) {
		int i,n;
		i= -1; n=0;
		while((i=s.indexOf('\n', i+1)) != -1) n++;
		if (n>0 || s.length()>0) n++;
		return(n);
	}
	
	public static String sprintOption(String ss) {
		String res;
		int i,ii;
		if (ss.indexOf(" ") == -1 
			&& ss.indexOf("\"") == -1 
			&& (!ss.equals("")) 
			&& !(ss.charAt(0)=='"')
			) {
			res = ss;
		} else {
			res = "\"";
			ii = 0;
			while ((i=ss.indexOf('"',ii))!=-1) {
				res += ss.substring(ii, i) + "${dq}";
				ii = i+1;
			}
			res += ss.substring(ii);
			res += "\"";
		}
		return(res);
	}

	public static boolean stringArrayEqual(String[] a1, String[] a2) {
		if (a1.length != a2.length) return(false);
		for(int i=0; i<a1.length; i++) {
			if (! a1[i].equals(a2[i])) return(false);
		}
		return(true);
	}

	public static int skipNumber(XrefCharBuffer ss, int i, int len) {
		char	c;
		c = ss.buf[i];
		while (i<len && Character.isDigit(c)) {
			i++;
			c = ss.buf[i];
		}
		return(i);
	}
	public static int skipBlank(XrefCharBuffer ss, int i, int len) {
		while (i<len && Character.isWhitespace(ss.buf[i])) i++;
		return(i);
	}


	private static GridBagConstraints gbc = new GridBagConstraints();
	
	public static void addGbcComponent(Container pane, 
									   int x, int y, 
									   int w, int h, 
									   int wx, int wy, 
									   int fill,
									   Component cc) {
		gbc.gridx = x; gbc.gridy = y; 
		gbc.gridwidth = w; gbc.gridheight = h;
		gbc.weightx = wx; gbc.weighty = wy; gbc.fill = fill;
		pane.add(cc, gbc);
	}

	public static void addButtonLine(JPanel panel, int y, JComponent [] components,
									 boolean borders) {
		int i,x;
		x = -1;
		
		if (borders) {
			x++;
			addGbcComponent(panel, x,y, 1,1, 10,10, 
							GridBagConstraints.HORIZONTAL,
							new JPanel());
		}

		for(i=0; i<components.length; i++) {
			x++;
			addGbcComponent(panel, x,y, 1,1, 1,10, 
							GridBagConstraints.HORIZONTAL,
							components[i]);
		}

		if (borders) {
			x++;
			addGbcComponent(panel, x,y, 1,1, 10,10, 
							GridBagConstraints.HORIZONTAL,
							new JPanel());
		}
	}

	public static void addExtraButtonLine(Container thiss, int x, int y, int dx, int dy,
										  int wx, int wy, JComponent [] buttons,
										  boolean borders) {
		JPanel bp = new JPanel();
		bp.setLayout(new GridBagLayout());
		addButtonLine(bp, 0, buttons, borders);
		addGbcComponent(thiss, x, y, dx, dy, wx, wy, 
						GridBagConstraints.HORIZONTAL,
						bp);
	}

	public static void buttonsSetFont(JComponent [] buttons, Font font) {
		int i;
		for(i=0; i<buttons.length; i++) buttons[i].setFont(font);
	}

	public static void buttonsAddActionListener(JButton [] buttons, ActionListener listener) {
		int i;
		for(i=0; i<buttons.length; i++) buttons[i].addActionListener(listener);
	}

	public static String computeActiveProject(Component parent, boolean projectCreationAllowed) {
		DispatchData ndata = new DispatchData(xbTask, parent);
		ndata.projectCreationAllowed = projectCreationAllowed;
		xbTask.tmpOption.clear();
		xbTask.tmpOption.add("-olcxgetprojectname");
		//& xbTask.addCommonOptions(xbTask.tmpOption, ndata);
		xbTask.addCurrentFileOptions(xbTask.tmpOption);
		XrefCharBuffer receipt = xbTask.callProcess(xbTask.tmpOption, ndata);
		Dispatch.dispatch(receipt, ndata);
		if (ndata.panic) return(null);
		return(ndata.info);
	}

	static void performMovingRefactoring(String moption, String fieldOption) {
		if (targetLine>0 && !targetFile.equals("")) {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add(moption);
			xroption.add("-commentmovinglevel=" + 
						 jEdit.getIntegerProperty(optRefactoryCommentMovingLevel,0));
			xroption.add("-movetargetfile="+targetFile);
			xroption.add("-rfct-param1="+targetLine);
			if (fieldOption!=null) {
				xroption.add("-rfct-param2="+fieldOption);
			}
			Refactorings.mainRefactorerInvocation(xroption,false);
		}
	}
	
	public static String computeSomeInformationInXref(Component parent, String option) {
		DispatchData ndata = new DispatchData(xbTask, parent);
		XrefCharBuffer receipt = ndata.xTask.callProcessOnFileNoSaves(
			new String[]{option}, 
			ndata
			);
		Dispatch.dispatch(receipt, ndata);
		if (ndata.panic) return(null);
		return(ndata.info);
	}
	
	public static String computeSomeInformationInXref(Component parent, String[] options) {
		DispatchData ndata = new DispatchData(xbTask, parent);
		XrefCharBuffer receipt = ndata.xTask.callProcessOnFileNoSaves(
			options, ndata);
		Dispatch.dispatch(receipt, ndata);
		if (ndata.panic) return(null);
		return(ndata.info);
	}
	
	public static String getOrComputeActiveProject() {
		if (activeProject == null) {
			activeProject = computeActiveProject(view,true);
		}
		return(activeProject);
	}

	public static void displayProjectInformationLater() {
		String message;
		String project = getOrComputeActiveProject();
		if (project == null) {
			message = "Can't infer Xrefactory project!";
		} else {
			message = "Xrefactory project: " + project;
		}
		// O.K. display it later, so io messages do not overwrite it
		SwingUtilities.invokeLater(new MessageDisplayer(message,true));
	}

	static public char getBufferChar(Buffer buffer, int offset) {
		return(buffer.getText(offset,1).charAt(0));
	}

	static boolean projectExists(String name, Options[] projects) {
		int i;
		for(i=0; i<projects.length; i++) {
			if (projects[i].projectName.equals(name)) return(true);
		}
		return(false);
	}

	static public String commonPrefix(String s1, String s2) {
		int i,l1,l2;
		l1 = s1.length();
		l2 = s2.length();
		for(i=0; i<l1 && i<l2; i++) {
			if (s1.charAt(i) != s2.charAt(i)) break;
		}
		return(s1.substring(0, i));
	}

	static public String getIdentifierOnCaret() {
		Buffer buffer = getBuffer();
		int len = buffer.getLength();
		int offset = getCaretPosition();
		if (offset == len) return("");
		int j = offset;
		while (j>0 && Character.isJavaIdentifierPart(getBufferChar(buffer, j))) {
			j --;
		}
		if (j>0) j++;
		int i = j;
		//&if (Character.isJavaIdentifierStart(getBufferChar(buffer, i))) {
		while (i<len && Character.isJavaIdentifierPart(getBufferChar(buffer, i))) {
			i ++;
		}
		//&}
		return(buffer.getText(j, i-j));
	}

	static public String getIdentifierBeforeCaret() {
		Buffer buffer = getBuffer();
		int len = buffer.getLength();
		int offset = getCaretPosition();
		if (offset == len) return("");
		int j = offset - 1;
		while (j>0 && Character.isJavaIdentifierPart(getBufferChar(buffer, j))) {
			j --;
		}
		if (j>0) j++;
		if (j>=offset) return("");
		return(buffer.getText(j, offset-j));
	}

	static public String getIdentifierAfterCaret() {
		Buffer buffer = getBuffer();
		int len = buffer.getLength();
		int offset = getCaretPosition();
		if (offset == len) return("");
		int i = offset;
		while (i<len && Character.isJavaIdentifierPart(getBufferChar(buffer, i))) {
			i ++;
		}
		return(buffer.getText(offset, i-offset));
	}

	public static Point getAbsCoordinate(Point pp, Component cc) {
		double x = pp.getX();
		double y = pp.getY();
		while (cc!=null) {
			x += cc.getX();
			y += cc.getY();
			cc = cc.getParent();
		}
		return(new Point((int)x, (int)y));
	}
	
	public static Point recommendedLocation(Component cc) {
		//&int line = getTextArea().getCaretLine();
		//&int vline = getTextArea().getFoldVisibilityManager().physicalToVirtual(line);
		//&int offset = getTextArea().getCaretPosition() - getTextArea().getLineStartOffset(line);
		Point pp = getTextArea().offsetToXY(getTextArea().getCaretPosition());
		if (pp==null) {
			JOptionPane.showMessageDialog(view, "Can't get caret position", 
										  "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
			pp = new Point(100,100);
		} else {
			pp = getAbsCoordinate(pp, cc);
		}
		FontMetrics fm = getTextArea().getPainter().getFontMetrics();
		return(new Point((int)pp.getX(), (int)pp.getY() + fm.getHeight()));
	}

	public static void moveOnScreen(Component cc) {
		Point ccloc = cc.getLocation();
		int x = (int)ccloc.getX();
		int y = (int)ccloc.getY();
		Dimension ccdimm = cc.getSize();
		Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
//&System.err.println("Checking "+ccloc.getX()+" "+ccdimm.getWidth()+" "+dim.getWidth());
		if (x < 0) x = 0;
		if (x+ccdimm.getWidth() > dim.getWidth()) {
			x = (int)(dim.getWidth()-ccdimm.getWidth());
		}
		if (y < 0) y = 0;
		if (y+ccdimm.getHeight() > dim.getHeight()) {
			y = (int)(dim.getHeight()-ccdimm.getHeight());
		}
		cc.setLocation(x, y);
	}
	
	static public String dotifyString(String ss) {
		String res;
		res = ss;
		res = res.replace('/','.');
		res = res.replace('\\','.');
		return(res);
	}

	static public int getNumberOfCharOccurences(String s, char c) {
		int i,n;
		i = n = 0;
		while ((i=s.indexOf(c, i)) != -1) {
			i++;
			n++;
		}
		return(n);
	}

	static public void arrayCopyDelElement(Object[] src, Object[] dest, int i) {
		System.arraycopy(src, 0, dest, 0, i);
		System.arraycopy(src, i+1, dest, i, src.length-i-1);
	}

	static public void arrayCopyAddElement(Object[] src, Object[] dest, int i) {
		System.arraycopy(src, 0, dest, 0, i);
		dest[i] = null;
		System.arraycopy(src, i, dest, i+1, src.length-i);
	}

	static public String upperCaseFirstLetter(String s) {
		String res = s;
		int slen = s.length();
		if (slen>0) res = s.substring(0,1).toUpperCase() + s.substring(1);
		return(res);
	}

	static public String lowerCaseFirstLetter(String s) {
		String res = s;
		int slen = s.length();
		if (slen>0) res = s.substring(0,1).toLowerCase() + s.substring(1);
		return(res);
	}

	// PARENTS COMPONENTS

	static public Component getProbableParent(Component c) {
		Component res, cc;
		res = cc = c;
		while (cc!=null && ! (res instanceof Window)) {
			res = cc;
			cc = cc.getParent();
		}
		if (res==null) res = view;
		return(res);
	}

	static public View getParentView(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof View)) {
			cc = cc.getParent();
		}
		if (cc==null) cc = view;
		return((View)cc);
	}

	static public JDialog getParentDialog(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof JDialog)) {
			cc = cc.getParent();
		}
		return((JDialog)cc);
	}

	static public JFrame getParentFrame(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof JFrame)) {
			cc = cc.getParent();
		}
		return((JFrame)cc);
	}

	static public ResolutionPanel getParentResolutionPanel(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof ResolutionPanel)) cc = cc.getParent();
		return((ResolutionPanel)cc);
	}

	static public DockableBrowser getParentBrowserPanel(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof DockableBrowser)) cc = cc.getParent();
		return((DockableBrowser)cc);
	}

	static public BrowserTopPanel getParentBrowserTopPanel(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof BrowserTopPanel)) cc = cc.getParent();
		return((BrowserTopPanel)cc);
	}

	static public BrowserTopPanel.TreePanel getParentTreePanel(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof BrowserTopPanel.TreePanel)) cc = cc.getParent();
		return((BrowserTopPanel.TreePanel)cc);
	}

	static public BrowserTopPanel.ReferencesPanel getParentReferencesPanel(Component c) {
		Component cc;
		cc = c;
		while (cc!=null && ! (cc instanceof BrowserTopPanel.ReferencesPanel)) cc = cc.getParent();
		return((BrowserTopPanel.ReferencesPanel)cc);
	}

	// ------------------------------------------

	public static String getViewParameter(int viewId) {
		return("view"+viewId);	
	}
	
	public static boolean synchronizedUpdateTagFile(Component parent) {
		String updateOption;
		if (Opt.fullAutoUpdate()) updateOption = "-update";
		else updateOption = "-fastupdate";
		//&updateOption = "-create";
		DispatchData ndata = new DispatchData(xbTask, parent);
		XrefTaskForTagFile tr = XrefTaskForTagFile.runXrefOnTagFile(updateOption, "Updating Tags.", false, ndata);
		if (tr == null) return(true);
		synchronized (tr.lock) {
			try {
				if (tr.running) tr.lock.wait();
			} catch(Exception e) {
				e.printStackTrace();
			}
		}
		return(tr.data.panic);
	}

	public static void dumpMemoryStatus() {
		Runtime runtime = Runtime.getRuntime();
		int freeMemory = (int)(runtime.freeMemory() / 1024);
		int totalMemory = (int)(runtime.totalMemory() / 1024);
		System.err.println(" memory " + freeMemory + "/" + totalMemory);
	}

	public static void saveAllBuffers(View view) {
		jEdit.saveAllBuffers(view, Opt.saveFilesAskForConfirmation());
		try {Thread.currentThread().sleep(100);} catch (InterruptedException e){}
		VFSManager.waitForRequests();
	}

	public static void showTagFileReport(XrefCharBuffer report, Component caller) {
		try {
			if (debug) System.err.println("report=="+report.toString());
			if (report.bufi != 0) {
				String message;
				if (report.bufi > s.MAX_FILE_SIZE_FOR_REPORT) {
					message = "Done. The report is "+report.bufi+" chars long and may run jEdit out of memory. View the report anyway ?";
				} else {
					message = "Done. View report ?";
				}
				int confirm = JOptionPane.YES_OPTION;
				confirm = JOptionPane.showConfirmDialog(
					getParentView(caller),
					message, "Confirmation", 
					JOptionPane.YES_NO_OPTION,
					JOptionPane.QUESTION_MESSAGE);
				if (confirm == JOptionPane.YES_OPTION) {
					SwingUtilities.invokeLater(
						new TagReportDisplayer(Dispatch.reportToHtml(report), caller)
							);
				}
			}
		} catch (Exception e) {
			e.printStackTrace(System.err);
			JOptionPane.showMessageDialog(view, "While reading report file: " + e, 
										  "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
		}
	}

	public static String getExternBrowserName() {
		if (osName.toLowerCase().indexOf("linux") != -1) return("mozilla");
		if (osName.toLowerCase().indexOf("sunos") != -1) return("netscape");
		if (osName.toLowerCase().indexOf("mac-os-x") != -1) return("konqueror");
		if (osName.toLowerCase().indexOf("unix") != -1) return("netscape");
		return("CMD /C start");
	}


	public static void browseUrl(String url, boolean externBrowser, View view) {
		if (externBrowser) {
			String bb = getExternBrowserName();
			try {
				Runtime.getRuntime().exec(bb+" "+url);
			} catch (Exception e) {
			}
		} else {
			/*&
			if (s.javaVersion.compareTo("1.4.0") < 0) {
				if (url.length()>9 && url.substring(0, 9).toLowerCase().equals("file:////")) {
					url = "file:///" +  url.substring(9);
				}
			}
			&*/
			view.getStatus().setMessageAndClear("Url: "+url);
			view.getDockableWindowManager().showDockableWindow(dockableUrlBrowserWindowName);
			DockableUrlBrowser urlViewer = (DockableUrlBrowser)view.getDockableWindowManager().getDockableWindow(dockableUrlBrowserWindowName);
			urlViewer.setPreferredSize(new Dimension(400,400));
			urlViewer.gotoURL(url, true);
		}
		//&SwingUtilities.invokeLater(new MessageDisplayer("Url "+url, true));
	}


	public static void insertCompletionDoNotMoveCaret(Buffer buffer, int offset, String completion) {
		int 		b, i, blen;
		String 		ss;
		if (jEdit.getBooleanProperty(s.optCompletionDelPendingId)) {
			ss = completion;
		} else {
			ss = completion + completionIdAfterCaret;
		}
		b = i = offset;
		blen = buffer.getLength();
		while (i<blen && Character.isJavaIdentifierPart(buffer.getText(i,1).charAt(0))) i++;
		int cidlen = i-b;
		String cid = buffer.getText(b, i-b);
		String cprefix = commonPrefix(cid, ss);
		int cprefixlen = cprefix.length();
		buffer.remove(b+cprefixlen, cidlen-cprefixlen);
		buffer.insert(b+cprefixlen, ss.substring(cprefixlen));
	}

	public static void insertCompletion(Buffer buffer, int offset, String completion) {
		insertCompletionDoNotMoveCaret(buffer, offset, completion);
		getTextArea().setCaretPosition(offset+completion.length());
	}

	// following code is taken from jEdit4.1, all the credits belong to Slava Pestov

	public static JToolBar loadToolBar(String name) {
		JToolBar toolBar = new JToolBar();
		toolBar.setFloatable(false);
		String buttons = jEdit.getProperty(name);
		if(buttons != null) {
			StringTokenizer st = new StringTokenizer(buttons);
			while(st.hasMoreTokens()) {
				String button = st.nextToken();
				if(button.equals("-")) {
					toolBar.addSeparator();
				} else {
					JButton b = GUIUtilities.loadToolButton(button);
					if(b != null) toolBar.add(b);
				}
			}
		}
		return toolBar;
	}

}

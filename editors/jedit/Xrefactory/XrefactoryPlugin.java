//& package com.xrefactory.jedit;

import java.util.*;
import javax.swing.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.textarea.*;
import org.gjt.sp.jedit.gui.*;
import org.gjt.sp.jedit.msg.*;
import com.xrefactory.jedit.*;


public class XrefactoryPlugin extends EditPlugin {

	public void start() {
		s.setGlobalValues();
		Random random = new Random();
		s.currentTmpFileCounter = random.nextInt(1000);
		s.tmpFileStep = random.nextInt(10) + 1;
		random = null;
//&		EditBus.addToNamedList(DockableWindow.DOCKABLE_WINDOW_LIST, s.dockableBrowserWindowName);
//&		EditBus.addToNamedList(DockableWindow.DOCKABLE_WINDOW_LIST, s.dockableRetrieverWindowName);
		if (s.debug) {
			JOptionPane.showMessageDialog(s.view, 
										  "Xrefactory debug mode is turned on", 
										  "Xrefactory Warning", 
										  JOptionPane.WARNING_MESSAGE);

		}
	}

	public void stop() {
		// save some values
		if (jEdit.getFirstView()!=null) {
			BrowserTopPanel t = s.getBrowser(jEdit.getFirstView());
			if (t!=null) {
				Opt.setBrowserTreeDividerPosition((int)t.treePanel.getSize().getWidth());
			}
		}
								  
	}

/*
  public void handleMessage(EBMessage message) {
  if(message instanceof CreateDockableWindow) {
  CreateDockableWindow cmsg = (CreateDockableWindow) message;
  if (cmsg.getDockableWindowName().equals(s.dockableBrowserWindowName)) {
  DockableWindow win = new BrowserDialog.BrowserPanel(cmsg.getView());
  //& DockableWindow win = new DockableBrowser(cmsg.getView());
  cmsg.setDockableWindow(win);
  } else if (cmsg.getDockableWindowName().equals(s.dockableRetrieverWindowName)) {
  cmsg.setDockableWindow(new SymbolRetriever(cmsg.getView()));
  }

  }
  }
*/

/*
    public void createMenuItems(Vector menuItems) {
        menuItems.addElement(GUIUtilities.loadMenu("xrefactory.menu"));
    }

	public void createOptionPanes(OptionsDialog od) {
		OptionGroup xrefactoryGroup = new OptionGroup("xrefactory");
		xrefactoryGroup.addOptionPane(new OptionsForJeditCompletion());
		xrefactoryGroup.addOptionPane(new OptionsForJeditBrowser());
		xrefactoryGroup.addOptionPane(new OptionsForJeditRetriever());
		xrefactoryGroup.addOptionPane(new OptionsForJeditRefactorer());
		xrefactoryGroup.addOptionPane(new OptionsForJeditProjects());
		//&xrefactoryGroup.addOptionPane(new OptionsForJeditCompletion());
		od.addOptionGroup(xrefactoryGroup);
	}
*/

	private static DispatchData mainBrowserNoFileInvocation(View view, String[] options) {
		//& s.setGlobalValues(view,true);
		DispatchData ndata = new DispatchData(s.xbTask, view);
		XrefCharBuffer receipt = ndata.xTask.callProcess(options, ndata);
		Dispatch.dispatch(receipt, ndata);
		return ndata;
	}
	private static DispatchData mainBrowserInvocation(View view, String[] options) {
		//& s.setGlobalValues(view,true);
		DispatchData ndata = new DispatchData(s.xbTask, view);
		XrefCharBuffer receipt = ndata.xTask.callProcessOnFile(options, ndata);
		Dispatch.dispatch(receipt, ndata);
		return ndata;
	}
	private static DispatchData mainBrowserInvocation(View view, String option) {
		return(mainBrowserInvocation(view, new String[]{option}));
	}

	private static void mainTagInvocation(View view, String option, String message){
		if (s.activeProject!=null) {
			DispatchData ndata = new DispatchData(s.xbTask, view);
			XrefTaskForTagFile.runXrefOnTagFile(option, message, true, ndata);
		}
	}

	public static void completion(View view) {
		s.setGlobalValues(view,true);
		CompletionDialog.completion(view);
	}

	public static void createTags(View view) {
		s.setGlobalValues(view, true);
		mainTagInvocation(view, "-create", "Creating Tags.");
	}

	public static void updateTags(View view) {
		s.setGlobalValues(view, true);
		mainTagInvocation(view, "-fastupdate", "Updating Tags.");
	}

	public static void fullUpdateTags(View view) {
		s.setGlobalValues(view, true);
		mainTagInvocation(view, "-update", "Updating Tags.");
	}

	public static void pushName(View view) {
		s.setGlobalValues(view,true);
		s.displayProjectInformationLater();
		DispatchData ndata = new DispatchData(s.xbTask, view);
		new PushSymbolDialog(view, ndata);
	}

	public static void pushNameFromEditor(View view) {
		s.setGlobalValues(view,true);
		s.displayProjectInformationLater();
		String name = s.getIdentifierOnCaret();
		if (name!=null) {
			Buffer buffer = s.getBuffer();
			int caret = s.getCaretPosition();
			int line = buffer.getLineOfOffset(caret);
			int col = caret - buffer.getLineStartOffset(line);
			DispatchData ndata = new DispatchData(s.xbTask, view);
			new Push(new String[] {"-olcxpushname="+name, "-olcxlccursor="+(line+1)+":"+col, "-olnodialog"}, ndata);
		}
	}

	public static void pushAndGotoDefinition(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData ndata = new DispatchData(s.xbTask, view);
			new Push(new String[]{"-olcxpush"}, ndata);
		}
	}

	public static void popAndReturn(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData data = mainBrowserNoFileInvocation(view, new String[]{"-olcxpop"});
			if (! data.panic) s.browserNeedsToUpdate(view);
		}
	}

	public static void repush(View view) {
		s.setGlobalValues(view, true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData ndata = mainBrowserInvocation(view, "-olcxrepush");
			if (! ndata.panic) s.browserNeedsToUpdate(view);
		}
	}

	public static void previousReference(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData data = mainBrowserNoFileInvocation(view, new String[]{"-olcxminus"});
			if (! data.panic) {
				if (s.browserIsDisplayed(view)) {
					s.getBrowser(view).referencesPanel.updateSelection();
				}
			}
		}
	}

	public static void nextReference(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData data = mainBrowserNoFileInvocation(view, new String[]{"-olcxplus"});
			if (! data.panic) {
				if (s.browserIsDisplayed(view)) {
					s.getBrowser(view).referencesPanel.updateSelection();
				}
			}
		}
	}

	public static void globalUnusedSymbols(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData ndata = new DispatchData(s.xbTask, view);
			new Push(new String[]{"-olcxpushglobalunused"}, ndata);
		}
	}

	public static void localUnusedSymbols(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			DispatchData ndata = new DispatchData(s.xbTask, view);
			new Push(new String[]{"-olcxpushfileunused"}, ndata);
		}
	}



	public static void killXrefTask(View view) {
		s.setGlobalValues(view,true);
		if (s.xbTask != null) s.xbTask.killThis(true);
	}


	public static void browser(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			view.getDockableWindowManager().showDockableWindow(s.dockableBrowserWindowName);
			DispatchData ndata = new DispatchData(s.xbTask, view);
			new Push(new String[]{"-olcxpushonly", "-olnodialog"}, ndata);
		}
	}

	public static void refactor(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			mainBrowserInvocation(view, "-olcxgetrefactorings");
		}
	}

	public static void viewClassTree(View view) {
		s.setGlobalValues(view,true);
		if (s.activeProject!=null) {
			s.displayProjectInformationLater();
			mainBrowserInvocation(view, "-olcxclasstree");
		}
	}

	public static void retrieveSymbol(View view) {
		s.setGlobalValues(view,true);
		s.displayProjectInformationLater();
		if (s.activeProject!=null) {
			s.view.getDockableWindowManager().showDockableWindow(s.dockableRetrieverWindowName);
		}
	}

	public static void registration(View view) {
		s.setGlobalValues(view, false);
		int answer = JOptionPane.showOptionDialog(s.view,
												  "\nFurther evolution and maintenance  of Xrefactory is entirely dependent\non its users.  The xref task is proprietary software.  In order to run\nit legally  you need to purchase  a license.  You  can obtain detailed\ninformation   about   available   licenses   and  order   process   on\nhttp://www.xref-tech.com/xrefactory/license.html  .   After successful\nregistration you will receive a  license string which will change your\nevaluation copy into regular one.\n\nIf you  have received a valid  license string, you can now continue to\nthe next dialog.",
 												  "Xrefactory", 
												  JOptionPane.OK_CANCEL_OPTION,
												  JOptionPane.INFORMATION_MESSAGE,
												  null,
												  new String[]{"Browse Url","Continue"},
												  "Continue"
			);
		if (answer == 0) {
			s.browseUrl(s.xrefRegistrationUrl, true, s.view);
		} else {
			new RegistrationDialog();
		}
	}
}


package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.text.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.textarea.*;
import org.gjt.sp.jedit.gui.*;


public class CompletionDialog extends JDialog implements KeyListener {

	DispatchData				data;
	JScrollPane 				scrollPane;
	JTextArea 					textArea;
	String						text;
	LinkedList	 				history;
	String						insertFile;
	int							insertOffset;
	//&boolean						urlBrowserOpenedAtStart; // not good idea

	static CompletionDialog 	lastcDialog;   // to ensure only one dialog at the time

	int							x,y;	// window coordinates
	int							mx, my;	// mouse coordinates

	public class CompletionMouseAdapter extends MouseAdapter {
		public void mousePressed(MouseEvent e) {
			mx = e.getX();
			my = e.getY();
		}
		public void mouseClicked(MouseEvent e) {
			if ((e.getModifiers() & InputEvent.BUTTON1_MASK) != 0) {
				callCompletionGoto();
			}
		}
	}
	public class CompletionMouseMotionAdapter extends MouseMotionAdapter {
		public void mouseDragged(MouseEvent e) {
			if ((e.getModifiers() & InputEvent.BUTTON2_MASK) != 0) {
				int cx = e.getX();
				int cy = e.getY();
				int dx = cx - mx;
				int dy = cy - my;
				x = x+dx;
				y = y+dy;
				CompletionDialog.this.setLocation(x, y);
			}
		}
	}

	static class HistoryElem {
		int 	caret;
		String 	selection;
		HistoryElem(int caret, String selection) {
			this.caret = caret;
			this.selection = selection;
		}
	}

	public static void insertFqtCompletion(Buffer buffer, int offset, String ss) {
		int lastdot = -1;
		int ii;
		while ((ii=ss.indexOf('.', lastdot+1)) != -1) lastdot = ii;
		if (lastdot < 0) {
			s.insertCompletion(buffer, offset, ss);
			return;
		}
		String pack = ss.substring(0, lastdot);
		String cl = ss.substring(lastdot+1);
		new FqtCompletionDialog(pack, cl, buffer, offset);
	}

	public static void addImportAndInsertCompletion(String imp, Buffer buffer, int offset, String cl) {
		// TODO, do this better
		try {
			DispatchData ndata = new DispatchData(s.xbTask, s.view);
			XrefCharBuffer receipt = ndata.xTask.callProcessOnFileNoSaves(
				new String[]{"-olcxtrivialprecheck", "-getlastimportline"},
				ndata);
			Dispatch.dispatch(receipt,ndata);
			int ln = Integer.parseInt(ndata.info);
			int off = buffer.getLineStartOffset(ln);
			String iinsertion = "import "+imp+"\n";
			buffer.insert(off , iinsertion);
			s.insertCompletion(buffer, offset+iinsertion.length(), cl);
		} catch (Exception e) {	}
	}

	static public String getPrefixOnCompletion() {
		int i,j;
		int offset = s.getCaretPosition();
		i = offset - 1;
		while (i>0 && Character.isJavaIdentifierPart(s.getBuffer().getText(i,1).charAt(0))) i--;
		i++;
		return(s.getBuffer().getText(i,offset-i));
	}

	public static int charCount(String ss, char cc, int b, int e) {
		int i = b;
		int res = 0;
		while ((i=ss.indexOf(cc, i))!=-1 && i<e) {
			res ++;
			i ++;
		}
		return(res);
	}

	int getCompletionLineNumber(int car) {
		int line = charCount(text, '\n', 0, car+1);
		return(line);
	}

	void displayProfileIfShouldBeDisplayed() {
		try {
			int i;
			int car = textArea.getCaretPosition();
			int ln = textArea.getLineOfOffset(car);
			int lb = textArea.getLineStartOffset(ln);
			int le = textArea.getLineEndOffset(ln);
			String line = textArea.getText(lb, le-lb);
			// search left par
			for(i=line.length()-1; i>=0; i--) {
				if (line.charAt(i)=='(' 
					&& i<line.length()-1 
					&& i>0
					&& line.charAt(i-1) != ':'
					&& line.charAt(i+1) != ')') break;
				if (line.charAt(i)==':') break;
			}
			if (i>=0 && line.charAt(i)=='(') {
				while (i>0 && line.charAt(i)!=' ') i--;
				String message = line.substring(i,line.length());
				SwingUtilities.invokeLater(new s.MessageDisplayer(message, false));
				//&s.view.getStatus().setMessage(message);
			}
		} catch (Exception e) {
			//& e.printStackTrace(System.err);
		}
	}

	void callCompletionGoto() {
		int car = textArea.getCaretPosition();
		int line = getCompletionLineNumber(car);
		DispatchData ndata = new DispatchData(data, this);
		XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxcgoto" + line, ndata);
		Dispatch.dispatch(receipt, ndata);
	}

	public void keyPressed(KeyEvent e) {
		char key = e.getKeyChar();
		int code = e.getKeyCode();
		int mod = e.getModifiers();
		//&boolean searchFlag = (code==KeyEvent.VK_S && mod==InputEvent.CTRL_MASK);
		boolean searchFlag = (code==KeyEvent.VK_TAB);
		if (code != KeyEvent.VK_UP && code != KeyEvent.VK_DOWN
			&& code != KeyEvent.VK_KP_UP && code != KeyEvent.VK_KP_DOWN
			&& code != KeyEvent.VK_PAGE_UP && code != KeyEvent.VK_PAGE_DOWN
			&& code != KeyEvent.VK_LEFT && code != KeyEvent.VK_RIGHT			
			&& code != KeyEvent.VK_HOME && code != KeyEvent.VK_END			
			) {
			e.consume();
		}
		if (code == KeyEvent.VK_ENTER) {
			int car = textArea.getCaretPosition();
			int line = getCompletionLineNumber(car);
			setVisible(false);
			displayProfileIfShouldBeDisplayed();
			DispatchData ndata = new DispatchData(data, this);
			XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcomplselect" + line, ndata);
			Dispatch.dispatch(receipt,ndata);
		} else if (code == KeyEvent.VK_SPACE 
				   && (e.getModifiers() & InputEvent.CTRL_MASK)!=0) {
			closeDialog();
			s.moveToPosition(s.getParentView(this),insertFile, insertOffset);
			completion(s.view);
		} else if (code == KeyEvent.VK_SPACE) {
			callCompletionGoto();
		} else if (code == KeyEvent.VK_BACK_SPACE) {
			if (history.size() > 1) {
				int oldCaret = ((HistoryElem)history.getFirst()).selection.length();
				history.removeFirst();
				int newCaret = ((HistoryElem)history.getFirst()).selection.length();
				showSelection();
				if (newCaret < oldCaret) {
					s.moveToPosition(s.getParentView(this),insertFile, insertOffset);			
					insertOffset--;
					s.getBuffer().remove(insertOffset, 1);
				}
			}
		} else if (Character.isLetterOrDigit(key) || key=='_' || key=='$' || searchFlag) {
			int car = ((HistoryElem) history.getFirst()).caret;
			String sel = ((HistoryElem) history.getFirst()).selection;
			String nsel = sel;
			if (searchFlag) {
				car ++;
			} else {
				nsel = sel+key;
			}
			int ncar = text.toLowerCase().indexOf("\n" + nsel.toLowerCase(), car);
			if (ncar != -1) {
				String nnsel = text.substring(ncar+1, ncar+nsel.length()+1);
				history.addFirst(new HistoryElem(ncar, nnsel));
				showSelection();
				if (! searchFlag) {
					s.insertCompletion(s.getBuffer(), insertOffset-sel.length(), nnsel);
					insertOffset++;
				}
			}
		} else if ("`~!@#$%^&*()_+|-=\\{}[]:\";'<>?,./".indexOf(key) != -1) {
			closeDialog();
			s.moveToPosition(s.getParentView(this),insertFile, insertOffset);
			s.getBuffer().insert(insertOffset, ""+key);
		} else if (code == KeyEvent.VK_ESCAPE) {
			closeDialog();
			if (mod == InputEvent.SHIFT_MASK || mod == InputEvent.ALT_MASK) {
				s.moveToPosition(s.getParentView(this), insertFile, insertOffset);								
			}
//&		} else if (code == KeyEvent.VK_LEFT) {
//&		} else if (code == KeyEvent.VK_RIGHT) {
		}			
	}
	public void keyReleased(KeyEvent e) {
		e.consume();
	}
	public void keyTyped(KeyEvent e) {
		e.consume();
	}

	void showSelection() {
		int car = ((HistoryElem) history.getFirst()).caret;
		String sel = ((HistoryElem) history.getFirst()).selection;
		textArea.setCaretPosition(car);
		textArea.moveCaretPosition(car+sel.length());
	}

	void closeDialog() {
		setVisible(false);
		/*& // not a good idea
		  if (! urlBrowserOpenedAtStart) {
		  DockableWindowManager dwm = s.view.getDockableWindowManager();
		  if (dwm.isDockableWindowVisible(s.dockableUrlBrowserWindowName)) {
		  dwm.removeDockableWindow(s.dockableUrlBrowserWindowName);
		  }
		  }
		  &*/
	}

	public static void completion(org.gjt.sp.jedit.View view) {
		if (s.activeProject!=null) {
			s.completionIdBeforeCaret = s.getIdentifierBeforeCaret();
			s.completionIdAfterCaret = s.getIdentifierAfterCaret();

			XrefStringArray options = new XrefStringArray();
			options.add("-olcxcomplet");
			if (jEdit.getBooleanProperty(s.optCompletionOldDialog)) options.add("-jeditc1"); // to be removed!!!!!!
			options.add("-maxcompls="+Opt.maxCompletions());
			options.add("-olfqtcompletionslevel="+jEdit.getIntegerProperty(s.optCompletionFqtLevel,1));
			if (Opt.completionLinkageCheck()) {
				options.add("-olchecklinkage");
			}
			if (Opt.completionAccessCheck()) {
				options.add("-olcheckaccess");
			}
			if (jEdit.getBooleanProperty(s.optCompletionCaseSensitive)) {
				options.add("-completioncasesensitive");
			}
			if (jEdit.getBooleanProperty(s.optCompletionInsParenthesis)) {
				options.add("-completeparenthesis");
			}
			DispatchData ndata = new DispatchData(s.xbTask, view);
			XrefCharBuffer receipt = ndata.xTask.callProcessOnFileNoSaves(options.toStringArray(true), ndata);
			Dispatch.dispatch(receipt, ndata);
			// do not display project information to not erase profile info 
			// from completions
			//& s.displayProjectInformationLater();
		}
	}

	void setCompletionDialog(String completionList, DispatchData data) {
		String prefix = getPrefixOnCompletion();
		this.data = data;
		text = "\n" + completionList;
		textArea.setText(completionList);
		textArea.setBackground(jEdit.getColorProperty(s.optCompletionBgColor, s.completionBgDefaultColor));
		textArea.setForeground(jEdit.getColorProperty(s.optCompletionFgColor, Color.black));
		textArea.setFont(jEdit.getFontProperty(s.optCompletionFont, s.defaultFont));
		textArea.setEditable(true);
		insertOffset = s.getCaretPosition();
		insertFile = s.getFileName();
		//sp.setMaximumSize(new Dimension(600, 300));
		// todo proportional size
		pack();
		Dimension dim = getSize();
		// 20 is for scroll bars
		if (dim.getHeight() > Opt.completionDialogMaxHeight()) {
			dim.setSize(dim.getWidth()+20, Opt.completionDialogMaxHeight());
		}
		if (dim.getWidth() > Opt.completionDialogMaxWidth()) {
			dim.setSize(Opt.completionDialogMaxWidth(), dim.getHeight()+20);
		}
		setSize(dim);
		setLocation(s.recommendedLocation(s.getTextArea()));
		s.moveOnScreen(this);
		x = getX();
		y = getY();
		history = new LinkedList();
		history.addFirst(new HistoryElem(0, prefix));
		showSelection();
		setVisible(true);
	}

	static void showCompletionDialog(String completionList, DispatchData data, int noFocus) {
		// DO always new dialog, because in multiple views, the dialog
		// memorizes its parent
		if (lastcDialog==null) {
			lastcDialog = new CompletionDialog();
		} else if (lastcDialog.isVisible()) {
			lastcDialog.setVisible(false);
			lastcDialog = new CompletionDialog();
		} else {
			lastcDialog = new CompletionDialog();
		}
		lastcDialog.setCompletionDialog(completionList, data);
		/*&
		  lastcDialog.urlBrowserOpenedAtStart = s.view.getDockableWindowManager().
		  isDockableWindowVisible(s.dockableUrlBrowserWindowName);
		  &*/
	}

	CompletionDialog() {
		super(s.view, "");
		if (s.javaVersion.compareTo("1.4.0") >= 0) setUndecorated(true);
		textArea = new JTextArea();
		textArea.addKeyListener(this);
		textArea.addMouseListener(new CompletionMouseAdapter());
		textArea.addMouseMotionListener(new CompletionMouseMotionAdapter());
		scrollPane = new JScrollPane(textArea);
		setContentPane(scrollPane);
		//& setModal(true);
		enableEvents(AWTEvent.KEY_EVENT_MASK);
	}

}


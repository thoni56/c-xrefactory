package com.xrefactory.jedit;

import org.gjt.sp.jedit.*;
import java.io.*;
import org.gjt.sp.jedit.help.*;
import javax.swing.*;
import org.gjt.sp.jedit.io.*;
import java.awt.*;


public class Dispatch {


	static String currentTag;

	static int attrLen;
	static int attrLine;
	static int attrCol;
	static int attrOffset;
	static int attrVal;
	static int attrLevel;
	static int attrNumber;
	static int attrBeep;
	static int attrSelected;
	static int attrMType;
	static int attrBase;
	static int attrDRefn;
	static int attrRefn;
	static int attrIndent;
	static int attrInterface;
	static int attrDefinition;
	static int attrTreeUp;
	static int attrContinue;
	static int attrNoFocus;

	static String attrTreeDeps;
	static String attrType;
	static String attrVClass;
	static String attrSymbol;

	static String stringVal;

	static String block;

	static void clearAttributes() {
		attrLen = 0;
		attrLine = 0;
		attrCol = 0;
		attrOffset = 0;
		attrVal = 0;
		attrLevel = 0;
		attrNumber = 0;
		attrBeep = 0;
		attrSelected = 0;
		attrMType = 0;
		attrBase = 0;
		attrDRefn = 0;
		attrRefn = 0;
		attrIndent = 0;
		attrInterface = 0;
		attrDefinition = 0;
		attrTreeUp = 0;
		attrNoFocus = 0;

		attrTreeDeps = null;
		attrType = null;
		attrVClass = null;
	}

	static void browseUrlIfConfirmed(String url, DispatchData data) {
		// sometimes it throws exception FileNotFoundor or what
		try {
			int confirm = JOptionPane.YES_OPTION;
			if (Opt.askBeforeBrowsingJavadoc()) {
				confirm = JOptionPane.showConfirmDialog(
					s.getProbableParent(data.callerComponent), 
					"Browse URL: "+url+"?", "Confirmation", 
					JOptionPane.YES_NO_OPTION,
					JOptionPane.QUESTION_MESSAGE);
			}
			if (confirm == JOptionPane.YES_OPTION) {
				s.browseUrl(url, false, s.getParentView(data.callerComponent));
			}
		} catch (Exception e) {}
	}

	public static void editorPreCheck(String str) throws XrefException {
		Buffer buffer = s.getBuffer();
		int caret = s.getCaretPosition();
		String text = buffer.getText(caret, str.length());
		if (! text.equals(str)) {
			throw new XrefException("expecting string "+str+" at this place, probably not updated references?");
		}
	}

	public static void editorReplace(String str, String with) throws XrefException {
		Buffer buffer = s.getBuffer();
		int caret = s.getCaretPosition();
		String text = buffer.getText(caret, str.length());
		if (! text.equals(str)) {
			throw new XrefException("should replace "+str+" with "+with+", but find "+text+" on this place");
		}
		buffer.remove(caret, str.length());
		buffer.insert(caret, with);
	}

	static void protocolCheckEq(String s1, String s2) throws Exception {
		if (! s1.equals(s2)) {
			throw new XrefException("Internal protocol error, got '"+s1+"' while expecting '"+s2+"'");
		}
	}

	public static int skipMyXmlIdentifier(XrefCharBuffer ss, int i, int len) {
		char	c;
		c = ss.buf[i];
		while (i<len && (Character.isLetterOrDigit(c) || c=='-')) {
			i++;
			c = ss.buf[i];
		}
		return(i);
	}

	public static int skipAttribute(XrefCharBuffer ss, int i, int len) {
		char	c;
		c = ss.buf[i];
		if (c=='"') {
			c = ' ';
			while (i<len && c!='"' && c!='\n') {
				i++;
				c = ss.buf[i];
			}
			if (i<len && c=='"') i++;
		} else {
			while (i<len && c!='>' && (! Character.isWhitespace(c))) {
				i++;
				c = ss.buf[i];
			}
		}
		return(i);
	}

	public static String unquote(String a) {
		int len = a.length();
		if (len>=2 && a.charAt(0)=='"' && a.charAt(len-1)=='"') {
			return(a.substring(1,len-1));
		} else {
			return(a);
		}
	}

	public static String getContext(XrefCharBuffer ss, int i, int len) {
		int j = i+20;
		String suffix="";
		if (j>=len) j=len;
		else suffix=" ...";
		return(ss.substring(i,j)+suffix);
	}

	public static int parseXmlTag(XrefCharBuffer ss, int i, int len) throws Exception {
		int 	j;
		char	c;
		String	attrib, attrval;
		i = s.skipBlank(ss, i, len);
		if (i>=len) throw new XrefException("parsing beyond end of communication string");
		if (ss.buf[i]!='<') {
			throw new XrefException("tag does not start with '<' ("+getContext(ss,i,len)+")");
		}
		i++;
		j = i;
		if (ss.buf[i]=='/') {
			// closing tag
			i++;
		}
		i = skipMyXmlIdentifier(ss, i, len);
		currentTag = ss.substring(j, i);
//&System.err.println("Tag == " + currentTag + "\n");		
		i = s.skipBlank(ss, i, len);
		while (ss.buf[i]!='>') {
			j=i; i=skipMyXmlIdentifier(ss, i, len);
			attrib = ss.substring(j,i);
			i = s.skipBlank(ss, i, len);
			if (ss.buf[i]!='=') throw new XrefException("= expected after attribute " + attrib);
			i++;
			j=i; i = skipAttribute(ss, i, len);
			attrval = ss.substring(j,i);
			// Integer.parseInt(ss.substring(j,i));
			// TODO!!!! do this via some hash table or what
			// this is pretty inefficient
			if (false) {
			} else if (attrib.equals(Protocol.PPCA_VCLASS)) {
				attrVClass = unquote(attrval);
			} else if (attrib.equals(Protocol.PPCA_BASE)) {
				attrBase = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_COL)) {
				attrCol = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_DEFINITION)) {
				attrDefinition = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_DEF_REFN)) {
				attrDRefn = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_INDENT)) {
				attrIndent = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_INTERFACE)) {
				attrInterface = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_LEN)) {
				attrLen = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_LINE)) {
				attrLine = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_OFFSET)) {
				attrOffset = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_REFN)) {
				attrRefn = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_SELECTED)) {
				attrSelected = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_TREE_DEPS)) {
				attrTreeDeps = unquote(attrval);
			} else if (attrib.equals(Protocol.PPCA_TREE_UP)) {
				attrTreeUp = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_MTYPE)) {
				attrMType = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_TYPE)) {
				attrType = unquote(attrval);
			} else if (attrib.equals(Protocol.PPCA_BEEP)) {
				attrBeep = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_VALUE)) {
				attrVal = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_LEVEL)) {
				attrLevel = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_NUMBER)) {
				attrNumber = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_CONTINUE)) {
				attrContinue = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_NO_FOCUS)) {
				attrNoFocus = Integer.parseInt(attrval);
			} else if (attrib.equals(Protocol.PPCA_SYMBOL)) {
				attrSymbol = unquote(attrval);
			} else if (s.debug) {
				throw new XrefException("unknown XML attribute '" + attrib + "'");
			}
			i = s.skipBlank(ss, i, len);
		}
		i++;
		return(i);
	}

	public static int parseXmlTagHandleMessages(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		boolean loop = true;
		while (loop) {
			clearAttributes();
			i = parseXmlTag(ss, i, len);
			if (false) {
			} else if (currentTag.equals(Protocol.PPC_ERROR)) {
				i = dispatchError(ss, i, len, data);
			} else if (currentTag.equals(Protocol.PPC_FATAL_ERROR)) {
				i = dispatchFatalError(ss, i, len, data);
			} else if (currentTag.equals(Protocol.PPC_WARNING)) {
				i = dispatchWarning(ss, i, len, data);
			} else if (currentTag.equals(Protocol.PPC_DEBUG_INFORMATION)) {
				i = dispatchDebugMessage(ss, i, len, data);
			} else if (currentTag.equals(Protocol.PPC_INFORMATION)) {
				i = dispatchMessage(ss, i, len, data);
			} else if (currentTag.equals(Protocol.PPC_BOTTOM_INFORMATION)) {
				i = dispatchBottomMessage(ss, i, len, data, Protocol.PPC_BOTTOM_INFORMATION);
			} else if (currentTag.equals(Protocol.PPC_BOTTOM_WARNING)) {
				i = dispatchBottomMessage(ss, i, len, data, Protocol.PPC_BOTTOM_WARNING);
			} else if (currentTag.equals(Protocol.PPC_IGNORE)) {
				i = dispatchIgnore(ss, i, len, data);
			} else if (currentTag.equals(Protocol.PPC_LICENSE_ERROR)) {
				i = dispatchLicenseError(ss, i, len, data);
			} else {
				loop = false;
			}
			if (loop) {
				i = s.skipBlank(ss, i, len);
				if (i>=len) loop = false;
			}
		}
		return(i);
	}

	public static int dispatchError(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_ERROR);
		throw new XrefErrorException(message);
		//&return(i);
	}

	public static int dispatchLicenseError(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_LICENSE_ERROR);
		int answer = JOptionPane.showOptionDialog(s.getProbableParent(data.callerComponent), 
												  message, 
												  "Xrefactory Warning", 
												  JOptionPane.OK_CANCEL_OPTION,
												  JOptionPane.WARNING_MESSAGE,
												  null,
												  new String[]{"Browse Url","Continue"},
												  "Continue"
			);
		if (answer == 0) {
			s.browseUrl(s.xrefRegistrationUrl, true, s.getParentView(data.callerComponent));
		}
		return(i);
	}

	public static int dispatchFatalError(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_FATAL_ERROR);
		throw new XrefErrorException(message);
		//&return(i);
	}

	public static int dispatchWarning(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_WARNING);
		JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), 
									  message, "Xrefactory Warning", JOptionPane.WARNING_MESSAGE);
		return(i);
	}

	public static int dispatchMessage(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_INFORMATION);
		JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), 
									  message, "Xrefactory Info", JOptionPane.INFORMATION_MESSAGE);
		return(i);
	}

	public static int dispatchDebugMessage(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_DEBUG_INFORMATION);
		if (s.debug) {
			JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), 
										  message, "Xrefactory Debug Info", JOptionPane.INFORMATION_MESSAGE);
		}
		return(i);
	}

	public static int dispatchBottomMessage(XrefCharBuffer ss, int i, int len, DispatchData data, String ctag) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		if (attrBeep!=0) s.view.getToolkit().beep();
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+ctag);
		//&s.getParentView(data.callerComponent).getStatus().setMessageAndClear(message);
		SwingUtilities.invokeLater(new s.MessageDisplayer(message,true));
		return(i);
	}

	public static int dispatchAskConfirmation(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_ASK_CONFIRMATION);
		int confirm = JOptionPane.showConfirmDialog(s.getProbableParent(data.callerComponent), 
													message, "Confirmation", 
													JOptionPane.YES_NO_OPTION,
													JOptionPane.WARNING_MESSAGE);
		if (confirm != JOptionPane.YES_OPTION) throw new XrefAbortException();
		return(i);
	}

	public static int dispatchIgnore(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_IGNORE);
		return(i);
	}

	// unused to be deleted
	public static int dispatchFileSaveAs(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String fname = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_FILE_SAVE_AS);
		s.getBuffer().save(s.view, fname, true);
		try {Thread.currentThread().sleep(35);} catch (InterruptedException e){}
		VFSManager.waitForRequests();
		return(i);
	}

	// unused to be deleted
	public static int dispatchMoveDirectory(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_STRING_VALUE);
		String oldname = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_STRING_VALUE);
		String newname = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_MOVE_DIRECTORY);
		int confirm = JOptionPane.showConfirmDialog(
			s.getProbableParent(data.callerComponent), 
			"\tCan I move directory\n"+oldname+"\n\tto\n"+newname,
			"Confirmation", JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE);
		if (confirm == JOptionPane.YES_OPTION) {
			File ff = new File(oldname);
			ff.renameTo(new File(newname));
		}
		return(i);
	}

	public static int dispatchMoveFileAs(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String fname = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_MOVE_FILE_AS);
		File ff = new File(fname);
		ff.getParentFile().mkdirs();
		File oldfile = new File(s.getBuffer().getPath());
		oldfile.delete();
		if (s.getBuffer().save(s.view, fname, true)) {
			// sleeping is a hack fixing jEdit dead-lock problem
			try {Thread.currentThread().sleep(35);} catch (InterruptedException e){}
			VFSManager.waitForRequests();
			Buffer oldb = jEdit.getBuffer(oldfile.getAbsolutePath());
			if (oldb!=null && oldb!=s.getBuffer()) {
				jEdit.closeBuffer(s.view, oldb);
			}
			try {Thread.currentThread().sleep(15);} catch (InterruptedException e){}
			VFSManager.waitForRequests();
		} else {
			throw new XrefException("can not move the file");
		}
		return(i);
	}

	public static int dispatchGoto(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		i = parseXmlTagHandleMessages(ss, i, len,data);
		if (currentTag.equals(Protocol.PPC_OFFSET_POSITION)) {
			String path =  ss.substring(i, i+attrLen);
			int offset = attrOffset;
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_OFFSET_POSITION);
			s.moveToPosition(s.getParentView(data.callerComponent), path, offset);
		} else {
			protocolCheckEq(currentTag, Protocol.PPC_LC_POSITION);
			String path =  ss.substring(i, i+attrLen);
			int line = attrLine;
			int col = attrCol;
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_LC_POSITION);
			s.moveToPosition(s.getParentView(data.callerComponent), path, line, col);
		}
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_GOTO);
		return(i);
	}

	public static int dispatchPreCheck(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String str =  ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_REFACTORING_PRECHECK);
		editorPreCheck(str);
		return(i);
	}

	public static int dispatchString(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_STRING_VALUE);
		stringVal =  ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);
		return(i);
	}

	public static int dispatchReplacement(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		i = dispatchString(ss, i, len, data);
		String str = stringVal;
		i = dispatchString(ss, i, len, data);
		String with = stringVal;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_REFACTORING_REPLACEMENT);
		editorReplace(str, with);
		return(i);
	}

	public static int dispatchDisplayResolution(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		boolean cont = attrContinue!=0;
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_DISPLAY_RESOLUTION);
		// Frame did not work, as it looses pointer to s.view
		//&if (cont) {
		new ResolutionDialog(message, attrMType, data, cont);
		//&} else {
		//&new ResolutionFrame(message, attrMType, data, cont);
		//&throw new XrefAbortException();
		//&}
		return(i);
	}

	public static int dispatchDisplayOrUpdateBrowser(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_DISPLAY_OR_UPDATE_BROWSER);
		s.showAndUpdateBrowser(s.getParentView(data.callerComponent));
		return(i);
	}

/*& // old way of displaying class tree
  public static int dispatchDisplayClassTree(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
  XrefCharBuffer 	tree = new XrefCharBuffer();
  int 			minLine = 1000000;
  int		 		cline = -1;
  String			prefix;
  int				callerLine = 0;
  clearAttributes();
  i = parseXmlTagHandleMessages(ss, i, len,data);
  while (currentTag.equals(Protocol.PPC_CLASS)) {
  String ctclass = ss.substring(i, i+attrLen);
  i += attrLen;
  cline ++;
  if (cline != 0) tree.append("\n");
  if (attrBase==1) {
  prefix = ">>";
  if (callerLine==0) callerLine = cline;
  } else {
  prefix = "  ";
  }
  if (attrTreeUp==1) {
  tree.append(prefix + attrTreeDeps + "(" + ctclass + ")  ");
  } else {
  tree.append(prefix + attrTreeDeps + ctclass + "  ");
  }
  if (minLine>attrLine) minLine = attrLine;
  i = parseXmlTagHandleMessages(ss, i, len,data);
  protocolCheckEq(currentTag, "/"+Protocol.PPC_CLASS);
  clearAttributes();
  i = parseXmlTagHandleMessages(ss, i, len,data);
  }
  protocolCheckEq(currentTag, "/"+Protocol.PPC_DISPLAY_CLASS_TREE);
  new DockableClassTree(s.getParentFrame(data.callerComponent), tree.toString(), data, minLine, callerLine);
  return(i);
  }
  &*/

	public static int dispatchDisplayClassTree(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		XrefTreeNode [] tt = new XrefTreeNode[s.XREF_MAX_TREE_DEEP];
		tt[0] = new XrefTreeNode("root", null, 0,0,0, false,false,false,false,false,false);
		i = dispatchParseOneSymbolClassHierarchy(ss, i, len, data, tt, "");
		protocolCheckEq(currentTag, "/"+Protocol.PPC_DISPLAY_CLASS_TREE);
		s.view.getDockableWindowManager().showDockableWindow(s.dockableClassTreeWindowName);
		DockableClassTree ctViewer = s.getClassTreeViewer(s.view);
		ctViewer.setTree(tt[0]);
		return(i);
	}

	static int dispatchParseOneSymbolClassHierarchy(XrefCharBuffer ss, int i, int len, DispatchData data, XrefTreeNode [] tt, String symbol) throws Exception {
		tt[1] = new XrefTreeNode(symbol, tt[0], 0,0,0, false,false,false,true,false,false);
		clearAttributes();
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_CLASS)) {
			String ctclass = ss.substring(i, i+attrLen);
			i += attrLen;
			int deep = attrIndent+2;
			s.assertt(deep < s.XREF_MAX_TREE_DEEP);
			tt[deep] = new XrefTreeNode(ctclass, tt[deep-1], 
										attrLine, attrRefn, attrDRefn,
										attrBase==1,attrSelected==1,attrInterface==1, 
										false,attrDefinition==1, false);
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_CLASS);
			clearAttributes();
			i = parseXmlTagHandleMessages(ss, i, len,data);
		}
		return(i);
	}

	static int dispatchParseTreeDescription(XrefCharBuffer ss, int i, int len, DispatchData data, XrefTreeNode [] tt) throws Exception {
		tt[0] = new XrefTreeNode("root", null, 0,0,0, false,false,false,false,false,false);
		XrefTreeNode nonVirtuals = new XrefTreeNode(s.XREF_NON_MEMBER_SYMBOL_NAME, tt[0], 
													0, 0, 0, false,false,false,true,false,false);
		
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_VIRTUAL_SYMBOL)
			   || currentTag.equals(Protocol.PPC_SYMBOL)) {
			String symbol = ss.substring(i, i+attrLen);
			i += attrLen;
			if (currentTag.equals(Protocol.PPC_SYMBOL)) {
				new XrefTreeNode(symbol, nonVirtuals,
								 attrLine, attrRefn, attrDRefn,
								 attrBase==1,attrSelected==1,attrInterface==1, 
								 false,attrDefinition==1, false);
				i = parseXmlTagHandleMessages(ss, i, len,data);
				protocolCheckEq(currentTag, "/"+Protocol.PPC_SYMBOL);
				clearAttributes();
				i = parseXmlTagHandleMessages(ss, i, len,data);
			} else {
				i = parseXmlTagHandleMessages(ss, i, len,data);
				protocolCheckEq(currentTag, "/"+Protocol.PPC_VIRTUAL_SYMBOL);
				i = dispatchParseOneSymbolClassHierarchy(ss, i, len, data, tt, symbol);
			}
		}
		if (nonVirtuals.subNodes.size()==0) tt[0].removeSubNode(nonVirtuals);
		return(i);
	}

	public static int dispatchSymbolResolution(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		XrefTreeNode [] tt = new XrefTreeNode[s.XREF_MAX_TREE_DEEP];
		i = dispatchParseTreeDescription(ss, i, len, data, tt);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_SYMBOL_RESOLUTION);
		BrowserTopPanel bpanel = s.getParentBrowserTopPanel(data.callerComponent);
		s.assertt(bpanel!=null);
		bpanel.treePanel.xtree.setTree(tt[0]);
		return(i);
	}

	public static int dispatchReferenceList(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		boolean firstFlag = true;
		XrefCharBuffer refs = new XrefCharBuffer();
		int crefn = attrVal;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_SRC_LINE)) {
			if (! firstFlag) refs.append("\n");
			firstFlag = false;
			for(int j=0; j<attrRefn; j++) {
				if (j!=0) refs.append("\n");
				refs.append(ss, i, attrLen);
			}
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_SRC_LINE);
			i = parseXmlTagHandleMessages(ss, i, len,data);
		}
		protocolCheckEq(currentTag, "/"+Protocol.PPC_REFERENCE_LIST);
		BrowserTopPanel bpanel = s.getParentBrowserTopPanel(data.callerComponent);
		s.assertt(bpanel!=null);
		bpanel.referencesPanel.reflist.setRefs(refs.toString(), crefn);
		return(i);
	}

	public static int dispatchUpdateCurrentReference(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		int val = attrVal;
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_UPDATE_CURRENT_REFERENCE);
		BrowserTopPanel bpanel = s.getParentBrowserTopPanel(data.callerComponent);
		s.assertt(bpanel!=null);
		bpanel.referencesPanel.reflist.setCurrentRef(val);
		return(i);
	}

	public static int dispatchProgress(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		// TDODO!!!
		//&ProgressMonitor progress = new ProgressMonitor(s.getProbableParent(data.callerComponent), "Updating references", null, 0, 100);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_PROGRESS);
		//&progress.close();
		return(i);
	}

	public static int dispatchUpdateReport(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		// O.K. skip everything until end of report
		// TODO, do this seriously, counting eventual nestings
		i += attrLen;
		clearAttributes();
		i = parseXmlTag(ss, i, len);
		while (i<len && ! currentTag.equals("/"+Protocol.PPC_UPDATE_REPORT)) {
			i += attrLen;
			clearAttributes();
			i = parseXmlTag(ss, i, len);
			// make an exception and report fatal error, as task is exited now
			if (currentTag.equals(Protocol.PPC_FATAL_ERROR)) {
				i = dispatchFatalError(ss, i, len, data);
			}
		}
		protocolCheckEq(currentTag, "/"+Protocol.PPC_UPDATE_REPORT);		
		return(i);
	}

	public static int dispatchVersionMismatch(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String taskVersion = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTag(ss, i, len);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_VERSION_MISMATCH);
		s.checkVersionCorrespondance(taskVersion, data);
		return(i);
	}

	public static int dispatchBrowseUrl(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_BROWSE_URL);
		browseUrlIfConfirmed(message, data);
		//&new HelpViewer(message);
		return(i);
	}

	public static int dispatchSingleCompletion(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String completion = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_SINGLE_COMPLETION);
		int caret = s.getCaretPosition();
		s.insertCompletion(s.getBuffer(), caret, completion);
		return(i);
	}

	public static int dispatchFqtCompletion(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String completion = ss.substring(i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_FQT_COMPLETION);
		int caret = s.getCaretPosition();
		CompletionDialog.insertFqtCompletion(s.getBuffer(), caret, completion);
		return(i);
	}

	public static int dispatchCompletionList(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		boolean firstFlag = true;
		XrefCharBuffer completions = new XrefCharBuffer();
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_MULTIPLE_COMPLETION_LINE)) {
			if (! firstFlag) completions.append("\n");
			firstFlag = false;
			completions.append(ss, i, attrLen);
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_MULTIPLE_COMPLETION_LINE);
			i = parseXmlTagHandleMessages(ss, i, len,data);
		}
		protocolCheckEq(currentTag, "/"+Protocol.PPC_MULTIPLE_COMPLETIONS);
		CompletionDialog.showCompletionDialog(completions.toString(), data, attrNoFocus);
		return(i);
	}

	public static int dispatchFullCompletionList(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		int ci, max;
		ci = 0;
		max = attrNumber;
		CompletionDialog3 cd =  CompletionDialog3.initCompletionDialog(data, max, attrNoFocus);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_MULTIPLE_COMPLETION_LINE)) {
			s.assertt(ci < max);
			cd.lns[ci] = new CompletionDialog3.LineData(cd.completions.bufi,
														cd.completions.bufi + attrVal,
														attrVClass);
			cd.completions.append(ss, i, attrLen);
			ci ++;
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_MULTIPLE_COMPLETION_LINE);
			i = parseXmlTagHandleMessages(ss, i, len,data);
		}
		cd.lns[ci] = new CompletionDialog3.LineData(cd.completions.bufi, -1, "");
		protocolCheckEq(currentTag, "/"+Protocol.PPC_FULL_MULTIPLE_COMPLETIONS);
		CompletionDialog3.showCompletionDialog();
		return(i);
	}

	public static int dispatchAllCompletionsList(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		XrefCharBuffer completions = new XrefCharBuffer();
		completions.append(ss, i, attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_ALL_COMPLETIONS);
		CompletionDialog.showCompletionDialog(completions.toString(), data, attrNoFocus);
		return(i);
	}

	public static int dispatchSymbolList(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		XrefCharBuffer symbols = new XrefCharBuffer();
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_STRING_VALUE)) {
			symbols.append(ss, i, attrLen);
			symbols.append("\n");
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);
			i = parseXmlTagHandleMessages(ss, i, len,data);
		}
		protocolCheckEq(currentTag, "/"+Protocol.PPC_SYMBOL_LIST);
		data.symbolList = symbols;
		return(i);
	}

	/* TODO, delete this method, it is useless since 1.6.0 */
	public static int dispatchSetProject(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		data.info = ss.substring( i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_SET_PROJECT);
		return(i);
	}

	public static int dispatchSetInfo(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		data.info = ss.substring( i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_SET_INFO);
		return(i);
	}

	public static int dispatchNoProject(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		int rr;
		String file = ss.substring( i, i+attrLen);
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		if (data.projectCreationAllowed) {
			protocolCheckEq(currentTag, "/"+Protocol.PPC_NO_PROJECT);
			rr = JOptionPane.showConfirmDialog(s.getProbableParent(data.callerComponent), 
											   "No project for file "+file+".\nCreate new project?" , "No Project", JOptionPane.YES_NO_OPTION);
			if (rr==JOptionPane.YES_OPTION) {
				OptionsForProjectsDialog od = new OptionsForProjectsDialog(s.view, true);
			}
		}
		throw new XrefAbortException();
	}

	public static int dispatchNoSymbol(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		// handle no symbol by pushing symbol by name
		String name = s.getIdentifierOnCaret();
		i += attrLen;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_NO_SYMBOL);
		new Push(new String[] {"-olcxpushname="+name, "-olnodialog"}, data);
		return(i);
	}

	public static int dispatchAvailableRefactorings(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		boolean firstFlag = true;
		Refactorings refactors[] = new Refactorings[Protocol.PPC_MAX_AVAILABLE_REFACTORINGS];
		int refactoringsi = 0;
		i = parseXmlTagHandleMessages(ss, i, len,data);
		while (currentTag.equals(Protocol.PPC_INT_VALUE)) {
			Refactorings rrr = Refactorings.getRefactoringFromCode(attrVal);
			if (rrr!=null) {
				s.assertt(refactoringsi < Protocol.PPC_MAX_AVAILABLE_REFACTORINGS);
				refactors[refactoringsi] = rrr;
				refactors[refactoringsi].param = ss.substring(i, i+attrLen);
				refactoringsi++;
			}
			i += attrLen;
			i = parseXmlTagHandleMessages(ss, i, len,data);
			protocolCheckEq(currentTag, "/"+Protocol.PPC_INT_VALUE);
			i = parseXmlTagHandleMessages(ss, i, len,data);
		}
		protocolCheckEq(currentTag, "/"+Protocol.PPC_AVAILABLE_REFACTORINGS);
		Refactorings[] refactorings = new Refactorings[refactoringsi];
		System.arraycopy(refactors, 0, refactorings, 0, refactoringsi);
		new RefactoringDialog(refactorings);
		return(i);
	}

	public static int dispatchCopyBlock(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		int cc = s.getCaretPosition();
		block = s.getBuffer().getText(cc, cc+attrVal);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_REFACTORING_COPY_BLOCK);
		return(i);
	}

	public static int dispatchCutBlock(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		Buffer buffer = s.getBuffer();
		int blen = buffer.getLength();
		int caret = s.getCaretPosition();
		// O.K. next line is to handle special case of final newline 
		// automatically added by jEdit to saved files
		if (caret+attrVal == blen+1) buffer.insert(blen, "\n");
		block = buffer.getText(caret, attrVal);
		buffer.remove(caret, attrVal);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_REFACTORING_CUT_BLOCK);
		return(i);
	}

	public static int dispatchPasteBlock(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		Buffer buffer = s.getBuffer();
		int caret = s.getCaretPosition();
		buffer.insert(caret, block);

		// do indentation somewhere later to not perturb refactorings offsets
		//&int blocklines = s.getNumberOfCharOccurences(block,'\n');
		//&int line = buffer.getLineOfOffset(caret);
		//&buffer.indentLines(buffer.getLineOfOffset(caret), line+blocklines);

		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_REFACTORING_PASTE_BLOCK);
		return(i);
	}

	public static int dispatchKillBufferRemoveFile(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String message = ss.substring( i, i+attrLen);
		i += attrLen;
		int confirm = JOptionPane.YES_OPTION;
		confirm = JOptionPane.showConfirmDialog(
			s.view,
			message,
			"Confirmation", 
			JOptionPane.YES_NO_OPTION,
			JOptionPane.QUESTION_MESSAGE);
		if (confirm == JOptionPane.YES_OPTION) {
			Buffer buf = s.getBuffer();
			buf.save(s.view, null);
			(new File(buf.getPath())).delete();
			jEdit.closeBuffer(s.view, buf);
		}
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_KILL_BUFFER_REMOVE_FILE);
		return(i);
	}

	public static int dispatchIndent(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		Buffer buffer = s.getBuffer();
		int caret = s.getCaretPosition();
		int line = buffer.getLineOfOffset(caret);
		buffer.indentLines(line, line+attrVal-1);
		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_INDENT);
		return(i);
	}

	public static int dispatchExtractDialog(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		String xtype = attrType;
		i += attrLen;

		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_STRING_VALUE);
		String invocation = ss.substring( i, i+attrLen);
		i += attrLen;
		i = parseXmlTag(ss, i, len);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);

		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_STRING_VALUE);
		String theHead = ss.substring( i, i+attrLen);
		i += attrLen;
		i = parseXmlTag(ss, i, len);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);

		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_STRING_VALUE);
		String theTail = ss.substring( i, i+attrLen);
		i += attrLen;
		i = parseXmlTag(ss, i, len);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_STRING_VALUE);

		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, Protocol.PPC_INT_VALUE);
		int targetLine = attrVal;
		i += attrLen;
		i = parseXmlTag(ss, i, len);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_INT_VALUE);

		i = parseXmlTagHandleMessages(ss, i, len,data);
		protocolCheckEq(currentTag, "/"+Protocol.PPC_EXTRACTION_DIALOG);
		new ExtractMethodDialog(invocation, theHead, theTail, targetLine, xtype);
		return(i);
	}

	public static int dispatchRecordAt(XrefCharBuffer ss, int i, int len, DispatchData data) throws Exception {
		int j;
		String unenclosed;
		i = s.skipBlank(ss, i, len);
		for(j=i; ss.buf[i] != '<' && i<len; i++) ;
		if (i!=j && s.debug) {
			unenclosed = ss.substring(j, i);
			JOptionPane.showMessageDialog(s.view, "Unenclosed string\n" + unenclosed, 
										  "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
		}
		if (j>=len) throw new XrefException("unexpected end of communication");
		clearAttributes();
		i = parseXmlTagHandleMessages(ss, i, len,data);
		if (i>=len || data.panic) {
			// no action in such cases
		} else if (currentTag.equals(Protocol.PPC_SINGLE_COMPLETION)) {
			i = dispatchSingleCompletion(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_FQT_COMPLETION)) {
			i = dispatchFqtCompletion(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_MULTIPLE_COMPLETIONS)) {
			i = dispatchCompletionList(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_FULL_MULTIPLE_COMPLETIONS)) {
			i = dispatchFullCompletionList(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_ALL_COMPLETIONS)) {
			i = dispatchAllCompletionsList(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_SYMBOL_LIST)) {
			i = dispatchSymbolList(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_SET_PROJECT)) {
			i = dispatchSetProject(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_SET_INFO)) {
			i = dispatchSetInfo(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_NO_PROJECT)) {
			i = dispatchNoProject(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_NO_SYMBOL)) {
			i = dispatchNoSymbol(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_UPDATE_CURRENT_REFERENCE)) {
			i = dispatchUpdateCurrentReference(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_BROWSE_URL)) {
			i = dispatchBrowseUrl(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_GOTO)) {
			i = dispatchGoto(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_REFACTORING_PRECHECK)) {
			i = dispatchPreCheck(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_REFACTORING_REPLACEMENT)) {
			i = dispatchReplacement(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_DISPLAY_OR_UPDATE_BROWSER)) {
			i = dispatchDisplayOrUpdateBrowser(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_DISPLAY_CLASS_TREE)) {
			i = dispatchDisplayClassTree(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_DISPLAY_RESOLUTION)) {
			i = dispatchDisplayResolution(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_SYMBOL_RESOLUTION)) {
			i = dispatchSymbolResolution(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_REFERENCE_LIST)) {
			i = dispatchReferenceList(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_ASK_CONFIRMATION)) {
			i = dispatchAskConfirmation(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_FILE_SAVE_AS)) {
			i = dispatchFileSaveAs(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_MOVE_FILE_AS)) {
			i = dispatchMoveFileAs(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_MOVE_DIRECTORY)) {
			i = dispatchMoveDirectory(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_AVAILABLE_REFACTORINGS)) {
			i = dispatchAvailableRefactorings(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_REFACTORING_COPY_BLOCK)) {
			i = dispatchCopyBlock(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_REFACTORING_CUT_BLOCK)) {
			i = dispatchCutBlock(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_REFACTORING_PASTE_BLOCK)) {
			i = dispatchPasteBlock(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_KILL_BUFFER_REMOVE_FILE)) {
			i = dispatchKillBufferRemoveFile(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_INDENT)) {
			i = dispatchIndent(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_EXTRACTION_DIALOG)) {
			i = dispatchExtractDialog(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_UPDATE_REPORT)) {
			i = dispatchUpdateReport(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_VERSION_MISMATCH)) {
			i = dispatchVersionMismatch(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_PROGRESS)) {
			i = dispatchProgress(ss, i, len, data);
		} else if (currentTag.equals(Protocol.PPC_SYNCHRO_RECORD)) {
			// this is usualy a problem, but ignore it for debuging
		} else {
			throw new XrefException("unknown XML Tag " + currentTag);
		}
		return(i);
	}

	public static void dispatch(XrefCharBuffer ss, DispatchData data) {
		//&JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), ss, "Got", JOptionPane.INFORMATION_MESSAGE);
		if (s.debug) System.err.println("Dispatching: " + ss);
		int i = 0;
		int len = ss.length();
		try {
			i = s.skipBlank(ss, i, len);
			while (i<len && ! data.panic) {
				i = dispatchRecordAt(ss, i, len, data);
				i = s.skipBlank(ss, i, len);
			}
		} catch (XrefAbortException e) {
			data.panic = true;
		} catch (XrefErrorException e) {
			if (s.debug) e.printStackTrace(System.err);
			JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), 
										  e, "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
			data.panic = true;
		} catch (Exception e) {
			JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), 
										  e, "Xrefactory Internal Error", JOptionPane.ERROR_MESSAGE);
			e.printStackTrace();
			data.panic = true;
		}
	}

	public static String reportToHtml(XrefCharBuffer ss) {
		XrefCharBuffer res = new XrefCharBuffer();
		int i, len;
		i = 0;
		len = ss.length();
		res.append("<html><pre>\n");
		try {
			while (i<len) {
				clearAttributes();
				i = parseXmlTag(ss, i, len);
				if (currentTag.equals(Protocol.PPC_ERROR) 
					|| currentTag.equals(Protocol.PPC_FATAL_ERROR)
					|| currentTag.equals(Protocol.PPC_WARNING)
					) {
					res.append("<font color=red>");
					int j = attrLen;
					while (j>0 && Character.isWhitespace(ss.buf[i+j-1])) j--;
					res.append(ss.substring(i, i+j));
					res.append("</font>\n");
					i += attrLen;
					i = parseXmlTag(ss, i, len);
				} else if (currentTag.equals(Protocol.PPC_INFORMATION)
						   || currentTag.equals(Protocol.PPC_BOTTOM_INFORMATION)
					) {
					res.append("<font color=gray>");
					res.append(ss.substring(i, i+attrLen));
					res.append("</font>\n");
					i += attrLen;
					i = parseXmlTag(ss, i, len);
				} else {
					res.append(ss.substring(i, i+attrLen));
					res.append("\n");
					i += attrLen;
				}
				i = s.skipBlank(ss, i, len);
			}
		} catch(Exception e) {
			e.printStackTrace(System.err);
			JOptionPane.showMessageDialog(s.view, 
										  "Problem while parsing report " + e, 
										  "Xrefactory Error", 
										  JOptionPane.ERROR_MESSAGE);
		}
		res.append("\n</pre></html>\n");
		if (s.debug) System.err.println("HTML == \n" + res.toString());
		return(res.toString());
	}


}


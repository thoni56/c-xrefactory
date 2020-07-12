package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.text.*;

class XrefRefListArea extends JTextArea {

	DispatchData 	data;
	int 			lastSelectedLine = -1;

	void renewSelection() {
		try {
			int cp = getCaretPosition();
			int ln = getLineOfOffset(cp);
			if (ln != lastSelectedLine) {
				lastSelectedLine = ln;
				DispatchData ndata = new DispatchData(data, this);
				XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxgoto" + (ln+1), ndata);
				Dispatch.dispatch(receipt, ndata);
			}
			setCaretPosition(getLineEndOffset(ln)-1);
			moveCaretPosition(getLineStartOffset(ln));
			getCaret().setSelectionVisible(true);
		} catch (Exception ex) {
		}
	}

	class MouseAdapter extends java.awt.event.MouseAdapter
		implements MouseMotionListener, KeyListener  {

		public void  mouseClicked( MouseEvent e) {
			// setting lastSelectedline ensures sending goto request (it is needed)
			lastSelectedLine = -1;
			renewSelection();
			e.consume();
		}
		public void  mouseDragged( MouseEvent e) {
			renewSelection();
			e.consume();
		}
		public void  mouseMoved( MouseEvent e) {}

		public void  keyPressed( KeyEvent e) {
			int code = e.getKeyCode();
			if (code == KeyEvent.VK_UP) {
				previous();
				e.consume();
			} else if (code == KeyEvent.VK_DOWN) {
				next();
				e.consume();		
			}
		}
		public void  keyReleased(KeyEvent e){
			renewSelection();
		}
		public void  keyTyped( KeyEvent e) {}
	}

	public void previous() {
		try {
			int cp = getCaretPosition();
			int ln = getLineOfOffset(cp);
			int lb = getLineStartOffset(ln);
			moveCaretPosition(lb-1);
			renewSelection();
		} catch (Exception e) {}
	}
	public void next() {
		try {
			int cp = getCaretPosition();
			int ln = getLineOfOffset(cp);
			int le = getLineEndOffset(ln);
			moveCaretPosition(le+1);
			renewSelection();
		} catch (Exception e) {}
	}

	void highlightReference(int actn) {
		try {
			moveCaretPosition(getLineStartOffset(actn)+1); 
		} catch(Exception e) {}
		// following ensures to not resent goto request!
		lastSelectedLine = actn;
		renewSelection();
	}

	public void setCurrentRef(int n) {
		highlightReference(n);
	}


	public void setRefs(String list, int actn) {
		setText(list);
		repaint();
		highlightReference(actn);
	}
	
	XrefRefListArea(DispatchData data) {
		this("", data);
	}

	XrefRefListArea(String list, DispatchData data) {
		super(list);
		this.data = data;
		setEditable(false);
		XrefRefListArea.MouseAdapter ma = new XrefRefListArea.MouseAdapter();
		addMouseListener(ma);
		addMouseMotionListener(ma);
		addKeyListener(ma);
	}
}




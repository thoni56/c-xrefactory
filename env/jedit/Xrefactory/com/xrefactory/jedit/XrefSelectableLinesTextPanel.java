package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.text.*;
import java.awt.event.*;

public class XrefSelectableLinesTextPanel extends JTextArea {

	DispatchData 	data;
	int				lineOffset;
	int 			lastSelectedLine = -1;
	String			inspectOption;
	String 			result="";

	void renewSelection() {
		try {
			int cp = getCaretPosition();
			int ln = getLineOfOffset(cp);
			if (ln != lastSelectedLine) {
				lastSelectedLine = ln;
			}
			setCaretPosition(getLineEndOffset(ln)-1);
			moveCaretPosition(getLineStartOffset(ln));
			getCaret().setSelectionVisible(true);
		} catch (Exception ex) {
		}
	}

	void inspectLine() {
		try {
			int cp = getCaretPosition();
			int ln = getLineOfOffset(cp);
			result = getText(getLineStartOffset(ln), getLineEndOffset(ln)-getLineStartOffset(ln)-1);
			if (inspectOption!=null && !inspectOption.equals("")) {
				DispatchData ndata = new DispatchData(data, this);
				XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt(inspectOption + (ln+lineOffset), ndata);
				Dispatch.dispatch(receipt, ndata);
				renewSelection();
			} else {
				s.getProbableParent(XrefSelectableLinesTextPanel.this).setVisible(false);
			}
		} catch (Exception ee) {}
	}

	class XrefClassTreeMouseAdapter extends MouseInputAdapter implements KeyListener  {
		void moveSelection(MouseEvent e) {
			int y = e.getY();
			int rh = getRowHeight();
			try {
				setCaretPosition(getLineStartOffset(y/rh));
				renewSelection();
			} catch (Exception ex) {}
		}
		public void  mouseClicked(MouseEvent e) {
			inspectLine();
		}
		public void  mouseDragged(MouseEvent e) {
			moveSelection(e);
		}
		public void  mouseMoved(MouseEvent e) {
			moveSelection(e);
		}
		public void  keyPressed(KeyEvent e) {
			int code = e.getKeyCode();
			if (code==KeyEvent.VK_ESCAPE) {
				result = "";
				JDialog pd = s.getParentDialog(XrefSelectableLinesTextPanel.this);
				if (pd!=null) pd.setVisible(false);
			} else if (code==KeyEvent.VK_ENTER || code==KeyEvent.VK_SPACE) {
				inspectLine();
			}
		}
		public void  keyReleased(KeyEvent e){
			renewSelection();
			e.consume();
		}
		public void  keyTyped(KeyEvent e) {
		}
	}

	public Dimension countPreferredSize() {
		int nlc = getLineCount();
		int height = nlc * getRowHeight();
		if (height >= 300) {
			return(new Dimension(500,300));
		} else {
			return(new Dimension(500,height));
		}
	}

	public int getCaretLineShiftOffset() {
		try {
			int cp = getCaretPosition();
			int ln = getLineOfOffset(cp);
			return((ln+1) * getRowHeight());
		} catch (Exception e) {
			return(0);
		}
	}

	XrefSelectableLinesTextPanel(String list, DispatchData data, String inspectOption, int lineOffset, int baseLine) {
		super(list);
		this.data = data;
		this.lineOffset = lineOffset;
		this.inspectOption = inspectOption;
		setEditable(false);
		setBackground(s.light_gray);
		XrefClassTreeMouseAdapter ma = new XrefClassTreeMouseAdapter();
		addMouseListener(ma);
		// do not make selection, so one can copy paste from results
		//& addMouseMotionListener(ma);
		addKeyListener(ma);
		try {
			setCaretPosition(getLineStartOffset(baseLine));
		} catch (Exception ee) {}
	}

	XrefSelectableLinesTextPanel(String inspectOption) {
		this("", null, inspectOption, 1, 0);
	}

}


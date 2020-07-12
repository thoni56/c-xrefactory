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
import javax.swing.table.*;


public class CompletionDialog3 extends JDialog {

	// determine total dialog frame trims width and height
	static final int trimWidth;
	static final int trimHeight;
	static {
		JDialog dlg = new JDialog();
		dlg.pack();
		Dimension fdim = dlg.getSize();
		Dimension cdim = dlg.getContentPane().getSize();
		dlg.dispose();
		trimWidth = fdim.width - cdim.width;
		trimHeight = fdim.height - cdim.height;
	}

	DispatchData	data;
	XrefCharBuffer	completions = new XrefCharBuffer();
	LineData[]		lns;
	JPanel			table;
	JScrollPane		scrollPane;
	int 			selectedLine = 0;

	int				insertOffset;
	Buffer			insertBuffer;
	String			insertFile;
	String			typedId;

	HistoryItem[]	history = new HistoryItem[s.MAX_COMPLETION_HISTORY];
	int				historyIndex = 0;

	CompletionKeyAdapter keyAdapter = new CompletionKeyAdapter();

	public static CompletionDialog3 lastcDialog = null;

	int							x,y;	// window coordinates
	int							mx, my;	// mouse coordinates


	public class CompletionMouseAdapter extends MouseAdapter {
		public void mousePressed(MouseEvent e) {
			mx = e.getX();
			my = e.getY();
		}
		public void mouseClicked(MouseEvent e) {
			if ((e.getModifiers() & InputEvent.BUTTON1_MASK) != 0) {
				int y = e.getY() /*& + scrollPane.getVerticalScrollBar().getValue() &*/;
				int lineheight = tableLineHeight();
				int line = y / lineheight;
				if (line < 0) line = 0;
				if (line >= lns.length-1) line=lns.length-2;
				moveSelection(line);
				callCompletionGoto();
			}
		}
	}
	public class CompletionMouseMotionAdapter extends MouseMotionAdapter {
		public void mouseDragged(MouseEvent e) {
			if ((e.getModifiers() & InputEvent.BUTTON1_MASK) != 0) {
				int cx = e.getX();
				int cy = e.getY();
				int dx = cx - mx;
				int dy = cy - my;
				x = x+dx;
				y = y+dy;
				CompletionDialog3.this.setLocation(x, y);
			}
		}
	}

	public static class LineData {
		int			lines;
		int			cidents;
		String		vclasses;
		Color		bgColor;
		JComponent	p1;
		JComponent	p2;
		JComponent	p3;

		LineData(int lines, int cidents, String vcls) {
			this.lines = lines;
			this.cidents = cidents;
			this.vclasses = vcls;
			this.bgColor = null;

			p1 = p2 = p3 = null;
		}
	}

	public class HistoryItem {
		String		typedId;
		int			selection;
	}

	public class CompletionKeyAdapter extends KeyAdapter {
		public void keyPressed(KeyEvent e) {
			keyPressedAction(e);
		}
		public void keyReleased(KeyEvent e) {
			e.consume();
		}
		public void keyTyped(KeyEvent e) {
			e.consume();
		}
	}

	void callCompletionGoto() {
		int line = selectedLine+1;
		DispatchData ndata = new DispatchData(data, this);
		XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxcgoto" + line, ndata);
		Dispatch.dispatch(receipt, ndata);
	}

	void moveSelection(int nsel) {
		// do nothing for one line selection
		if (lns.length-1 == 1) return;
		if (nsel<0) nsel = 0;
		if (nsel>lns.length-2) nsel = lns.length-2;
		lns[selectedLine].p1.setBackground(lns[selectedLine].bgColor);
		lns[selectedLine].p2.setBackground(lns[selectedLine].bgColor);
		lns[selectedLine].p3.setBackground(lns[selectedLine].bgColor);
		selectedLine = nsel;
		lns[selectedLine].p1.setBackground(s.completionSelectionColor);
		lns[selectedLine].p2.setBackground(s.completionSelectionColor);
		lns[selectedLine].p3.setBackground(s.completionSelectionColor);
		// move selection on scroll panel
		JScrollBar jScrollBar = scrollPane.getVerticalScrollBar();
		int scrollmin = jScrollBar.getValue();
		int scrollsize = (int) scrollPane.getViewportBorderBounds().getHeight();
		int scrollmax = scrollmin + scrollsize;
		Point ll = new Point();
		ll = lns[selectedLine].p1.getLocation(ll);
		if (ll.y < scrollmin) {
			jScrollBar.setValue(ll.y);
		} else if (ll.y + lns[selectedLine].p1.getHeight() > scrollmax) {
			jScrollBar.setValue(ll.y + lns[selectedLine].p1.getHeight() - scrollsize);
		}
		//&CompletionDialog3.this.repaint();
	}

	int tableLineHeight() {
		int lineheight = (table.getHeight()/(lns.length-1));
		return(lineheight);
	}

	int scrollPaneDisplyedLines() {
		return((int)(scrollPane.getViewportBorderBounds().getHeight() / tableLineHeight()));
	}

	int searchForwardForSymbol(String sym) {
		int i;
		String s = sym.toLowerCase();
		int slen = s.length();
		for(i = selectedLine; i<lns.length-1; i++) {
			if (completions.substring(lns[i].cidents, lns[i].cidents+slen).toLowerCase().equals(s)) {
				return(i);
			}
		}
		return(i);
	}

	void recordHistory() {
		if (historyIndex >= s.MAX_COMPLETION_HISTORY) {
			System.arraycopy(history, 1, history, 0, s.MAX_COMPLETION_HISTORY-1);
			historyIndex --;
		}
		if (history[historyIndex]==null) history[historyIndex] = new HistoryItem();
		history[historyIndex].typedId = typedId;
		history[historyIndex].selection = selectedLine;
		historyIndex ++;
	}

	void escapeFromCompletionDialog() {
		String cc;
		if (jEdit.getBooleanProperty(s.optCompletionDelPendingId)) {
			cc = s.completionIdBeforeCaret+s.completionIdAfterCaret;
		} else {
			cc = s.completionIdBeforeCaret;
		}
		s.insertCompletionDoNotMoveCaret(insertBuffer, insertOffset, cc);
		CompletionDialog3.this.closeDialog();
	}

	public void keyPressedAction(KeyEvent e) {
		char key = e.getKeyChar();
		int code = e.getKeyCode();
		int mod = e.getModifiers();
		//&boolean searchFlag = (code==KeyEvent.VK_S && mod==InputEvent.CTRL_MASK);
		if (code != KeyEvent.VK_UP && code != KeyEvent.VK_DOWN
			&& code != KeyEvent.VK_KP_UP && code != KeyEvent.VK_KP_DOWN
			&& code != KeyEvent.VK_PAGE_UP && code != KeyEvent.VK_PAGE_DOWN
			&& code != KeyEvent.VK_LEFT && code != KeyEvent.VK_RIGHT			
			&& code != KeyEvent.VK_HOME && code != KeyEvent.VK_END			
			) {
			e.consume();
		}
		if (code == KeyEvent.VK_ENTER) {
			int line = selectedLine + 1;
			CompletionDialog3.this.setVisible(false);
			DispatchData ndata = new DispatchData(data, CompletionDialog3.this);
			XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcomplselect" + line, ndata);
			Dispatch.dispatch(receipt,ndata);
		} else if (code == KeyEvent.VK_SPACE 
				   && (e.getModifiers() & InputEvent.CTRL_MASK)!=0) {
			CompletionDialog3.this.closeDialog();
			//&s.moveToPosition(s.getParentView(CompletionDialog3.this),insertFile, insertOffset);
			CompletionDialog.completion(s.view);
		} else if (code == KeyEvent.VK_SPACE) {
			callCompletionGoto();
			SwingUtilities.invokeLater(new s.FocusRequester(table));
		} else if (code == KeyEvent.VK_BACK_SPACE) {
			if (historyIndex>0) {
				historyIndex --;
				typedId = history[historyIndex].typedId;
				moveSelection(history[historyIndex].selection);
				s.insertCompletion(insertBuffer, insertOffset, typedId);
			}
		} else if (code == KeyEvent.VK_ESCAPE) {
			escapeFromCompletionDialog();
			if ((e.getModifiers() & InputEvent.ALT_MASK)==0
				&& (e.getModifiers() & InputEvent.CTRL_MASK)==0) {
				s.moveToPosition(s.getParentView(CompletionDialog3.this), insertFile, 
								 insertOffset+s.completionIdBeforeCaret.length());
			}
		} else if (code == KeyEvent.VK_DOWN) {
			moveSelection(selectedLine+1);
			e.consume();
		} else if (code == KeyEvent.VK_UP) {
			moveSelection(selectedLine-1);
			e.consume();
		} else if (code == KeyEvent.VK_PAGE_DOWN) {
			moveSelection(selectedLine + scrollPaneDisplyedLines());
			e.consume();
		} else if (code == KeyEvent.VK_PAGE_UP) {
			moveSelection(selectedLine - scrollPaneDisplyedLines());
			e.consume();
		} else if (Character.isLetterOrDigit(key) || key=='_' || key=='$') {
			recordHistory();
			typedId = typedId + key;
			int nsel = searchForwardForSymbol(typedId);
			String nid;
			if (nsel < lns.length-1) {
				int symIndex = lns[nsel].cidents;
				typedId = completions.substring(symIndex, symIndex + typedId.length());
			}
			moveSelection(nsel);
			s.insertCompletion(insertBuffer, insertOffset, typedId);
		} else if ("`~!@#$%^&*()_+|-=\\{}[]:\";'<>?,./".indexOf(key) != -1) {
			CompletionDialog3.this.closeDialog();
			s.getBuffer().insert(s.getCaretPosition(), ""+key);
		}
	}

	void closeDialog() {
		setVisible(false);
	}	

	public static Color colorShift(Color c, int cs) {
		int r,g,b;
		r = c.getRed() + cs;
		g = c.getGreen() + cs;
		b = c.getBlue() + cs;
		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;
		return(new Color(r,g,b));
	}

	public JPanel crCompletionStringLine(int from, int to, Font f1, Font f2, Color c1, Color c2) {
		int i;
		for(i=from; i<to; i++) {
			if ((! Character.isJavaIdentifierPart(completions.buf[i]))
				&& completions.buf[i] != '.') break;
		}
		JPanel p = new JPanel();
		p.setLayout(new BoxLayout(p, BoxLayout.X_AXIS));
		JLabel l1 = new JLabel(completions.substring(from, i));
		l1.setForeground(c1);
		l1.setFont(f1);
		p.add(l1);
		JLabel l2 = new JLabel(completions.substring(i, to));
		l2.setForeground(c2);
		l2.setFont(f2);
		p.add(l2);
		return(p);
	}

	void setBackgroundColors() {
		Color bgc[] = new Color[2];
		bgc[0] = jEdit.getColorProperty(s.optCompletionBgColor, s.completionBgDefaultColor);
		bgc[1] = jEdit.getColorProperty(s.optCompletionBgColor2, s.completionBgDefaultColor2);
		
		int ci = 0;
		for(int i=0; i<lns.length-1; i++) {
			if (i>0 && ! lns[i].vclasses.equals(lns[i-1].vclasses)) ci = (ci+1) % 2;
			Color cc = bgc[ci];
			lns[i].bgColor = cc;
			lns[i].p1.setBackground(cc);
			lns[i].p2.setBackground(cc);
			lns[i].p3.setBackground(cc);
		}
	}

	public void refreshTable() {
		int n = lns.length-1;
		JLabel l1,l2;
		table = new JPanel();
		//&table.setDoubleBuffered(true);
		table.setLayout(null);
		Font f1 = jEdit.getFontProperty(s.optCompletionSymbolFont, s.defaultComplSymFont);
		Font f2 = jEdit.getFontProperty(s.optCompletionFont, s.defaultFont);
		Color c1 = jEdit.getColorProperty(s.optCompletionSymbolFgColor, Color.black);
		Color c2 = jEdit.getColorProperty(s.optCompletionFgColor, Color.black);

		int maxwidth1 = 0;
		int maxwidth2 = 0;
		int maxwidth3 = 0;
		int maxheight = 0;
		for(int i=0; i<lns.length-1; i++) {
			// left prefix
			if (lns[i].lines == lns[i].cidents) {
				l1 = new JLabel(" ");
			} else {
				l1 = new JLabel(completions.substring(lns[i].lines, lns[i].cidents),
								SwingConstants.RIGHT);
			}
			l1.setForeground(c2);
			l1.setFont(f2);
			l1.setOpaque(true);
			table.add(l1);
			lns[i].p1= l1;
			Dimension dim = l1.getMinimumSize();
			int width = (int) dim.getWidth();
			if (width > maxwidth1) maxwidth1 = width;
			int heigh = (int) dim.getHeight();
			if (heigh > maxheight) maxheight = heigh;

			// symbol and profile
			JPanel p = crCompletionStringLine(lns[i].cidents, lns[i+1].lines, f1, f2, c1, c2);
			if (s.javaVersion.compareTo("1.4.0") >= 0) p.setFocusable(true);
			p.setBorder(null);
			table.add(p);
			lns[i].p2 = p;
			Dimension dim2 = p.getMinimumSize();
			int width2 = (int) dim2.getWidth();
			if (width2 > maxwidth2) maxwidth2 = width2;
			heigh = (int) dim2.getHeight();
			if (heigh > maxheight) maxheight = heigh;

			// original class
			if (lns[i].vclasses == null) {
				l2 = new JLabel(" ");
			} else {
				l2 = new JLabel(lns[i].vclasses, SwingConstants.LEFT);
			}
			l2.setForeground(c2);
			l2.setFont(f2);
			l2.setOpaque(true);
			table.add(l2);
			lns[i].p3 = l2;
			dim = l2.getMinimumSize();
			width = (int) dim.getWidth();
			if (width > maxwidth3) maxwidth3 = width;
		}
		//&System.err.println("maxwidth1 == " +maxwidth1+"; maxheight =="+maxheight);
		for(int i=0; i<lns.length-1; i++) {
			lns[i].p1.setLocation(0, i*maxheight);
			lns[i].p1.setSize(maxwidth1, maxheight);
			lns[i].p2.setLocation(maxwidth1, i*maxheight);
			lns[i].p2.setSize(maxwidth2, maxheight);
			lns[i].p3.setLocation(maxwidth1+maxwidth2, i*maxheight);
			lns[i].p3.setSize(maxwidth3, maxheight);
		}
		//table.setBackground(jEdit.getColorProperty(s.optCompletionBgColor, s.completionBgDefaultColor));
		//table.setForeground(jEdit.getColorProperty(s.optCompletionFgColor, Color.BLACK));
		int totalWidth = maxwidth1+maxwidth2+maxwidth3;
		Dimension totalDim = new Dimension(totalWidth, maxheight*(lns.length-1));
		table.setSize(totalWidth, maxheight*(lns.length-1));
		table.setMinimumSize(totalDim);
		table.setPreferredSize(totalDim);
		if (s.javaVersion.compareTo("1.4.0") >= 0) table.setFocusable(true);
		table.addKeyListener(keyAdapter);
		table.addMouseListener(new CompletionMouseAdapter());
		table.addMouseMotionListener(new CompletionMouseMotionAdapter());
		setBackgroundColors();
		moveSelection(0);
	}

	public void showDialog() {
		refreshTable();
		Dimension dim = new Dimension(table.getPreferredSize());
		scrollPane.setViewportView(table);
		Insets insets = scrollPane.getInsets();
		dim.height += insets.top + insets.bottom;
		dim.width += insets.left + insets.right;
		if (s.javaVersion.compareTo("1.4.0") < 0) {
			// decorated
			dim.height += trimHeight; dim.width += trimWidth;
		}
		int maxWidth = Opt.completionDialogMaxWidth();
		int maxHeight = Opt.completionDialogMaxHeight();
		if (dim.height > maxHeight) {
			dim.width += scrollPane.getVerticalScrollBar().getPreferredSize().getWidth();
			dim.height = maxHeight;
		}
		if (dim.width > maxWidth) {
			dim.width = maxWidth;
			dim.height += scrollPane.getHorizontalScrollBar().getPreferredSize().getHeight();
			if (dim.height > maxHeight) {
				dim.height = maxHeight;
			}
		}
		setSize(dim); 
		moveSelection(0);
		Point recLocation = s.recommendedLocation(s.getTextArea());
		recLocation.x -= lns[0].p1.getWidth();
		setLocation(recLocation);
		s.moveOnScreen(this);
		
		setVisible(true);
		x = getX();
		y = getY();
		SwingUtilities.invokeLater(new s.FocusRequester(table));
	}

	void initCompletionCoordinates() {
		typedId = s.completionIdBeforeCaret;
		//&System.err.println("before=='"+s.completionIdBeforeCaret+"' after== '"+s.completionIdAfterCaret+"'");
		insertOffset = s.getCaretPosition() - s.completionIdBeforeCaret.length();
		insertBuffer = s.getBuffer();
		insertFile = s.getFileName();
		//&System.err.println("offset=='"+insertOffset+"' file== '"+insertFile+"'");
		historyIndex = 0;
	}

	static CompletionDialog3 initCompletionDialog(DispatchData data, int number, int noFocus) {
		// DO always new dialog, because in multiple views, the dialog
		// memorizes its parent
		if (lastcDialog==null) {
			lastcDialog = new CompletionDialog3();
		} else if (lastcDialog.isVisible()) {
			lastcDialog.setVisible(false);
			lastcDialog = new CompletionDialog3();
		} else {
			lastcDialog = new CompletionDialog3();
		}
		lastcDialog.data = data;
		lastcDialog.completions.clear();
		lastcDialog.lns = new LineData[number+1];
		lastcDialog.initCompletionCoordinates();
		return(lastcDialog);
	}

	static void showCompletionDialog() {
		lastcDialog.showDialog();
	}

	CompletionDialog3() {
		super(s.view, "");
		if (s.javaVersion.compareTo("1.4.0") >= 0) setUndecorated(true);
		else setResizable(false);
		table = new JPanel();
		scrollPane = new JScrollPane(table);
		scrollPane.getVerticalScrollBar().setUnitIncrement(5);
		setContentPane(scrollPane);
		//setModal(true);
		enableEvents(AWTEvent.KEY_EVENT_MASK);
	}

}


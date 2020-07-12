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


public class CompletionDialog2 extends JDialog {

	DispatchData	data;
	XrefCharBuffer	completions = new XrefCharBuffer();
	int[]			lines;
	int[]			cidents;
	int[]			vlevs;
	JTable			table;
	JScrollPane		scrollPane;

	public static CompletionDialog2 lastcDialog = null;

	public class CompletionTableModel extends AbstractTableModel {
		public int getColumnCount() { 
			return 2; 
		}
		public int getRowCount() { 
			return lines.length - 1;
		}
		public Object getValueAt(int row, int col) {
			if (col==0) {
				return completions.substring(lines[row], cidents[row]);
			} else {
				return completions.substring(cidents[row], lines[row+1]);
			}
		}
	}

	public class CompletionTableCellRenderer implements TableCellRenderer {
		public Component getTableCellRendererComponent(JTable t, Object val, boolean isSelected, 
													   boolean hasFocus, int row, int column) {
			if (column==0) {
				return new JLabel(completions.substring(lines[row], cidents[row]),
								  SwingConstants.RIGHT);
			} else if (column==1) {
				return new JLabel(completions.substring(cidents[row], lines[row+1]),
								  SwingConstants.LEFT);
			} else {
				return new JLabel("");
			}
		}
	}

	public class CompletionKeyAdapter extends KeyAdapter {
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
				int line = table.getSelectedRow()+1;
				CompletionDialog2.this.setVisible(false);
				DispatchData ndata = new DispatchData(data, CompletionDialog2.this);
				XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcomplselect" + line, ndata);
				Dispatch.dispatch(receipt,ndata);
			} else if (code == KeyEvent.VK_SPACE 
					   && (e.getModifiers() & InputEvent.CTRL_MASK)!=0) {
				/*&
				CompletionDialog2.this.closeDialog();
				s.moveToPosition(s.getParentView(CompletionDialog2.this),insertFile, insertOffset);
				completion(s.view);
				&*/
			} else if (code == KeyEvent.VK_SPACE) {
				//&callCompletionGoto();
			} else if (code == KeyEvent.VK_BACK_SPACE) {
				/*&
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
				&*/
			} else if (Character.isLetterOrDigit(key) || key=='_' || key=='$' || searchFlag) {
				/*&
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
						insertCompletion(s.getBuffer(), insertOffset-sel.length(), nnsel);
						insertOffset++;
					}
				}
				&*/
			} else if ("`~!@#$%^&*()_+|-=\\{}[]:\";'<>?,./".indexOf(key) != -1) {
				/*&
				CompletionDialog2.this.closeDialog();
				s.moveToPosition(s.getParentView(CompletionDialog2.this),insertFile, insertOffset);
				s.getBuffer().insert(insertOffset, ""+key);
				&*/
			} else if (code == KeyEvent.VK_ESCAPE) {
				CompletionDialog2.this.closeDialog();
				/*&
				if (mod == InputEvent.SHIFT_MASK || mod == InputEvent.ALT_MASK) {
					s.moveToPosition(s.getParentView(CompletionDialog2.this), insertFile, insertOffset);								
				}
				&*/
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
	}

	void closeDialog() {
		setVisible(false);
	}	

	void showDialog() {
		table.setModel(new CompletionTableModel());
		table.setShowGrid(false);
		//&table.setTableHeader(null);
		table.setDefaultRenderer(table.getColumnClass(0), new CompletionTableCellRenderer());
		pack();
		Dimension dim = table.getSize();
		dim.height += 3; dim.width += 3;
		// 20 is for scroll bars
		if (dim.getHeight() > Opt.completionDialogMaxHeight()) {
			dim.setSize(dim.getWidth()+20, Opt.completionDialogMaxHeight());
		}
		if (dim.getWidth() > Opt.completionDialogMaxWidth()) {
			dim.setSize(Opt.completionDialogMaxWidth(), dim.getHeight()+20);
		}
		setSize(dim);
		table.setAutoResizeMode(JTable.AUTO_RESIZE_ALL_COLUMNS);
		table.doLayout();
		setLocation(s.recommendedLocation(s.getTextArea()));
		s.moveOnScreen(this);
		/*&
		x = getX();
		y = getY();
		history = new LinkedList();
		history.addFirst(new HistoryElem(0, prefix));
		showSelection();
		&*/
		setVisible(true);
	}

	static CompletionDialog2 initCompletionDialog(DispatchData data, int number, int noFocus) {
		// DO always new dialog, because in multiple views, the dialog
		// memorizes its parent
		if (lastcDialog==null) {
			lastcDialog = new CompletionDialog2();
		} else if (lastcDialog.isVisible()) {
			lastcDialog.setVisible(false);
			lastcDialog = new CompletionDialog2();
		} else {
			lastcDialog = new CompletionDialog2();
		}
		lastcDialog.data = data;
		lastcDialog.completions.clear();
		lastcDialog.lines = new int[number+1];
		lastcDialog.cidents = new int[number];
		lastcDialog.vlevs = new int[number];
		return(lastcDialog);
	}

	static void showCompletionDialog() {
		lastcDialog.showDialog();
	}

	CompletionDialog2() {
		super(s.view, "");
		if (s.javaVersion.compareTo("1.4.0") >= 0) setUndecorated(true);
		table = new JTable();
		scrollPane = new JScrollPane(table);
		setContentPane(scrollPane);
		setModal(true);
		table.addKeyListener(new CompletionKeyAdapter());
		enableEvents(AWTEvent.KEY_EVENT_MASK);
	}

}


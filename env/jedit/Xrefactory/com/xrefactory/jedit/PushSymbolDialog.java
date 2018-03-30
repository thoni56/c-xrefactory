package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;
import org.gjt.sp.jedit.*;

public class PushSymbolDialog extends JDialog {

	public static class PushSymbolPanel extends JPanel {

		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				s.getParentDialog(this).setVisible(false);
			}
		}
		class ButtonPush extends JButton implements ActionListener {
			ButtonPush() {super("Push"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				s.getParentDialog(this).setVisible(false);
				String name = (String)symbol.getSelectedItem();
				if (!history.getLast().equals(name)) history.add(name);
				Buffer buffer = s.getBuffer();
				int caret = s.getCaretPosition();
				int line = buffer.getLineOfOffset(caret);
				int col = caret - buffer.getLineStartOffset(line);
				DispatchData ndata = new DispatchData(data, this);
				new Push(new String[] {"-olcxpushname="+name, "-olcxlccursor="+(line+1)+":"+col, "-olnodialog"}, ndata);
			}
		}
		class ButtonBrowse extends JButton implements ActionListener {
			ButtonBrowse() {super("Retrieve"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				// clear retriever, because of interference!
				DockableSymbolRetriever sr = (DockableSymbolRetriever)s.getParentView(data.callerComponent).getDockableWindowManager().getDockable(s.dockableRetrieverWindowName);
				if (sr!=null) sr.setResult(new XrefCharBuffer(), data);
				// O.K. now continue
				DispatchData ndata = new DispatchData(data, this);
				XrefCharBuffer receipt = ndata.xTask.callProcessOnFile(new String[] {"-searchdefshortlist", "-olcxtagsearch="}, ndata);
				Dispatch.dispatch(receipt, ndata);
				if (ndata.symbolList!=null && ! ndata.panic) {
					XrefSelectableLinesTextPanel	sp;
					JDialog dd = new JDialog(s.getParentDialog(this), true);
					if (s.javaVersion.compareTo("1.4.0") >= 0) dd.setUndecorated(true);
					sp = new XrefSelectableLinesTextPanel(ndata.symbolList.toString(), ndata, "", 1, 0);
					dd.setContentPane(new JScrollPane(sp));
					Rectangle mb = PushSymbolPanel.this.getBounds(null);
					dd.setSize((int)mb.getWidth(), 200);
					dd.setLocationRelativeTo(s.getParentDialog(this));
					dd.setLocation(dd.getX()+5, this.getY()+s.getParentDialog(this).getY()+50);
					dd.setVisible(true);
					symbol.setSelectedItem(sp.result);
				}
			}
		}
			
		JComboBox					symbol;
		DispatchData				data;
		static XrefStringArray		history = new XrefStringArray();
		
		PushSymbolPanel(DispatchData data, String defaultsym) {
			super();
			int i;
			int y= -1;

			this.data = data;

			setLayout(new GridBagLayout());

			JButton buttons[] = {
				new ButtonCancel(),
				new ButtonPush(),
			};

			if (!history.getLast().equals(defaultsym)) history.add(defaultsym);
			symbol = new JComboBox(history.toStringArray(false));
			symbol.setEditable(true);
			symbol.setSelectedItem(defaultsym);

			y++;
			s.addGbcComponent(this, 1,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel("Symbol to push on reference stack", SwingConstants.CENTER));

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			s.addGbcComponent(this, 1,y, 1,1, 1000,1, 
							  GridBagConstraints.HORIZONTAL,
							  symbol);
			s.addGbcComponent(this, 2,y, 1,1, 1,1, 
							  GridBagConstraints.HORIZONTAL,
							  new ButtonBrowse());
			s.addGbcComponent(this, 3,y, 1,1, 1,1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addExtraButtonLine(this, 0,y, 4,1, 1,1, buttons,true);

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
		}

	}

	void init(Component parent, DispatchData data) {
		setTitle("Xrefactory");
		PushSymbolPanel ttt = new PushSymbolPanel(data, s.getIdentifierOnCaret());
		setContentPane(ttt);
		setSize(400,150);
		setLocationRelativeTo(parent);
		setModal(true);
		setVisible(true);
	}

	public PushSymbolDialog(JDialog parent, DispatchData data) {
		super(parent);
		init(parent, data);
	}

	public PushSymbolDialog(JFrame parent, DispatchData data) {
		super(parent);
		init(parent, data);
	}
}

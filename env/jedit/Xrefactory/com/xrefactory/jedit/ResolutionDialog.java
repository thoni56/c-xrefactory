package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;

class ResolutionPanel extends JPanel {

	DispatchData		data;
	BrowserTopPanel		browser;

	class ButtonClose extends JButton implements ActionListener {
		ButtonClose() {super("Close"); addActionListener(this);}
		public void actionPerformed( ActionEvent e) {
			JDialog dd =  s.getParentDialog(this);
			if (dd!=null) {
				dd.setVisible(false);
			} else {
				JFrame ff =  s.getParentFrame(this);
				if (ff!=null) ff.setVisible(false);
			}
		}
	}
	class ButtonContinue extends JButton implements ActionListener {
		ButtonContinue() {super("Continue"); addActionListener(this);}
		public void actionPerformed( ActionEvent e) {
			s.getParentDialog(this).setVisible(false);
			XrefStringArray nooption = new XrefStringArray();
			XrefCharBuffer receipt = data.xTask.callProcess(nooption, data);
			Dispatch.dispatch(receipt, data);		
		}
	}

	ResolutionPanel(String message, int messageType, DispatchData data, boolean cont) throws Exception {
		super();
		int i,y,py;

		y = -1;

		this.data = data;

		JButton[] buttons;
		if (cont) {
			buttons = new JButton[] {new ButtonClose(), new ButtonContinue()};
		} else {
			buttons = new JButton[] {new ButtonClose()};
		}
		browser = new BrowserTopPanel(data, false, JSplitPane.HORIZONTAL_SPLIT);
		setLayout(new GridBagLayout());

		int itype;
		switch (messageType) {
		case Protocol.PPCV_BROWSER_TYPE_WARNING: 	itype = JOptionPane.WARNING_MESSAGE;
			break;
		case Protocol.PPCV_BROWSER_TYPE_INFO: 	itype = JOptionPane.INFORMATION_MESSAGE;
			break;
		default:
			throw new XrefException("Unknown PPCV_BROWSER_TYPE");
		}
				
		JOptionPane info = new JOptionPane(message, itype, JOptionPane.DEFAULT_OPTION, null, new JPanel[] {new JPanel()});

		JPanel infopanel = new JPanel();
		infopanel.setLayout(new GridBagLayout());

		y++;
		s.addGbcComponent(this, 0,y, 1,1, 1,1, 
						  GridBagConstraints.BOTH,
						  new JPanel());
		s.addGbcComponent(this, 1,y, 1,1, 1,1, 
						  GridBagConstraints.BOTH,
						  info);
		s.addGbcComponent(this, 2,y, 1,1, 1,1, 
						  GridBagConstraints.BOTH,
						  new JPanel());

		y++;
		s.addGbcComponent(this, 0,y, 3,1, 1000,1000, 
						  GridBagConstraints.BOTH,
						  browser);

		y++;
		s.addGbcComponent(this, 0,y, 3,1, 1,1, 
						  GridBagConstraints.HORIZONTAL,
						  new JPanel());

		y++;
		s.addExtraButtonLine(this, 0,y, 3,1, 1,1, buttons,true);
	}

}


class ResolutionFrame extends JFrame {

	ResolutionFrame(String message, int messageType, DispatchData data, boolean cont) throws Exception {
		super("Symbol resolution");
		ResolutionPanel ttt = new ResolutionPanel(message, messageType, data, cont);
		setContentPane(ttt);
		setSize(600,400);
		ttt.browser.updateData();
		setLocationRelativeTo(s.view);
		setVisible(true);
	}

}

public class ResolutionDialog extends JDialog {

	ResolutionDialog(String message, int messageType, DispatchData data, boolean cont) throws Exception {
		super(s.view, "Symbol resolution");
		ResolutionPanel ttt = new ResolutionPanel(message, messageType, data, cont);
		setContentPane(ttt);
		setSize(600,400);
		ttt.browser.updateData();
		setLocationRelativeTo(s.view);
		if (cont) setModal(true);
		setVisible(true);
	}

}




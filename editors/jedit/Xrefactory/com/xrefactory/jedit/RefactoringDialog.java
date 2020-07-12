package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.text.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.textarea.*;


public class RefactoringDialog extends JDialog {

	class RefactoringPanel extends JPanel implements KeyListener {
		JList 	list;

		public void keyPressed(KeyEvent e) {
			if (e.getKeyCode() == KeyEvent.VK_ESCAPE) {
				s.getParentDialog(this).setVisible(false);
			} else if (e.getKeyCode() == KeyEvent.VK_ENTER) {
				s.getParentDialog(this).setVisible(false);
				Refactorings rr = (Refactorings)list.getSelectedValue();
				rr.perform();
			}
		}
		public void keyReleased(KeyEvent e) {
			e.consume();
		}
		public void keyTyped(KeyEvent e) {
			e.consume();
		}

		RefactoringPanel(Refactorings[] rr) {
			super();
			int y;
			setLayout(new GridBagLayout());
			list = new JList(rr);
			list.addKeyListener(this);
			list.setBackground(s.light_gray);
			list.setSelectedIndex(0);
			SwingUtilities.invokeLater(new s.FocusRequester(list));

			JTextArea label;
			if (rr.length == 0) {
				label = new JTextArea(
" No refactoring for this place. Usually you need \n to move caret at the  definition  of  a  symbol \n before invoking refactorings.");
				/*&
				  } else if (rr.length == 1) {
				  label = new JLabel("Hit <enter> to ");
				  &*/
			} else {
				label = new JTextArea("Select refactoring from:");
			}
			label.setEditable(false);
			label.setBackground(s.light_gray);
			if (s.javaVersion.compareTo("1.4.0") >= 0) label.setFocusable(false);

			y=0;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  label);
			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  list);
		}
	}

	RefactoringDialog(Refactorings[] rr) {
		super(s.view,"",true);
		Component 	cc;
		enableEvents(AWTEvent.KEY_EVENT_MASK);
		setContentPane(new JScrollPane(new RefactoringPanel(rr)));
		if (s.javaVersion.compareTo("1.4.0") >= 0) setUndecorated(true);
		pack();
		setLocation(s.recommendedLocation(s.getTextArea()));
		s.moveOnScreen(this);
		//&addKeyListener(this);
		//&setSize(500,120);
		setVisible(true);
	}


}

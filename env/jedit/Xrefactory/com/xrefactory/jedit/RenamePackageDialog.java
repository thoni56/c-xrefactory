package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.event.*;
import java.awt.*;


public class RenamePackageDialog extends JDialog {

	public static class RenamePackagePanel extends JPanel implements KeyListener {
		JTextField newname;

		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionCancel();
			}
		}
		class ButtonContinue extends JButton implements ActionListener {
			ButtonContinue() {super("Continue"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionContinue();
			}
		}

		void actionContinue() {
			s.getParentDialog(this).setVisible(false);
		}

		void actionCancel() {
			s.getParentDialog(this).setVisible(false);
			newname.setText("");
		}

		public void keyTyped(KeyEvent e) {}
		public void keyReleased(KeyEvent e) {}
		public void keyPressed(KeyEvent e) {
			int code = e.getKeyCode();
			if (code == KeyEvent.VK_ESCAPE) {
				e.consume();
				actionCancel();
			} else if (code == KeyEvent.VK_ENTER) {
				e.consume();
				actionContinue();
			}
		}

		RenamePackagePanel(String packageName, String defaultName) {
			super();
			int y,py;
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue()};
			newname = new JTextField(defaultName);
			newname.addKeyListener(this);
			setLayout(new GridBagLayout());

			JPanel mainpanel = new JPanel();
			mainpanel.setLayout(new GridBagLayout());

			py = 0;
			s.addGbcComponent(mainpanel, 0,py, 2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("You are going to rename (move) package: "+packageName));
			py++;
			s.addGbcComponent(mainpanel, 0,py, 2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel(" Enter fully qualified name of the new package."));
			py++;
			s.addGbcComponent(mainpanel, 0,py, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			py++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("New package name:"));
			s.addGbcComponent(mainpanel, 1,py, 1,1, 100,1, 
							  GridBagConstraints.HORIZONTAL,
							  newname);

			y=0;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  mainpanel);
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addButtonLine(this, y, buttons,true);
		}
	}

	RenamePackageDialog(String packageName, String defaultName) {
		super(s.view);
		setContentPane(new RenamePackagePanel(packageName, defaultName));
		setModal(true);
		pack();
		setLocationRelativeTo(s.view);
		setVisible(true);
	}
}

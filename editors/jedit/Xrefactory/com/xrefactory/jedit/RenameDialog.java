package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.event.*;
import java.awt.*;


public class RenameDialog extends JDialog {

	public static class RenamePanel extends JPanel implements KeyListener {
		JTextField textField;

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
			textField.setText("");
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

		RenamePanel(String sort, String oldname) {
			super();
			int y= -1;
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue()};
			textField = new JTextField(oldname);
			textField.addKeyListener(this);
			setLayout(new GridBagLayout());

			y++;
			s.addGbcComponent(this, 0,y, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("  Rename "+sort+" "+oldname+" to  ", SwingConstants.CENTER));
			y++;
			s.addGbcComponent(this, 0,y, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("  new name:", SwingConstants.RIGHT));
			s.addGbcComponent(this, 1,y, 1,1, 100,1, 
							  GridBagConstraints.HORIZONTAL,
							  textField);
			y++;
			s.addGbcComponent(this, 0,y, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addExtraButtonLine(this, 0, y, 2,1, 1,1, buttons,true);

			y++;
			s.addGbcComponent(this, 0,y, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
		}
	}

	public String getNewName() {
		String newname = ((RenamePanel)getContentPane()).textField.getText();
		if (newname.equals("")) return(null);
		else return(newname);
	}

	public RenameDialog(String sort, String oldname) {
		super(s.view, "Renaming");
		setContentPane(new RenamePanel(sort, oldname));
		setModal(true);
		pack();
		setLocationRelativeTo(s.view);
		setVisible(true);
	}
}

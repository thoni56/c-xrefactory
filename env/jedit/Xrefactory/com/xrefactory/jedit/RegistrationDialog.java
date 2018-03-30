package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import com.xrefactory.jedit.Options.*;
import java.io.*;

public class RegistrationDialog  extends JDialog {

	public static class RegistrationPanel extends JPanel implements KeyListener {
		JTextField license;

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

		String setNewLicenseString(String lstr) throws Exception {
			String olds = null;
			OptionParser parser = new OptionParser(new File(s.configurationFile));
			Options[] projects = parser.parseFile();
			Option ls = projects[0].getContainedOption(Options.optLicense);
			if (ls!=null) {
				olds = ls.fulltext[1];
				projects[0].delete(ls);
			}
			projects[0].add(new Option(Options.optLicense, lstr));
			Options.saveOptions(projects, s.configurationFile);
			return(olds);
		}

		void actionContinue() {
			s.getParentDialog(this).setVisible(false);
			try {
				String olds = setNewLicenseString(license.getText());
				// o.k. this is just to het some action requiring license
				String nl=s.computeSomeInformationInXref(s.view, new String[]{"-get", "nl"});
				if (nl==null) {
					// the only possibility is that the string is wrong
					if (olds!=null) setNewLicenseString(olds);
				}
			} catch (Exception e) {	}
		}

		void actionCancel() {
			s.getParentDialog(this).setVisible(false);
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

		RegistrationPanel() {
			super();
			int y = -1;
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue()};
			license = new JTextField("");
			license.addKeyListener(this);
			setLayout(new GridBagLayout());

			JPanel mainpanel = new JPanel();
			mainpanel.setLayout(new GridBagLayout());

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel("Insert license string", SwingConstants.CENTER));

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  license);

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addExtraButtonLine(this, 0,y, 1,1, 1,1,
								 buttons, true);

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

		}
	}

	public RegistrationDialog() {
		super(s.view, "Register Your Copy");
		setContentPane(new RegistrationPanel());
		setModal(true);
		setSize(400, 150);
		setLocationRelativeTo(s.view);
		setVisible(true);
	}
}

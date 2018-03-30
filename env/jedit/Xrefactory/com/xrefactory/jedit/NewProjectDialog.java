package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;

public class NewProjectDialog extends JDialog {

	public static class NewProjectPanel extends JPanel {

		class ButtonBrowseSource extends JButton implements ActionListener {
			ButtonBrowseSource() {super("Browse"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				PathSelectionDialog np = new PathSelectionDialog(
					s.getParentDialog(this), sourceDirectory.getText());
				sourceDirectory.setText(np.resultString);
			}
		}
		class ButtonBrowseClassPath extends JButton implements ActionListener {
			ButtonBrowseClassPath() {super("Browse"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				PathSelectionDialog np = new PathSelectionDialog(
					s.getParentDialog(this), classPath.getText());
				classPath.setText(np.resultString);
			}
		}
		class ButtonBrowseSourcePath extends JButton implements ActionListener {
			ButtonBrowseSourcePath() {super("Browse"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				PathSelectionDialog np = new PathSelectionDialog(
					s.getParentDialog(this), sourcePath.getText());
				sourcePath.setText(np.resultString);
			}
		}
		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				projectName.setText("");
				s.getParentDialog(this).setVisible(false);
			}
		}
		class ButtonContinue extends JButton implements ActionListener {
			ButtonContinue() {super("Continue"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				if (s.projectExists(projectName.getText(), projects)) {
					JOptionPane.showMessageDialog(
						s.getParentDialog(this), 
						"Project " + projectName.getText() + " yet exists", 
						"Xrefactory Error", JOptionPane.ERROR_MESSAGE);
				} else if (! projectName.getText().equals("")) {
					s.getParentDialog(this).setVisible(false);
				}
			}
		}

		public JTextField projectName;
		public JTextField sourceDirectory;
		public JTextField classPath;
		public JTextField sourcePath;
		public JCheckBox makeActive;
		public Options[] projects;
		static int newcounter = 0;

		NewProjectPanel(Options[] projects) {
			int y = -1;
			setLayout(new GridBagLayout());

			this.projects = projects;
			do {
				projectName = new JTextField("test"+newcounter++);
			} while (s.projectExists(projectName.getText(), projects));
			String initdir;
			String initcp;
			if (s.view==null) {
				initdir = "";
				initcp = "";
			} else {
				initdir = (new File(s.view.getBuffer().getPath())).getParent();
				initcp = s.inferDefaultSourcePath();
			}
			sourceDirectory = new JTextField(initdir);
			classPath = new JTextField(initcp);
			sourcePath = new JTextField(initcp);
			makeActive = new JCheckBox("Become the active project");

			JButton[] buttons = {
				new ButtonCancel(),
				new ButtonContinue(),
			};

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0, y, 3, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Creating New Project!", SwingConstants.CENTER));
			y++;
			s.addGbcComponent(this, 0, y, 3, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("with", SwingConstants.CENTER));
			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Project Name: ", SwingConstants.RIGHT));
			s.addGbcComponent(this, 1, y, 2, 1, 1000, 1, 
							  GridBagConstraints.HORIZONTAL,
							  projectName);

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Source directories: ", SwingConstants.RIGHT));
			s.addGbcComponent(this, 1, y, 1, 1, 1000, 1, 
							  GridBagConstraints.HORIZONTAL,
							  sourceDirectory);
			s.addGbcComponent(this, 2, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new ButtonBrowseSource());

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Classpath: ", SwingConstants.RIGHT));
			s.addGbcComponent(this, 1, y, 1, 1, 1000, 1, 
							  GridBagConstraints.HORIZONTAL,
							  classPath);
			s.addGbcComponent(this, 2, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new ButtonBrowseClassPath());

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Sourcepath: ", SwingConstants.RIGHT));
			s.addGbcComponent(this, 1, y, 1, 1, 1000, 1, 
							  GridBagConstraints.HORIZONTAL,
							  sourcePath);
			s.addGbcComponent(this, 2, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new ButtonBrowseSourcePath());

			y++;
			s.addGbcComponent(this, 1, y, 2, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  makeActive);

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());

			y++;
			s.addExtraButtonLine(this,0, y,6,1,1,1, buttons,true);

		}

	}

	void init(Component parent, Options[] projects) {
		setContentPane(new NewProjectPanel(projects));
		setSize(600,250);
		setModal(true);
		setLocationRelativeTo(parent);
		setVisible(true);
	}


	NewProjectDialog(Dialog parent, Options[] projects) {
		super(parent, "Create New Project");
		init(parent, projects);
	}
	NewProjectDialog(Frame parent, Options[] projects) {
		super(parent, "Create New Project");
		init(parent, projects);
	}
}

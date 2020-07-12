package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import com.xrefactory.jedit.Options.*;
import org.gjt.sp.jedit.*;

public class OptionsForProjectsDialog extends JDialog {

	public static class OptionsForProjectsPanel extends JPanel {

		class OptionsForProjectsNoFilePanel extends JPanel {

			class GraphicOption extends  Container {

				class ButtonBrowse extends JButton implements ActionListener {
					String browseMode;
					ButtonBrowse() {
						super("Browse"); 
						setToolTipText("Browse");
						setFont(s.optionsBrowseButtonFont);
						addActionListener(this);
					}
					public void actionPerformed( ActionEvent e) {
						String mm = browseMode.toLowerCase();
						if (mm.equals("d")	|| mm.equals("f") || mm.equals("fd")) {
							// file / directory chooser
							int mode;
							JFileChooser fc = new JFileChooser();
							if (mm.equals("d")) mode = JFileChooser.DIRECTORIES_ONLY;
							else if (mm.equals("f")) mode = JFileChooser.FILES_ONLY;
							else mode = JFileChooser.FILES_AND_DIRECTORIES;
							fc.setFileSelectionMode(mode);
							fc.setApproveButtonText("Select");
							//OptionsPanel.this.add(fc);
							int i = fc.showOpenDialog(s.getParentDialog(this));
							tf.setText(fc.getSelectedFile().getPath());
						} else if (mm.equals("p")) {
							// path chooser
							PathSelectionDialog np = new PathSelectionDialog(
								s.getParentDialog(this), tf.getText());
							tf.setText(np.resultString);
						}
						updateAndSelectCheckBox();
					}
				}

				class checkBoxActionListener implements ActionListener {
					public void actionPerformed(ActionEvent e) {
						checkBoxChanged();
					}
				}
				class TextFieldKeyListener implements KeyListener {
					public void keyPressed(KeyEvent e) {
						if (e.getKeyCode() == KeyEvent.VK_ENTER) {
							updateAndSelectCheckBox();
							e.consume();
						}
					}
					public void keyReleased(KeyEvent e) {}
					public void keyTyped(KeyEvent e) {}
				}
				class TextFieldFocusListener implements FocusListener {
					public void focusGained(FocusEvent e) {
					}

					public void focusLost(FocusEvent e) {
						updateAndSelectCheckBox();
					}
				}


				// ------------------------ GraphicOption ------------------
				Option 			option;

				JCheckBox		jb;
				JTextField		tf;
				ButtonBrowse 	bb;

				void checkBoxChanged() {
					if (jb.isSelected()) {
						options.add(option);
					} else {
						options.delete(option);
					}
					updateStringOption();
				}

				void updateAndSelectCheckBox() {
					int index;
					String val = tf.getText();
					if (jb.isSelected()) {
						index = options.delete(option);
						option.fulltext[1] = tf.getText();
						if (val.equals("")) {
							jb.setSelected(false);
						} else {
							options.add(option, index);
						}
						updateStringOption();
					} else {
						option.fulltext[1] = val;
						if (! val.equals("")) {
							jb.setSelected(true);
							checkBoxChanged();
						}
					}

					update(val);
					if (! jb.isSelected() && !val.equals("")) {
						jb.setSelected(true);
						checkBoxChanged();
					}
				}

				public void update(String newText) {
					int index;
					if (jb.isSelected()) {
						index = options.delete(option);
						option.fulltext[1] = tf.getText();
						options.add(option, index);
						updateStringOption();
					} else {
						option.fulltext[1] = tf.getText();
					}
				}

				GraphicOption(OptionDescription opt, Option option) {
					if (option==null) {
						this.option = new Option(opt, "");
					} else {
						this.option = option;
					}
					setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
					//&jb = new JCheckBox(opt.name + "  ( " + opt.shortDescription + " )");

					if (opt.arity==1) {
						jb = new JCheckBox(opt.shortDescription);
						jb.addActionListener(new checkBoxActionListener());
						add(jb);
					} else {
						jb = new JCheckBox();
						add(new JLabel(opt.shortDescription));
					}
					if (option!=null) jb.setSelected(true);
					if (opt.arity>1) {
						// just for now it should be O.K.
						s.assertt(opt.arity == 2);
						String val;
						tf = new JTextField(this.option.fulltext[1]);
						tf.setPreferredSize(new Dimension(300,20));
						add(tf);
						tf.addFocusListener(new TextFieldFocusListener());
						tf.addKeyListener(new TextFieldKeyListener());
					}
					if (opt.fileSystemBrowseOptions!=null) {
						bb = new ButtonBrowse();
						bb.browseMode = opt.fileSystemBrowseOptions;
						add(bb);
					}
				}

			}

			OptionsForProjectsNoFilePanel(Options options, int kind) {
				int y = -1;

				setLayout(new GridBagLayout());

				y++;
				for(int i=0; i<Options.allOptions.length; i++) {
					OptionDescription od = Options.allOptions[i];
					if (od.interactive && (kind==od.kind || od.kind==Options.OPT_BOTH)) {
						Option oo = null;
						for(int j=0; j<options.optioni; j++) {
							Option o = options.option[j];
							if (o.option == od) {
								oo = o;
							}
						}
						GraphicOption go = new GraphicOption(od, oo);
						s.addGbcComponent(this, 0, y++, 1, 1, 10000, 10000, 
										  GridBagConstraints.HORIZONTAL,
										  go);
					}
				}

			}

		}

		class StringOptionFocusListener implements FocusListener {
			public void focusGained(FocusEvent e) {
			}
			public void focusLost(FocusEvent e) {
				updateOptionsFromOptionsString();
				updateStringOption();
			}
		}
		class StringOptionKeyListener implements KeyListener {
			public void keyPressed(KeyEvent e) {
				if (e.getKeyCode() == KeyEvent.VK_ENTER) {
					stringOption.insert("\n", stringOption.getCaretPosition());
					updateOptionsFromOptionsString();
					e.consume();
				}
			}
			public void keyReleased(KeyEvent e) {
				if (e.getKeyCode() == KeyEvent.VK_ENTER) {
					e.consume();
				}
			}
			public void keyTyped(KeyEvent e) {}
		}

		// ------------------------ OptionPanel ------------------

		boolean								displayString;
		Options								options;
		int									kind;
		
		JTextArea 							stringOption;
		OptionsForProjectsNoFilePanel		optionsPanel;

		public void addOptionsPanel() {
			s.addGbcComponent(this, 0, 0, 1, 1, 10000, 10000, 
							  GridBagConstraints.BOTH,
							  optionsPanel);
		}

		void updateStringOption() {
			int cp = stringOption.getCaretPosition();
			stringOption.setText(options.toString());
			stringOption.setCaretPosition(cp);
		}

		public void updateOptionsFromOptionsString() {
			try {
				Options.OptionParser pp = new Options.OptionParser(stringOption.getText());
				pp.reParseSingleProject(options);
				OptionsForProjectsPanel.this.remove(optionsPanel);
				optionsPanel = new OptionsForProjectsNoFilePanel(options, kind);
				addOptionsPanel();
				OptionsForProjectsPanel.this.revalidate();
			} catch (Exception ee) {
				JOptionPane.showMessageDialog(s.getParentDialog(stringOption), ee, "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
				if (s.debug) ee.printStackTrace(System.err);
			}
		}

		public OptionsForProjectsPanel(Options options, int kind, boolean displayString) {
			int y = -1;

			this.displayString = displayString;
			this.options = options;
			this.kind = kind;

			optionsPanel = new OptionsForProjectsNoFilePanel(options, kind);
			stringOption = new JTextArea(options.toString());
			stringOption.addFocusListener(new StringOptionFocusListener());
			stringOption.addKeyListener(new StringOptionKeyListener());

			setLayout(new GridBagLayout());

			addOptionsPanel();

			Component disp;
			if (displayString) {
				JScrollPane sp = new JScrollPane(stringOption);
				sp.setMinimumSize(new Dimension(100,80));
				sp.setPreferredSize(new Dimension(300,200));
				disp = sp;
			} else {
				disp = new JPanel();
			}
			s.addGbcComponent(this, 0, 1, 1, 1, 100, 5000, 
							  GridBagConstraints.BOTH,
							  disp);
		}
	}

	public static class ProjectOptionsPanel extends JPanel {

		void saveProjectSettings() {
			checkConfigurationFile();
			if (checkOptionsConsistency()) {
				Options.saveOptions(projects, s.configurationFile);
				checkProjectsModifications();
				originalProjects = crOptionsArrayCopy(projects);
				Opt.setActiveProject((String)activeProject.getSelectedItem());
			}
		}

		class ButtonNewProject extends JButton implements ActionListener {
			ButtonNewProject() {super("Add"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				addNewProject(s.getProbableParent(this));
			}
		}
		class ButtonDelProject extends JButton implements ActionListener {
			ButtonDelProject() {super("Del"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				int i = projectsPanel.getSelectedIndex();
				if (i==0) {
					JOptionPane.showMessageDialog(s.getProbableParent(this), "This project can not be deleted");
				} else {
					int confirm = JOptionPane.YES_OPTION;
					confirm = JOptionPane.showConfirmDialog(
						s.getProbableParent(this), 
						"Really delete project "+projectsPanel.getTitleAt(projectsPanel.getSelectedIndex()), "Confirmation", 
						JOptionPane.YES_NO_OPTION,
						JOptionPane.QUESTION_MESSAGE);
					if (confirm == JOptionPane.YES_OPTION) {
						if (i>0) {
							Options[] no = new Options[projects.length-1];
							s.arrayCopyDelElement(projects, no, i);
							projects = no;
							if (activeProject.getSelectedIndex()==i) {
								// removing active project
								activeProject.setSelectedIndex(0);
							}
							activeProject.removeItemAt(i);
							projectsPanel.removeTabAt(i);
							if (i<no.length) {
								projectsPanel.setSelectedIndex(i);
							} else {
								projectsPanel.setSelectedIndex(no.length-1);
							}
							//& remakeOptionsPanel();
						}
					}
				}
			}
		}

		// ---------------------  ProjectOptionsPanel ---------------------

		JTextField 					configurationFile;
		OptionsForProjectsPanel 	optionsPanel;
		JComboBox 					activeProject;
		JTabbedPane 				projectsPanel;
		Options[] 					projects;
		Options[] 					originalProjects;
		int							opy;

		public void checkConfigurationFile() {
			if (! s.configurationFile.equals(configurationFile.getText())) {
				jEdit.setProperty(s.configurationFileOption, configurationFile.getText());
				JOptionPane.showMessageDialog(s.view, 
											  "You have to restart jEdit in order to use new Xrefactory configuration file.", 
											  "Xrefactory Message", JOptionPane.INFORMATION_MESSAGE);
			}
		}

		public boolean addNewProject(Component parent) {
			NewProjectDialog nn;
			if (parent instanceof Dialog) nn = new NewProjectDialog((Dialog)parent, projects);
			else if (parent instanceof Frame) nn = new NewProjectDialog((Frame)parent, projects);
			else {nn=null; s.assertt(false);}
			NewProjectDialog.NewProjectPanel pp = (NewProjectDialog.NewProjectPanel)nn.getContentPane();
			if (! pp.projectName.getText().equals("")) {
				Options[] no = new Options[projects.length+1];
				System.arraycopy(projects,0,no,0,projects.length);
				no[projects.length] = new Options(pp.projectName.getText(), 
												  pp.sourceDirectory.getText()
					);
				no[projects.length].addNewProjectOptions(
					pp.projectName.getText(), pp.sourceDirectory.getText(), 
					pp.classPath.getText(), pp.sourcePath.getText()
					);
				projects = no;
				int i = projects.length-1;
				projectsPanel.add(projects[i].projectName, new OptionsForProjectsPanel(projects[i], Options.OPT_PROJECT, Opt.optionsEditingDisplaysFile));
				projectsPanel.setSelectedIndex(i);
				activeProject.addItem(projects[i].projectName);
				JOptionPane.showMessageDialog(parent, "New project was created. Please verify that remaining options are correctly set.\nIn particular that Jdk runtime library is the file storing your jdk classes (usualy rt.jar)");
				if (pp.makeActive.isSelected()) {
					Opt.setActiveProject(pp.projectName.getText());
				}
				return(true);
			} else {
				return(false);
			}
		}

		public boolean checkOptionsConsistency() {
			int i,j,rr;
			for(i=0; i<projects.length; i++) {
				for(j=i+1; j<projects.length; j++) {
					PathIterator ii = new PathIterator(
						projects[i].getProjectAutoDetectionOpt(),s.classPathSeparator);
					while(ii.hasNext()) {
						String cpi = ii.next();
						PathIterator ji = new PathIterator(
							projects[j].getProjectAutoDetectionOpt(),s.classPathSeparator);
						while(ji.hasNext()) {
							String cpj = ji.next();
							if (Options.projectMarkersOverlapps(cpi, cpj)) {
								rr = JOptionPane.showOptionDialog(s.getProbableParent(this), "Conflict between projects "+projects[i].projectName+" and "+projects[j].projectName+".\nProjects overlap in auto detection paths: "+cpi+" and "+cpj, "Configuration File Problem", JOptionPane.YES_NO_OPTION, JOptionPane.ERROR_MESSAGE, null, new String[] {"Quit without saving", "Ignore this problem and save", "Ignore all problems and save"}, null);
								if (rr==0) return(false);
								if (rr==2) return(true);
							}
						}
					}
				}
			}
			return(true);
		}

		public void recreateTagFile(Options o) {
			DispatchData ndata = new DispatchData(s.xbTask, this);
			s.activeProject = o.projectName; // this is a hack, new project is active for creation
			XrefTaskForTagFile.runXrefOnTagFile("-create", "Creating Tags.",true, ndata);
		}

		public void checkProjectsModifications() {
			int i,j,k;
			for(i=0; i<projects.length; i++) {
				for(j=0; j<originalProjects.length; j++) {
					if (projects[i].projectName.equals(originalProjects[j].projectName)) break;
				}
				if (j==originalProjects.length) {
					int confirm = JOptionPane.showConfirmDialog(
						s.getProbableParent(this), 
						"Project "+ projects[i].projectName + " added. Can I create Tags for it?", 
						"Xrefactory Confirmation", 
						JOptionPane.YES_NO_OPTION,
						JOptionPane.QUESTION_MESSAGE);
					if (confirm == JOptionPane.YES_OPTION) {
						recreateTagFile(projects[i]);
					}
				} else {
					// check for modification of important options
					Options newopt = projects[i];
					Options oldopt = originalProjects[j];
					for(k=0; k<newopt.optioni; k++) {
						if (newopt.option[k].option.recreateTagsIfModified) {
							if (! oldopt.contains(newopt.option[k])) break;
						}
					}
					if (k!=newopt.optioni) {
						int confirm = JOptionPane.showConfirmDialog(
							s.getProbableParent(this), 
							"Options in project "+newopt.projectName + " changed. Recreate Tags?", 
							"Xrefactory Confirmation", 
							JOptionPane.YES_NO_OPTION,
							JOptionPane.QUESTION_MESSAGE);
						if (confirm == JOptionPane.YES_OPTION) {
							recreateTagFile(newopt);
						}
					}
				}
			}
		}

		public static Options[] crOptionsArrayCopy(Options[] oo) {
			Options[] res;
			res = new Options[oo.length];
			for(int i=0; i<oo.length; i++) res[i] = new Options(oo[i]);
			return(res);
		}

		class ConfigurationFileListener implements FocusListener {
			public void focusGained(FocusEvent e) {}
			public void focusLost(FocusEvent e) {
				//& createActiveProjectList();
			}
		}

		void activeProjectListRefresh(ComboBoxModel model) {
			int n = model.getSize();
			String[] projects = new String[n];
			projects[0] = s.projectAutoDetectionName;
			for(int i=1; i<n; i++) projects[i] = (String)model.getElementAt(i);
			activeProject.setModel(new DefaultComboBoxModel(projects));
		}

		public ProjectOptionsPanel(Options[] projects, String defaultp) {
			int y = -1;

			this.projects = projects;
			this.originalProjects = crOptionsArrayCopy(projects);

			configurationFile = new JTextField(s.configurationFile);
			activeProject = new JComboBox(Options.getProjectNames(
				projects, s.projectAutoDetectionName));
			activeProject.setSelectedItem(Opt.activeProject());

			projectsPanel = new JTabbedPane();
			projectsPanel.add("Common options", new OptionsForProjectsPanel(projects[0], Options.OPT_COMMON, Opt.optionsEditingDisplaysFile));
							  
			for(int i=1; i<projects.length; i++) {
				projectsPanel.add(projects[i].projectName, new OptionsForProjectsPanel(projects[i], Options.OPT_PROJECT, Opt.optionsEditingDisplaysFile));
				if (projects[i].projectName.equals(defaultp)) {
					projectsPanel.setSelectedIndex(i);
				}
			}

			setLayout(new GridBagLayout());

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			s.addGbcComponent(this, 1, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Store options in the file: ",SwingConstants.RIGHT));
			s.addGbcComponent(this, 2, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  configurationFile);
			s.addGbcComponent(this, 2, y, 1, 1, 100, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			s.addGbcComponent(this, 4, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new ButtonNewProject());
			s.addGbcComponent(this, 5, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			s.addGbcComponent(this, 1, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JLabel("Active Project: ",SwingConstants.RIGHT));
			s.addGbcComponent(this, 2, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  activeProject);
			s.addGbcComponent(this, 3, y, 1, 1, 100, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			s.addGbcComponent(this, 4, y, 1, 1, 1, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new ButtonDelProject());
			s.addGbcComponent(this, 5, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			s.addGbcComponent(this, 1, y, 4, 1, 1000, 1000, 
							  GridBagConstraints.BOTH,
							  projectsPanel);
			s.addGbcComponent(this, 5, y, 1, 1, 10, 1, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());

			//&y++;
			//&opy = y; addOptionsPanel();
			
			//&setSize(600,200);
			//&y++;
			//&s.addExtraButtonLine(this,0, y,6,1,1,1, buttons,true);

		}
	}
	
	public static class OptionsForProjectsSelfStandingPanel extends JPanel {

		ProjectOptionsPanel panel;			

		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				s.getProbableParent(this).setVisible(false);
			}
		}
		class ButtonSave extends JButton implements ActionListener {
			ButtonSave() {super("Save"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				panel.saveProjectSettings();
			}
		}
		class ButtonClose extends JButton implements ActionListener {
			ButtonClose() {super("Continue"); addActionListener(this);}
			public void actionPerformed(ActionEvent e) {
				panel.saveProjectSettings();
				s.getProbableParent(this).setVisible(false);
			}
		}
	
		OptionsForProjectsSelfStandingPanel(Options[] xrefrc, String defaultp, Component parent) {
			int y = -1;	
			JButton[] buttons = new JButton[] {
				new ButtonCancel(),
				new ButtonSave(),
				new ButtonClose(),
			};

			setLayout(new GridBagLayout());
			panel = new ProjectOptionsPanel(xrefrc, defaultp);

			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 1000, 1000, 
							  GridBagConstraints.BOTH,
							  panel);
			y++;
			s.addGbcComponent(this, 0, y, 1, 1, 10, 10, 
							  GridBagConstraints.HORIZONTAL,
							  new JPanel());
			y++;
			s.addExtraButtonLine(this, 0,y, 1,1, 1,1, buttons, true);
		}
			
	}

	void init(JFrame parent, boolean newProject) {
		try {
			OptionParser parser = new OptionParser(new File(s.configurationFile));
			Options[] xrefrc = parser.parseFile();
			//&Options.dump(xrefrc);
			String defaultProject = s.projectAutoDetectionName;
			if (! newProject) defaultProject = s.activeProject;
			OptionsForProjectsSelfStandingPanel op = new OptionsForProjectsSelfStandingPanel(xrefrc, defaultProject, parent);
			setContentPane(op);
			pack();
			setSize(600,400);
			setLocationRelativeTo(parent);
			if (newProject) {
				boolean created = op.panel.addNewProject(parent);
				if (created) setVisible(true);
			} else {
				setVisible(true);
			}
		} catch(Exception e) {
			e.printStackTrace(System.err);
		}
	}

	public OptionsForProjectsDialog(JFrame parent) {
		super(parent);
		init(parent, false);
	}
	public OptionsForProjectsDialog(JFrame parent, boolean newProject) {
		super(parent);
		init(parent, newProject);
	}
}

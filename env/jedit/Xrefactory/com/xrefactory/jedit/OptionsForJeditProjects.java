package com.xrefactory.jedit;

import java.io.*;
import org.gjt.sp.jedit.*;
import com.xrefactory.jedit.Options.*;
import com.xrefactory.jedit.OptionsForProjectsDialog.*;
import java.awt.*;

public class OptionsForJeditProjects extends AbstractOptionPane {

	ProjectOptionsPanel		panel = null;
	
	protected void _init() {
		try {
			s.setGlobalValues(s.getParentView(this), false);
	       	OptionParser parser = new OptionParser(new File(s.configurationFile));
			Options[] xrefrc = parser.parseFile();
			//&Options.dump(xrefrc);
			if (panel!=null) remove(panel);
			panel = new ProjectOptionsPanel(xrefrc, s.activeProject);
			setLayout(new GridBagLayout());
			s.addGbcComponent(this, 0, 0, 1, 1, 10000, 10000, 
							  GridBagConstraints.BOTH,
							  panel);
		} catch (Exception e) {
			e.printStackTrace(System.err);
		}
	}

	

	protected void _save() {
		panel.saveProjectSettings();
	}

	public OptionsForJeditProjects() {
		super("xrefactory-projects");
	}
}



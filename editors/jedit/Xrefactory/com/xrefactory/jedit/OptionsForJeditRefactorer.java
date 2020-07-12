package com.xrefactory.jedit;

import java.io.*;
import javax.swing.*;
import org.gjt.sp.jedit.*;
import com.xrefactory.jedit.Options.*;
import com.xrefactory.jedit.OptionsForProjectsDialog.*;
import java.awt.*;
import java.awt.event.*;

public class OptionsForJeditRefactorer extends AbstractOptionPane {

	JCheckBox preferImportsOnDemand;
	JCheckBox updateBeforeRefactorings;
	JCheckBox saveFilesBeforeRefactorings;
	JCheckBox saveFilesAfterRefactorings;
	JCheckBox saveFilesAskForConfirmation;

	JComboBox commentaryMovingLevel;

	protected void _init() {
		s.setGlobalValues(s.getParentView(this), false);

		commentaryMovingLevel = new JComboBox(
			new String[]{
				"do not move comments",
				"move single // comment", 
				"move single /* */ comment", 
				"move single // and /* */ comment", 
				"move all // comments", 
				"move all /* */ comments", 
				"move all comments",
			});
		commentaryMovingLevel.setEditable(false);
		commentaryMovingLevel.setSelectedIndex(jEdit.getIntegerProperty(s.optRefactoryCommentMovingLevel, 0));
		addComponent("Comments moving strategy: ", commentaryMovingLevel);

		preferImportsOnDemand = new JCheckBox(
			"Prefer imports on demand when generating import statements",
			jEdit.getBooleanProperty(s.optRefactoryPreferImportOnDemand, true));
		addComponent(preferImportsOnDemand);

		saveFilesBeforeRefactorings = new JCheckBox(
			"Save all modified buffers before refactoring",
			Opt.saveFilesBeforeRefactorings());
		addComponent(saveFilesBeforeRefactorings);

		updateBeforeRefactorings = new JCheckBox(
			"Update tags before refactoring",
			Opt.updateBeforeRefactorings());
		addComponent(updateBeforeRefactorings);

		saveFilesAfterRefactorings = new JCheckBox(
			"Save all modified buffers after refactoring",
			Opt.saveFilesAfterRefactorings());
		addComponent(saveFilesAfterRefactorings);

		saveFilesAskForConfirmation = new JCheckBox(
			"Automatic saving of buffers asks for confirmation",
			Opt.saveFilesAskForConfirmation());
		addComponent(saveFilesAskForConfirmation);

	}


	protected void _save() {
		jEdit.setIntegerProperty(s.optRefactoryCommentMovingLevel, commentaryMovingLevel.getSelectedIndex());
		jEdit.setBooleanProperty(s.optRefactoryPreferImportOnDemand, preferImportsOnDemand.isSelected());
		Opt.setUpdateBeforeRefactorings(updateBeforeRefactorings.isSelected());
		Opt.setSaveFilesBeforeRefactorings(saveFilesBeforeRefactorings.isSelected());
		Opt.setSaveFilesAfterRefactorings(saveFilesAfterRefactorings.isSelected());
		Opt.setSaveFilesAskForConfirmation(saveFilesAskForConfirmation.isSelected());
	}

	public OptionsForJeditRefactorer() {
		super("xrefactory-refactorer");
	}
}




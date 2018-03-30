package com.xrefactory.jedit;

import java.io.*;
import javax.swing.*;
import org.gjt.sp.jedit.*;
import com.xrefactory.jedit.Options.*;
import com.xrefactory.jedit.OptionsForProjectsDialog.*;
import java.awt.*;
import java.awt.event.*;
import org.gjt.sp.jedit.gui.*;

public class OptionsForJeditCompletion extends AbstractOptionPane {

	FontSelector 		font;
	FontSelector 		symbolFont;
	ColorWellButton		foregroundColor;
	ColorWellButton		symbolForegroundColor;
	ColorWellButton		backgroundColor;
	ColorWellButton		backgroundColor2;

	JCheckBox completionCaseSensitive;
	JCheckBox completionDeletesPendingId;
	JCheckBox completionInsertsParenthesis;
	JCheckBox completionAccessCheck;
	JCheckBox completionLinkageCheck;

	JComboBox fullyQualifiedNamesLevel;
	JComboBox maxCompletions;
	JComboBox completionDialogMaxWidth;
	JComboBox completionDialogMaxHeight;

	JCheckBox completionOldDialog;

	protected void _init() {
		s.setGlobalValues(s.getParentView(this), false);

		completionCaseSensitive = new JCheckBox(
			"Case sensitive completions",
			jEdit.getBooleanProperty(s.optCompletionCaseSensitive));
		addComponent(completionCaseSensitive);

		completionDeletesPendingId = new JCheckBox(
			"Delete pending identifier",
			jEdit.getBooleanProperty(s.optCompletionDelPendingId));
		addComponent(completionDeletesPendingId);

		completionInsertsParenthesis = new JCheckBox(
			"Insert parenthesis after method",
			jEdit.getBooleanProperty(s.optCompletionInsParenthesis));
		addComponent(completionInsertsParenthesis);

		completionAccessCheck = new JCheckBox(
			"Completion checks accessibility (public/private/protected)",
			Opt.completionAccessCheck());
		addComponent(completionAccessCheck);

		completionLinkageCheck = new JCheckBox(
			"Completion reports only static symbols in static context.",
			Opt.completionLinkageCheck());
		addComponent(completionLinkageCheck);

		fullyQualifiedNamesLevel = new JComboBox(
			new String[]{
				"No fully qualified names",
				"Jars classes only", 
				"Jars and active project classes", 
				"Jars, project and class paths", 
				"Jars, project, class and source paths"
			});
		fullyQualifiedNamesLevel.setEditable(false);
		fullyQualifiedNamesLevel.setSelectedIndex(jEdit.getIntegerProperty(s.optCompletionFqtLevel,1));
		addComponent("Fully qualified names level: ", fullyQualifiedNamesLevel);

		maxCompletions = new JComboBox(
			new String[]{"100","200","300","500","1000","2000","3000","5000", "10000"}
			);
		maxCompletions.setEditable(true);
		maxCompletions.setSelectedItem(""+Opt.maxCompletions());
		addComponent("Maximal number of reported completions: ", maxCompletions);

		completionDialogMaxWidth = new JComboBox(
			new String[]{"200","400","600","800","1000","1200","1400","1600"}
			);
		completionDialogMaxWidth.setEditable(true);
		completionDialogMaxWidth.setSelectedItem(""+Opt.completionDialogMaxWidth());
		addComponent("Completion window maximal width: ", completionDialogMaxWidth);

		completionDialogMaxHeight = new JComboBox(
			new String[]{"50", "100","200","300","400","500","600","700","800","900"}
			);
		completionDialogMaxHeight.setEditable(true);
		completionDialogMaxHeight.setSelectedItem(""+Opt.completionDialogMaxHeight());
		addComponent("Completion window maximal height: ", completionDialogMaxHeight);

		addComponent("Font:", font = new FontSelector(
			jEdit.getFontProperty(s.optCompletionFont, s.defaultFont)
			));

		addComponent("Text color:", foregroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optCompletionFgColor, Color.black)),
			GridBagConstraints.VERTICAL);

		addComponent("Completed symbol font:", symbolFont = new FontSelector(
			jEdit.getFontProperty(s.optCompletionSymbolFont, s.defaultComplSymFont)
			));

		addComponent("Completed symbol text color:", symbolForegroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optCompletionSymbolFgColor, Color.black)),
			GridBagConstraints.VERTICAL);

		addComponent("Background color1:", backgroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optCompletionBgColor, s.completionBgDefaultColor)),
			GridBagConstraints.VERTICAL);

		addComponent("Background color2:", backgroundColor2 = new ColorWellButton(
			jEdit.getColorProperty(s.optCompletionBgColor2, s.completionBgDefaultColor2)),
			GridBagConstraints.VERTICAL);

		completionOldDialog = new JCheckBox(
			"Use old implementation of completion dialog",
			jEdit.getBooleanProperty(s.optCompletionOldDialog));
		addComponent(completionOldDialog);

	}


	protected void _save() {
		jEdit.setFontProperty(s.optCompletionFont,font.getFont());
		jEdit.setFontProperty(s.optCompletionSymbolFont,symbolFont.getFont());
		jEdit.setColorProperty(s.optCompletionFgColor,foregroundColor.getSelectedColor());
		jEdit.setColorProperty(s.optCompletionSymbolFgColor,symbolForegroundColor.getSelectedColor());
		jEdit.setColorProperty(s.optCompletionBgColor,backgroundColor.getSelectedColor());
		jEdit.setColorProperty(s.optCompletionBgColor2,backgroundColor2.getSelectedColor());
		jEdit.setIntegerProperty(s.optCompletionFqtLevel, fullyQualifiedNamesLevel.getSelectedIndex());
		jEdit.setBooleanProperty(s.optCompletionCaseSensitive, completionCaseSensitive.isSelected());
		jEdit.setBooleanProperty(s.optCompletionDelPendingId, completionDeletesPendingId.isSelected());
		jEdit.setBooleanProperty(s.optCompletionInsParenthesis, completionInsertsParenthesis.isSelected());
		jEdit.setBooleanProperty(s.optCompletionOldDialog, completionOldDialog.isSelected());

		Opt.setCompletionAccessCheck(completionAccessCheck.isSelected());
		Opt.setCompletionLinkageCheck(completionLinkageCheck.isSelected());
		try {
			Opt.setMaxCompletions(Integer.parseInt(
				(String)maxCompletions.getSelectedItem()));
		} catch (Exception e) {}
		try {
			Opt.setCompletionDialogMaxWidth(Integer.parseInt(
				(String)completionDialogMaxWidth.getSelectedItem()));
		} catch (Exception e) {}
		try {
			Opt.setCompletionDialogMaxHeight(Integer.parseInt(
				(String)completionDialogMaxHeight.getSelectedItem()));
		} catch (Exception e) {}
	}

	public OptionsForJeditCompletion() {
		super("xrefactory-completion");
	}
}




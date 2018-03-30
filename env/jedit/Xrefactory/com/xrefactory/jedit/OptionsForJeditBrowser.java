package com.xrefactory.jedit;

import java.io.*;
import javax.swing.*;
import org.gjt.sp.jedit.*;
import com.xrefactory.jedit.Options.*;
import com.xrefactory.jedit.OptionsForProjectsDialog.*;
import java.awt.*;
import java.awt.event.*;
import org.gjt.sp.jedit.gui.*;

public class OptionsForJeditBrowser extends AbstractOptionPane {

	JCheckBox fullAutoUpdate;
	JCheckBox updateBeforePush;
	JCheckBox referencePushingsKeepBrowserFilter;
	JCheckBox askBeforeBrowsingJavadoc;
	JCheckBox fileNamesCaseUnSensitive;
	JCheckBox srcWithReferences;

	JComboBox toolBarPosition;

	FontSelector 		font;
	ColorWellButton		foregroundColor;
	ColorWellButton		backgroundColor;
	ColorWellButton		selectionColor;
	ColorWellButton		symbolColor;
	ColorWellButton		nmSymbolColor;

	protected void _init() {
		s.setGlobalValues(s.getParentView(this), false);

		referencePushingsKeepBrowserFilter = new JCheckBox(
			"Keep reference filter over pushing (in browser)",
			Opt.referencePushingsKeepBrowserFilter());
		addComponent(referencePushingsKeepBrowserFilter);

		askBeforeBrowsingJavadoc = new JCheckBox(
			"Ask for confirmation before browsing URL",
			Opt.askBeforeBrowsingJavadoc());
		addComponent(askBeforeBrowsingJavadoc);

		updateBeforePush = new JCheckBox(
			"Update tags before pushing references",
			Opt.updateBeforePush());
		addComponent(updateBeforePush);

		fullAutoUpdate = new JCheckBox(
			"Updating of tags provides full update (with dependencies checks)",
			Opt.fullAutoUpdate());
		addComponent(fullAutoUpdate);

		srcWithReferences = new JCheckBox(
			"Display one line of source code along references",
			jEdit.getBooleanProperty(s.optBrowserSrcWithRefs, false));
		addComponent(srcWithReferences);

		/*&   // too dangerous
		  fileNamesCaseUnSensitive = new JCheckBox(
		  "File names are considered case unsensitive",
		  (projects.length>0 && projects[0].contains(fileNamesCaseUnsensitiveOpt)));
		  addComponent(fileNamesCaseUnSensitive);
		  &*/

		//&addComponent(new JPanel());

		toolBarPosition = new JComboBox(
			new String[]{"Left", "Right", "Top", "Bottom"}
			);
		toolBarPosition.setEditable(false);
		toolBarPosition.setSelectedItem(jEdit.getProperty(s.optBrowserToolBarPos));
		addComponent("Toolbar position: ", toolBarPosition);

		addComponent("Font:", font = new FontSelector(
			jEdit.getFontProperty(s.optBrowserFont, s.browserDefaultFont)
			));

		addComponent("Text color:", foregroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optBrowserFgColor, Color.black)),
			GridBagConstraints.VERTICAL);

		addComponent("Background color:", backgroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optBrowserBgColor, Color.white)),
			GridBagConstraints.VERTICAL);

		addComponent("Tree selection color:", selectionColor = new ColorWellButton(
			jEdit.getColorProperty(s.optBrowserSelectionColor, Color.blue)),
			GridBagConstraints.VERTICAL);

		addComponent("Tree symbol color:", symbolColor = new ColorWellButton(
			jEdit.getColorProperty(s.optBrowserSymbolColor, Color.red)),
			GridBagConstraints.VERTICAL);

		addComponent("Tree non member symbol color:", nmSymbolColor = new ColorWellButton(
			jEdit.getColorProperty(s.optBrowserNmSymbolColor, s.light_gray)),
			GridBagConstraints.VERTICAL);

	}


	protected void _save() {
		Opt.fullAutoUpdate(fullAutoUpdate.isSelected());
		Opt.setAskBeforeBrowsingJavadoc(askBeforeBrowsingJavadoc.isSelected());
		Opt.setUpdateBeforePush(updateBeforePush.isSelected());
		Opt.setReferencePushingsKeepBrowserFilter(referencePushingsKeepBrowserFilter.isSelected());
		
		jEdit.setProperty(s.optBrowserToolBarPos, (String)toolBarPosition.getSelectedItem());
		jEdit.setFontProperty(s.optBrowserFont,font.getFont());
		jEdit.setColorProperty(s.optBrowserFgColor,foregroundColor.getSelectedColor());
		jEdit.setColorProperty(s.optBrowserBgColor,backgroundColor.getSelectedColor());
		jEdit.setColorProperty(s.optBrowserSelectionColor,selectionColor.getSelectedColor());
		jEdit.setColorProperty(s.optBrowserSymbolColor,symbolColor.getSelectedColor());
		jEdit.setColorProperty(s.optBrowserNmSymbolColor,nmSymbolColor.getSelectedColor());
		jEdit.setBooleanProperty(s.optBrowserSrcWithRefs, srcWithReferences.isSelected());

		s.setupCtreeFontsAndColors();
		s.updateBrowserVisage();
	}

	public OptionsForJeditBrowser() {
		super("xrefactory-browser");
	}
}




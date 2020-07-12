package com.xrefactory.jedit;

import java.io.*;
import javax.swing.*;
import org.gjt.sp.jedit.*;
import com.xrefactory.jedit.Options.*;
import com.xrefactory.jedit.OptionsForProjectsDialog.*;
import java.awt.*;
import java.awt.event.*;
import org.gjt.sp.jedit.gui.*;

public class OptionsForJeditRetriever extends AbstractOptionPane {

	FontSelector 		font;
	ColorWellButton		foregroundColor;
	ColorWellButton		backgroundColor;

	protected void _init() {
		s.setGlobalValues(s.getParentView(this), false);
		addComponent("Font:", font = new FontSelector(
			jEdit.getFontProperty(s.optRetrieverFont, s.defaultFont)
			));

		addComponent("Text color:", foregroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optRetrieverFgColor, Color.black)),
			GridBagConstraints.VERTICAL);

		addComponent("Background color:", backgroundColor = new ColorWellButton(
			jEdit.getColorProperty(s.optRetrieverBgColor, s.light_gray)),
			GridBagConstraints.VERTICAL);
	}


	protected void _save() {
		jEdit.setFontProperty(s.optRetrieverFont,font.getFont());
		jEdit.setColorProperty(s.optRetrieverFgColor,foregroundColor.getSelectedColor());
		jEdit.setColorProperty(s.optRetrieverBgColor,backgroundColor.getSelectedColor());

		DockableSymbolRetriever rr = (DockableSymbolRetriever)
			s.view.getDockableWindowManager().getDockableWindow(s.dockableRetrieverWindowName);
		if (rr!=null) rr.resetTextProperties();
	}

	public OptionsForJeditRetriever() {
		super("xrefactory-retriever");
	}
}




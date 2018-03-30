package com.xrefactory.jedit;

import java.awt.*;
import java.io.*;
import javax.swing.*;

public class XrefactorerTask extends XrefTask {

	public XrefactorerTask(XrefStringArray options) {
		xrefOutputFile = s.getNewTmpFileName();
		restartable = false;
		try {
			startingOption.clear();
			startingOption.add("-refactory");
			// browse url via direct URL link. ? Is this necessary here ?
			startingOption.add("-urldirect");
			// out file
			startingOption.add("-o");
			startingOption.add(xrefOutputFile.getAbsolutePath());
			// todo get active project first
			startingOption.add(options);
			addFileProcessingOptions(startingOption);
			startThis();
		} catch(Exception e) {
			if (s.debug) e.printStackTrace(System.err);
			JOptionPane.showMessageDialog(s.view, "While passing file to task: "+ e, 
										  "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
		}
	}
}

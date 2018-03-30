package com.xrefactory.jedit;

import java.awt.*;

public class BrowserTask extends XrefTask {

	public BrowserTask() {
		xrefOutputFile = s.getNewTmpFileName();
		restartable = true;
		startingOption.clear();
		startingOption.add("-task_regime_server");
		// set line number for completion formatting to suffisient size
		startingOption.add("-olinelen=10000");
		// browse url via direct URL link.
		startingOption.add("-urldirect");
		// out file
		startingOption.add("-o");
		startingOption.add(xrefOutputFile.getAbsolutePath());
		try {startThis();} catch (Exception eee) {};
	}
}

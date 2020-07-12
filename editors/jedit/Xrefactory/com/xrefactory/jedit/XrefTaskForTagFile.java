package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.io.*;

public class XrefTaskForTagFile  extends XrefTask implements Runnable {

	DispatchData				data;
	boolean 					showReport;
	static XrefTaskForTagFile 	activeProcess = null;
	static boolean				running = false;

	Object 				lock = new String("This thread lock");

	public void run() {
		activeProcess = this;
		try {
			startThis();
			XrefCharBuffer report = getTaskOutput(data);
			if (showReport) {
				//not data.callerComponent as parent, because of O.K. in options
				// not s.view, because of dead windw on Apply button
				s.showTagFileReport(report, data.callerComponent); 
			}
		} catch (Exception eee) {
			if (s.debug) eee.printStackTrace(System.err);
			JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent),
										  eee, "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
		}
		killThis(false);
		activeProcess = null;
		running = false;
		synchronized (lock) {
			lock.notify();
		}
	}


	public XrefTaskForTagFile(String opt, DispatchData data, boolean showReport) {
		xrefOutputFile = new File(s.tagProcessingReportFile);
		keepOutputFile = true;
		this.data = data;
		this.showReport = showReport;
		restartable = false;
		startingOption.clear();
		startingOption.add("-o");
		startingOption.add(xrefOutputFile.getAbsolutePath());
		String project = s.getOrComputeActiveProject();
		if (project == null) return;  // no project
		s.displayProjectInformationLater();
		startingOption.add("-p");
		startingOption.add(project);
		startingOption.add(opt);
		startingOption.add("-errors");
		addModifiedFilesOptions(startingOption, true);
	}

	public static boolean passCheckForRunningProcess(String message, Component caller) {
		while (running) {
			int res = JOptionPane.showConfirmDialog(caller, message, "Tag processing", JOptionPane.YES_NO_CANCEL_OPTION);
			if (res == JOptionPane.YES_OPTION) {
				// verify once more time, maybe exited in between
				if (activeProcess!=null) {
					activeProcess.killThis(false);
				}
			} else if (res == JOptionPane.CANCEL_OPTION) {
				return(false);
			}
		}
		return(true);
	}

	public static XrefTaskForTagFile runXrefOnTagFile(String option, String message,boolean showReport, DispatchData data) {
		if (! passCheckForRunningProcess("A Tag maintenance process is yet running. Kill it?", data.callerComponent)) {
			return(null);
		}
		DispatchData ndata = new DispatchData(data);
		ndata.progressMessage = message;
		XrefTaskForTagFile task = new XrefTaskForTagFile(option, ndata, showReport);
		running = true;
		//& Thread t = new Thread(task);
		task.run(); //t.start();
		return(task);
	}
 
}

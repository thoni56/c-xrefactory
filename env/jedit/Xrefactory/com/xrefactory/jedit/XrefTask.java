package com.xrefactory.jedit;

import org.gjt.sp.jedit.textarea.*;
import java.io.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.text.*;
import java.util.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.io.*;

public class XrefTask {

	public boolean killed = false;

	public Process pp = null;
	public OutputStream oo = null;
	public InputStream ii = null;

	public boolean restartable = true;
	XrefStringArray startingOption = new XrefStringArray();
	XrefStringArray tmpOption = new XrefStringArray();

	Progress progress = null;

	LinkedList 		temporaryFiles = new LinkedList();
	File 			xrefOutputFile;
	boolean			keepOutputFile = false;

	static class TmpBufferFile {
		Buffer 	buffer;
		File 	savedAsFile;
		TmpBufferFile(Buffer buffer, File file) {
			this.buffer = buffer;
			this.savedAsFile = file;
		}
	}

	public void killThis(boolean verbose) {
		killed = true;
		if (s.debug) System.err.println("killing the process");
		if (progress!=null) {
			progress.setVisible(false);
			progress = null;
		}
		if (pp!=null) {
			pp.destroy();
			pp = null;
			if (verbose) {
				SwingUtilities.invokeLater(new s.MessageDisplayer("Xrefactory task killed",true));
			}
		} else {
			if (verbose) {
				SwingUtilities.invokeLater(new s.MessageDisplayer("No task to kill",true));
			}
		}
		if (oo != null) {
			try {oo.close();} catch (IOException e) {}
			oo = null;
		}
		if (ii != null) {
			try {ii.close();} catch (IOException e) {}
			ii = null;
		}
		try {deleteXrefOutputFile();} catch (Exception e) {}
		try {removeTemporaryFiles();} catch (Exception e) {}
	}
	
	public void startThis() throws IOException {
		Runtime rt = Runtime.getRuntime();
		XrefStringArray ex = new XrefStringArray();

		ex.add(s.xrefTaskPath);
		// we are using xref2 protocol
		ex.add("-xrefactory-II");
		// editor
		ex.add("-editor=jedit");
		// configuration file
		ex.add("-xrefrc="+s.configurationFile);
		// use crlf conversion
		ex.add("-crlfconversion");
		ex.add("-crconversion");
		//
		ex.add(startingOption);

		String[] cmds = ex.toCmdArray();
		if (s.debug) {
			String dd = "";
			for(int i=0; i<cmds.length; i++) dd += cmds[i] + " ";
			System.err.println(dd);
		}
		pp = rt.exec(cmds);
		oo = pp.getOutputStream();
		ii = pp.getInputStream();
		killed = false;
	}

	public void saveEditedBufferToTemporaryFile(Buffer buffer, String tmpFile) {
		FileOutputStream off;
		File ofl;
		try {
			/*
			  off = new FileOutputStream(ofl);
			  DataOutputStream odf = new DataOutputStream(off);
			  odf.writeBytes(buffer.getText(0, buffer.getLength()));
			  off.close();
			*/
			buffer.save(s.view, tmpFile, false);
			ofl = new File(tmpFile);
			temporaryFiles.add(new TmpBufferFile(buffer, ofl));
		} catch (Exception e) {
			if (s.debug) e.printStackTrace(System.err);
			JOptionPane.showMessageDialog(s.view, "While saving tmp files: " + e, 
										  "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
		}
	}

	void deleteXrefOutputFile() {
		if (xrefOutputFile!=null && xrefOutputFile.exists() && !s.debug) {
			xrefOutputFile.delete();
		}
	}

	void removeTemporaryFiles() {
		if (! keepOutputFile) deleteXrefOutputFile();
		while (! temporaryFiles.isEmpty()) {
			TmpBufferFile ff = (TmpBufferFile)temporaryFiles.getFirst();
			temporaryFiles.removeFirst();
//&System.err.println("removing tmo file " + ff.savedAsFile.getAbsolutePath());
			if (! s.debug) {
				ff.savedAsFile.delete();
			}
		}
	}

	void passeBufferThrougTmpFile(XrefStringArray args, Buffer buff) {
		File xrefTmpFile;
		String xrefTmpFilename;
		xrefTmpFile = s.getNewTmpFileName();
		xrefTmpFilename = xrefTmpFile.getAbsolutePath();
		// preload opened buffer from temporary file
		args.add("-preload");
		args.add(buff.getPath());
		args.add(xrefTmpFilename);
		saveEditedBufferToTemporaryFile(buff, xrefTmpFilename);
	}

	public synchronized void addModifiedFilesOptions(XrefStringArray args, boolean addCurrent) {
		File xrefTmpFile;
		String xrefTmpFilename;
		int i;
		Buffer[] buffs = jEdit.getBuffers();
		Buffer cb = s.getBuffer();
		for(i=0; i<buffs.length; i++) {
			if (buffs[i].isDirty()) {
				if (buffs[i]!=cb || addCurrent) {
					passeBufferThrougTmpFile(args, buffs[i]);
				}
			}
		}
		VFSManager.waitForRequests();
	}

	public synchronized void addCurrentFileOptions(XrefStringArray args) {
		Selection selection = s.getTextArea().getSelectionAtOffset(s.getTextArea().getCaretPosition());
		passeBufferThrougTmpFile(args, s.getBuffer());
		VFSManager.waitForRequests();
		args.add(s.getBuffer().getPath());
		if (selection == null) {
			args.add("-olcursor=" + s.getTextArea().getCaretPosition());
			args.add("-olmark=" + s.getTextArea().getCaretPosition());
		} else {
			args.add("-olcursor=" + selection.getEnd());
			args.add("-olmark=" + selection.getStart());
		}
	}

	public void addFileProcessingOptions(XrefStringArray args) {
		addModifiedFilesOptions(args, false);
		// add input file, must be after -preload
		addCurrentFileOptions(args);
	}


	void sendDataToRunningProcess(String data) throws IOException {
		if (s.debug) System.err.println("sending "+data);
		PrintStream ss = new PrintStream(oo);
		ss.println(data);
		oo.flush();
		if (s.debug) System.err.println("sent");
	}

	public XrefCharBuffer getTaskOutput(DispatchData data) {
		XrefCharBuffer 	res = new XrefCharBuffer();
		int 			c;
		boolean 		loop, loop2;
		c = ' '; loop = true;
 		try {
			while (loop) {
				res.clear();
				while (c != -1 && Character.isWhitespace((char)c)) c = ii.read();
				while (c != -1 && loop) {
					// in reality this is just pipe synchronisation, 
					// real answer is written to a file, here just 
					// loop until a <synchro> record is readed
					res.append(""+(char)c);
					if (s.debug) System.err.println("got : " + c + "==" + (char)c);
					if (c=='>') loop = false;
					c = ii.read();
				}
				loop = false;
				if (res.toString().equals("<"+Protocol.PPC_PROGRESS+">")) {
					loop2 = true;
					int val = 0;
					while (c != -1 && loop2) {
						if (c>='0' && c<='9') {
							val = val*10 + c - '0';
						} else {
							loop2 = false;
						}
						c = ii.read();
					}
					if (s.debug) System.err.println("progress == "+val);
					if (progress==null) progress = Progress.crNew(data.callerComponent, data.progressMessage);
					if (s.debug) System.err.println("setting progress to "+val);
					if (! progress.setProgress(val)) {
						killThis(true);
						loop = false;
					} else {
						loop = true;
					}
					//&res = null;
				} else {
					if (! res.toString().equals("<"+Protocol.PPC_SYNCHRO_RECORD+">")) {
						if (killed) {
							// O.K. process was regularly killed by another thread
							res.clear();
						} else {
							if (pp!=null) pp.destroy();
							if (ii!=null) {
								while (c != -1) {
									res.append(""+(char)c);
									c = ii.read();
								}
							}
							if (s.debug) System.err.println("kill due to synchro problem");
							new Exception().printStackTrace(System.err);
							JOptionPane.showMessageDialog(
								s.getProbableParent(data.callerComponent),
								"Internal Error, synchro problem: " + res.toString(), 
								"Xrefactory Error",
								JOptionPane.ERROR_MESSAGE);
							//& throw new XrefException("'"+res.toString()+"'");
							res.clear();
							if (s.debug) {
								res.appendFileContent(xrefOutputFile);
							}
							killThis(false);
						}
					} else {
						res.clear();
						// all this stuff should be reorganized to be readable
						res.appendFileContent(xrefOutputFile);
						// you can also delete it here (I think)
						deleteXrefOutputFile();
						removeTemporaryFiles();
					}
					if (progress!=null) {
						progress.setVisible(false);
						progress = null;
					}
				}
			}
		} catch (Exception e) {
			if (s.debug) e.printStackTrace();
			JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent), 
										  "Communication problem: " + e, 
										  "Xrefactory Error",
										  JOptionPane.ERROR_MESSAGE);
			//res.clear();
		}
		return(res);
	}
	
	public synchronized XrefCharBuffer callProcess(XrefStringArray args, DispatchData data) {
		String				putargs = null;
		XrefCharBuffer 		res = new XrefCharBuffer();
		int 				i,c,reslen,lmlen;
		boolean 			loop,freshProcess=false;
		char        		pilot;

		// first clean output file, to avoid interferences
		// no additional options here, (because of continue after resolution dialog)
		putargs = args.toString();
 		putargs += " end-of-options\n";
 		try {
			if (pp == null && restartable) startThis();
			sendDataToRunningProcess(putargs);
		} catch (Exception e) {
			if (! restartable) {
				if (s.debug) e.printStackTrace();
				JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent),
											  "Internal Error: process exited " + e, 
											  "Xrefactory Error",
											  JOptionPane.ERROR_MESSAGE);
				return(res);
			}
			killThis(false);
			try {	
				startThis();
				sendDataToRunningProcess(putargs);
			} catch (Exception e2) {
				killThis(false);
				if (s.debug) e2.printStackTrace();
				JOptionPane.showMessageDialog(s.getProbableParent(data.callerComponent),
											  "Internal Error: can't start process: " + e2, 
											  "Xrefactory Error",
											  JOptionPane.ERROR_MESSAGE);
				if (s.debug) e.printStackTrace();
				return(res);
			}
		}
		res = getTaskOutput(data);
		//& removeTemporaryFiles();
		return(res);
	}

	public static void addCommonOptions(XrefStringArray options, DispatchData data) {
		options.add("-user");
		options.add(s.getViewParameter(data.viewId));
		String project = Opt.activeProject();
		if (s.activeProject != null) {
			options.add("-p");
			options.add(s.activeProject);
		}
		if (! jEdit.getBooleanProperty(s.optBrowserSrcWithRefs, false)) options.add("-rlistwithoutsrc");
	}

	public XrefCharBuffer callProcessOnFile(XrefStringArray args, DispatchData data) {
		XrefCharBuffer res = new XrefCharBuffer();
		FileOutputStream off = null;
		File ofl = null;
		try {
			XrefStringArray options = new XrefStringArray(args);
			addFileProcessingOptions(options);
			addCommonOptions(options, data);
			res = callProcess(options, data);
		} catch(Exception e) {
			if (s.debug) e.printStackTrace();
			JOptionPane.showMessageDialog(s.view, "Internal Error while passing file to task: " + e, 
										  "Xrefactory Error", 
										  JOptionPane.ERROR_MESSAGE);
		}
		return(res);
	}

	public XrefCharBuffer callProcess(String[] options, DispatchData data) {
		tmpOption.clear();
		for(int i=0; i<options.length; i++) {
			if (options[i]!=null) tmpOption.add(options[i]);
		}
		addCommonOptions(tmpOption, data);
		return(callProcess(tmpOption, data));
	}
	public XrefCharBuffer callProcessSingleOpt(String option, DispatchData data) {
		return(callProcess(new String[]{option}, data));
	}
	public XrefCharBuffer callProcessOnFileNoSaves(String[] options, DispatchData data) {
		tmpOption.clear();
		for(int i=0; i<options.length; i++) {
			tmpOption.add(options[i]);
		}
		addCommonOptions(tmpOption, data);
		addCurrentFileOptions(tmpOption);
		return(callProcess(tmpOption, data));
	}
	public XrefCharBuffer callProcessOnFile(String[] options, DispatchData data) {
		tmpOption.clear();
		for(int i=0; i<options.length; i++) {
			tmpOption.add(options[i]);
		}
		return(callProcessOnFile(tmpOption, data));
	}
	public XrefCharBuffer callProcessOnFileSingleOpt(String option, DispatchData data) {
		tmpOption.clear();
		if (option!=null) tmpOption.add(option);
		return(callProcessOnFile(tmpOption, data));
	}

}


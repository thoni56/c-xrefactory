package com.xrefactory.jedit;

import org.gjt.sp.jedit.*;
import javax.swing.*;
import org.gjt.sp.jedit.io.*;
import java.io.*;

public abstract class Refactorings {

	String 				param;

	// abstract methods
	abstract int getCode();
	abstract public String toString();
	void perform() {
		if (s.debug) new Exception().printStackTrace(System.err);
		JOptionPane.showMessageDialog(s.view, "Not yet implemented", "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
	}

	static void setMovingTarget() {
		s.targetFile = s.getFileName();
		s.targetLine = s.getTextArea().getCaretLine()+1;
	}

	static public class No_refactoring extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_NO_REFACTORING);
		}
		public String toString() {
			return("No Refactoring");
		}
		void perform() {
		}
	}
	static public class Rename_symbol extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_RENAME_SYMBOL);
		}
		public String toString() {
			return("Rename");
		}

		void perform() {
			RenameDialog dd = new RenameDialog("symbol", s.getIdentifierOnCaret());
			String newname = dd.getNewName();
			renameSymbol(newname);
		}
	}
	static public class Rename_class extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_RENAME_CLASS);
		}
		public String toString() {
			return("Rename Class");
		}

		void perform() {
			RenameDialog dd = new RenameDialog("class",s.getIdentifierOnCaret());
			String newname = dd.getNewName();
			renameClass(newname,false);
		}
	}
	static public class Rename_package extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_RENAME_PACKAGE);
		}
		public String toString() {
			return("Rename Package");
		}
		void perform() {
			RenameDialog dd = new RenameDialog("package", s.dotifyString(param));
			String newname = dd.getNewName();
			if (newname!=null) {
				XrefStringArray xroption = new XrefStringArray();
				xroption.add("-rfct-rename-package");
				xroption.add("-renameto="+newname);
				mainRefactorerInvocation(xroption,false);
			}
		}
	}
	static public class Add_parameter extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_ADD_PARAMETER);
		}
		public String toString() {
			return("Add Parameter");
		}
		void perform() {
			AddParameterDialog d = new AddParameterDialog(s.getIdentifierOnCaret());
			int position = Integer.parseInt(d.getPosition());
			String definition = d. getDefinition();
			String usage = d.getUsage();
			if (position>0 && (!definition.equals("")) && (!usage.equals(""))) {
				XrefStringArray xroption = new XrefStringArray();
				xroption.add("-rfct-add-param");
				xroption.add("-olcxparnum="+position);
				xroption.add("-rfct-param1="+definition);
				xroption.add("-rfct-param2="+usage);
				mainRefactorerInvocation(xroption,false);
			}
		}
	}
	static public class Del_parameter extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_DEL_PARAMETER);
		}
		public String toString() {
			return("Delete Parameter");
		}
		void perform() {
			DelParameterDialog d = new DelParameterDialog(s.getIdentifierOnCaret());
			int position = Integer.parseInt(d.getPosition());
			if (position>0) {
				XrefStringArray xroption = new XrefStringArray();
				xroption.add("-rfct-del-param");
				xroption.add("-olcxparnum="+position);
				mainRefactorerInvocation(xroption,false);
			}
		}
	}
	static public class Move_parameter extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_MOVE_PARAMETER);
		}
		public String toString() {
			return("Move Parameter");
		}
		void perform() {
			MoveParameterDialog d = new MoveParameterDialog(s.getIdentifierOnCaret());
			int position = Integer.parseInt(d.getPosition());
			int position2 = Integer.parseInt(d.getPosition2());
			if (position>0 && position2>0) {
				XrefStringArray xroption = new XrefStringArray();
				xroption.add("-rfct-move-param");
				xroption.add("-olcxparnum="+position);
				xroption.add("-olcxparnum2="+position2);
				mainRefactorerInvocation(xroption,false);
			}
		}
	}
	static public class Extract_method extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_EXTRACT_METHOD);
		}
		public String toString() {
			return("Extract Method");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-extract-method");
			mainRefactorerInvocation(xroption, true);
			if (s.xExtractMethod!=null) {
				String def = s.getBuffer().getLineText(s.xExtractMethod.definitionLine-1);
				int offset = def.indexOf("newMethod_");
				if (offset != -1) {
					s.moveToPosition(s.view, s.getFileName(), s.xExtractMethod.definitionLine, offset);
					renameSymbol(s.xExtractMethod.name.getText());
				} else {
					offset = def.indexOf("NewClass_");
					if (offset != -1) {
						s.moveToPosition(s.view, s.getFileName(), s.xExtractMethod.definitionLine, offset);
						renameClass(s.upperCaseFirstLetter(s.xExtractMethod.name.getText()), true);
						String lcdef = s.getBuffer().getLineText(s.xExtractMethod.invocationLine-1);
						offset = lcdef.indexOf("newClass_");
						if (offset != -1) {
							s.moveToPosition(s.view, s.getFileName(), s.xExtractMethod.invocationLine, offset);
							renameSymbol(s.lowerCaseFirstLetter(s.xExtractMethod.name.getText()));
						}
					}
				}
			}
		}
	}
	static public class Extract_function extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_EXTRACT_FUNCTION);
		}
		public String toString() {
			return("Extract Function");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-extract-method");
			mainRefactorerInvocation(xroption, true);
			if (s.xExtractMethod!=null) {
				String def = s.getBuffer().getLineText(s.xExtractMethod.definitionLine-1);
				int offset = def.indexOf("newFunction_");
				if (offset != -1) {
					s.moveToPosition(s.view, s.getFileName(), s.xExtractMethod.definitionLine, offset);
					renameSymbol(s.xExtractMethod.name.getText());
				}
			}
		}
	}
	static public class Extract_macro extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_EXTRACT_MACRO);
		}
		public String toString() {
			return("Extract Macro");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-extract-macro");
			mainRefactorerInvocation(xroption, true);
			if (s.xExtractMethod!=null) {
				String def = s.getBuffer().getLineText(s.xExtractMethod.definitionLine-1);
				int offset = def.indexOf("NEW_MACRO_");
				if (offset != -1) {
					s.moveToPosition(s.view, s.getFileName(), s.xExtractMethod.definitionLine, offset);
					renameSymbol(s.xExtractMethod.name.getText());
				}
			}
		}
	}
	static public class Move_static_field extends Refactorings {
		MoveDialog d;
		int getCode() {
			return(Protocol.PPC_AVR_MOVE_STATIC_FIELD);
		}
		public String toString() {
			return("Move Static Field");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Move static field '"+name+"'", false, "-rfct-move-static-field","-olcxmmtarget");
		}
	}
	static public class Move_static_method extends Refactorings {
		MoveDialog d;
		int getCode() {
			return(Protocol.PPC_AVR_MOVE_STATIC_METHOD);
		}
		public String toString() {
			return("Move Static Method");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Move static method '"+name+"'", false, "-rfct-move-static-method","-olcxmmtarget");
		}
	}
	static public class Move_class extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_MOVE_CLASS);
		}
		public String toString() {
			return("Move Class");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Move class '"+name+"'", false, "-rfct-move-class","-olcxmctarget");
		}
	}
	static public class Move_class_to_new_file extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_MOVE_CLASS);
		}
		public String toString() {
			return("Move Class To New File");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			s.Position cpos = s.getPosition(s.view);
			JFileChooser chooser = new JFileChooser();
			chooser.setSelectedFile(new File((new File(s.getBuffer().getPath())).getParentFile().getAbsolutePath()+s.slash+name+".java"));
			chooser.setDialogTitle("Select file for class moving");
			int returnVal = chooser.showSaveDialog(s.view);
			if(returnVal == JFileChooser.APPROVE_OPTION) {
				File ff = chooser.getSelectedFile();
				if (ff.getAbsolutePath().equals(cpos.file)) {
					JOptionPane.showMessageDialog(
						s.view,
						"The file contains the class to be moved. Can not clear it and then move.", 
						"Xrefactory Error",
						JOptionPane.ERROR_MESSAGE);
				} else {
					s.moveToPosition(s.view, ff.getAbsolutePath(), 0);
					if (ff.exists()) {
						int confirm = JOptionPane.YES_OPTION;
						confirm = JOptionPane.showConfirmDialog(
							s.view, 
							"File "+ff+" exists. Can I erase it first?",
							"Confirmation", 
							JOptionPane.YES_NO_OPTION,
							JOptionPane.QUESTION_MESSAGE);
						if (confirm == JOptionPane.YES_OPTION) {
							s.getTextArea().setText("");
						}
					}
					setMovingTarget();
					s.moveToPosition(cpos);
					s.performMovingRefactoring("-rfct-move-class-to-new-file", null);
				}
			}
		}
	}
	static public class Move_field extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_MOVE_FIELD);
		}
		public String toString() {
			return("Move Field");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Move field '"+name+"'", true, "-rfct-move-field","-olcxmmtarget");
		}
	}
	static public class Pull_up_field extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_PULL_UP_FIELD);
		}
		public String toString() {
			return("Pull Up Field");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Pull up field '"+name+"'", false, "-rfct-pull-up-field","-olcxmmtarget");
		}
	}
	static public class Pull_up_method extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_PULL_UP_METHOD);
		}
		public String toString() {
			return("Pull Up Method");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Pull up method '"+name+"'", false, "-rfct-pull-up-method","-olcxmmtarget");
		}
	}
	static public class Push_down_field extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_PUSH_DOWN_FIELD);
		}
		public String toString() {
			return("Push Down Field");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Push down field '"+name+"'", false, "-rfct-push-down-field","-olcxmmtarget");
		}
	}
	static public class Push_down_method extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_PUSH_DOWN_METHOD);
		}
		public String toString() {
			return("Push Down Method");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new MoveDialog("Push down method '"+name+"'", false, "-rfct-push-down-method","-olcxmmtarget");
		}
	}
	static public class Encapsulate_field extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_ENCAPSULATE_FIELD);
		}
		public String toString() {
			return("Encapsulate Field");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-encapsulate-field");
			mainRefactorerInvocation(xroption,false);
		}
	}
	static public class Self_encapsulate_field extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_SELF_ENCAPSULATE_FIELD);
		}
		public String toString() {
			return("Self Encapsulate Field");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-self-encapsulate-field");
			mainRefactorerInvocation(xroption,false);
		}
	}
	static public class Turn_dynamic_method_to_static extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC);
		}
		public String toString() {
			return("Turn Virtual Method to Static");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new TurnDynamicToStaticDialog(name);
		}
	}
	static public class Turn_static_method_to_dynamic extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC);
		}
		public String toString() {
			return("Turn Static Method to Virtual");
		}
		void perform() {
			String name = s.getIdentifierOnCaret();
			new TurnStaticToDynamicDialog(name);
		}
	}
	static public class Reduce_names extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_REDUCE_NAMES);
		}
		public String toString() {
			return("Reduce Names");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-reduce");
			mainRefactorerInvocation(xroption,false);
		}
	}
	static public class Expand_names extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_EXPAND_NAMES);
		}
		public String toString() {
			return("Expand Names");
		}
		void perform() {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-expand");
			mainRefactorerInvocation(xroption,false);
		}
	}
	static public class Set_move_target extends Refactorings {
		int getCode() {
			return(Protocol.PPC_AVR_SET_MOVE_TARGET);
		}
		public String toString() {
			return("Set Target for Next Moving Refactoring");
		}
		void perform() {
			setMovingTarget();
			SwingUtilities.invokeLater(new s.MessageDisplayer("Next moving refactoring will move to "+new File(s.getBuffer().getPath()).getName()+":"+s.targetLine,false));
		}
	}

	public static Refactorings getRefactoringFromCode(int code) {
		Refactorings res = null;
		switch(code) {
		case Protocol.PPC_AVR_NO_REFACTORING:
			res = new No_refactoring();
			break;
		case Protocol.PPC_AVR_RENAME_SYMBOL:
			res = new Rename_symbol();
			break;
		case Protocol.PPC_AVR_RENAME_CLASS:
			res = new Rename_class();
			break;
		case Protocol.PPC_AVR_RENAME_PACKAGE:
			res = new Rename_package();
			break;
		case Protocol.PPC_AVR_ADD_PARAMETER:
			res = new Add_parameter();
			break;
		case Protocol.PPC_AVR_DEL_PARAMETER:
			res = new Del_parameter();
			break;
		case Protocol.PPC_AVR_MOVE_PARAMETER:
			res = new Move_parameter();
			break;
		case Protocol.PPC_AVR_EXTRACT_METHOD:
			res = new Extract_method();
			break;
		case Protocol.PPC_AVR_EXTRACT_FUNCTION:
			res = new Extract_function();
			break;
		case Protocol.PPC_AVR_EXTRACT_MACRO:
			res = new Extract_macro();
			break;
		case Protocol.PPC_AVR_MOVE_STATIC_FIELD:
			res = new Move_static_field();
			break;
		case Protocol.PPC_AVR_MOVE_STATIC_METHOD:
			res = new Move_static_method();
			break;
		case Protocol.PPC_AVR_MOVE_FIELD:
			res = new Move_field();
			break;
		case Protocol.PPC_AVR_PULL_UP_FIELD:
			res = new Pull_up_field();
			break;
		case Protocol.PPC_AVR_PULL_UP_METHOD:
			res = new Pull_up_method();
			break;
		case Protocol.PPC_AVR_PUSH_DOWN_FIELD:
			res = new Push_down_field();
			break;
		case Protocol.PPC_AVR_PUSH_DOWN_METHOD:
			res = new Push_down_method();
			break;
		case Protocol.PPC_AVR_MOVE_CLASS:
			res = new Move_class();
			break;
		case Protocol.PPC_AVR_MOVE_CLASS_TO_NEW_FILE:
			res = new Move_class_to_new_file();
			break;
		case Protocol.PPC_AVR_ENCAPSULATE_FIELD:
			res = new Encapsulate_field();
			break;
		case Protocol.PPC_AVR_SELF_ENCAPSULATE_FIELD:
			res = new Self_encapsulate_field();
			break;
		case Protocol.PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC:
			res = new Turn_dynamic_method_to_static();
			break;
		case Protocol.PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC:
			res = new Turn_static_method_to_dynamic();
			break;
		case Protocol.PPC_AVR_REDUCE_NAMES:
			res = new Reduce_names();
			break;
		case Protocol.PPC_AVR_EXPAND_NAMES:
			res = new Expand_names();
			break;
		case Protocol.PPC_AVR_SET_MOVE_TARGET:
			res = new Set_move_target();
			break;
		default:
			res = null;
			JOptionPane.showMessageDialog(s.view, "Unknown refactoring code: " + code, 
										  "Xrefactory Internal Error", JOptionPane.ERROR_MESSAGE);
		}
		return(res);
	}

	static class ApplyRefactoring implements Runnable {
		XrefCharBuffer	receipt;
		DispatchData 	ndata;
		public void run() {
			Dispatch.dispatch(receipt, ndata);
			if ((!ndata.panic) && Opt.saveFilesAfterRefactorings()) s.saveAllBuffers(s.view);
		}
		ApplyRefactoring(XrefCharBuffer receipt, DispatchData ndata) {
			this.receipt = receipt;
			this.ndata = ndata;
		}
	}

	static class RefactoringsRunnable implements Runnable {
		XrefStringArray				runOptions;
		static boolean 				running;
		static XrefactorerTask		activeProcess;

		public static boolean passCheckForRunningProcess() {
			while (running) {
				int res = JOptionPane.showConfirmDialog(s.view, "A refactoring process is running, kill it?", "Xrefactory", JOptionPane.YES_NO_CANCEL_OPTION);
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

		public void run() {
			if (! passCheckForRunningProcess()) return;
			running = true;
			String project = null;
			if (Opt.saveFilesBeforeRefactorings()) s.saveAllBuffers(s.view);
			if (Opt.updateBeforeRefactorings()) {
				boolean panic = s.synchronizedUpdateTagFile(s.view);
				if (panic) {
					running = false;
					return;
				}
			} else {
				// also check that there is no Tag updating running
				if (! XrefTaskForTagFile.passCheckForRunningProcess("Tags are currently being created or updated. Can I kill this process?", s.view)) {
					running = false;
					return;
				}
			}
			project = s.activeProject; // getActiveProject(s.view);
			if (project!=null) {
				XrefStringArray taskOptions = new XrefStringArray();
				DispatchData ndata = new DispatchData(s.view);
				taskOptions.add("-p");
				taskOptions.add(project);
				taskOptions.add(runOptions);
				taskOptions.add("-user");
				taskOptions.add(s.getViewParameter(ndata.viewId));
				if (jEdit.getBooleanProperty(s.optRefactoryPreferImportOnDemand, true)) {
					taskOptions.add("-addimportdefault=0");
				} else {
					taskOptions.add("-addimportdefault=1");
				}
				activeProcess = new XrefactorerTask(taskOptions);
				ndata.xTask = activeProcess;
				ndata.progressMessage = "Computing patches.";
				XrefCharBuffer receipt = activeProcess.getTaskOutput(ndata);

				new ApplyRefactoring(receipt, ndata).run();
				//&VFSManager.runInAWTThread(new ApplyRefactoring(receipt, ndata));
				//&VFSManager.waitForRequests();
			}
			running = false;
		}


		RefactoringsRunnable(XrefStringArray runOptions) {
			this.runOptions = runOptions;
		}
	}

	public static void renameSymbol(String newname) {
		if (newname!=null) {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-rename");
			xroption.add("-renameto=" + newname);
			mainRefactorerInvocation(xroption, false);
		}
	}

	public static void renameClass(String newname, boolean synchro) {
		if (newname!=null) {
			XrefStringArray xroption = new XrefStringArray();
			xroption.add("-rfct-rename-class");
			xroption.add("-renameto="+newname);
			mainRefactorerInvocation(xroption, synchro);
		}
	}

	static boolean 		running;

	public static void mainRefactorerInvocation(XrefStringArray options,
												boolean synchronisation
		) {
		RefactoringsRunnable r = new RefactoringsRunnable(options);
		if (synchronisation) {
			r.run();
		} else {
			//&Thread t = new Thread(new RefactoringsRunnable(options));
			r.run(); //t.start();
		}
	}


}


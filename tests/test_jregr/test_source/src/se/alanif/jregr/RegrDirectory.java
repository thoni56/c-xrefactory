package se.alanif.jregr;

import java.io.FilenameFilter;
import java.util.ArrayList;

import se.alanif.jregr.exec.RegrCase;
import se.alanif.jregr.io.Directory;
import se.alanif.jregr.io.File;

public class RegrDirectory {

	private static final long serialVersionUID = 1L;

	public static final String COMMANDS_FILE_NAME = ".jregr";

	private static final String OUTPUT_EXTENSION = ".output";
	private static final String SUSPENDED_EXTENSION = ".suspended";
	private static final String EXPECTED_EXTENSION = ".expected";
	private String case_extension;

	private Directory directory;
	
	private FilenameFilter caseNameFilter = new FilenameFilter() {
		public boolean accept(java.io.File dir, String name) {
			return isCaseName(name);
		}
	};

	private Runtime runtime;

	public RegrDirectory(Directory directory, Runtime runtime) {
		this.directory = directory;
		this.runtime = runtime;
		CommandsDecoder commandsDecoder = new CommandsDecoder(directory.getBufferedReaderForFile(getCommandsFile()));
		commandsDecoder.reset();
		case_extension = commandsDecoder.getExtension();
	}

	private String stripExtension(String fileName) {
		if (fileName.endsWith(case_extension))
			return fileName.substring(0, fileName.length()-case_extension.length());
		else
			return fileName;
	}

	public String getName() {
		return directory.getName();
	}

	public String getPath() {
		return directory.getAbsolutePath();
	}

	public boolean hasCases() {
		return getCases().length > 0;
	}

	public RegrCase[] getCases() {
		String[] fileNames = directory.list(caseNameFilter);
		ArrayList<RegrCase> cases = new ArrayList<RegrCase>();
		if (fileNames != null && fileNames.length > 0) {
			for (String string : fileNames) {
				cases.add(new RegrCase(runtime, stripExtension(string), this));
			}
		}
		return cases.toArray(new RegrCase[cases.size()]);
	}

	public RegrCase[] getCases(String[] files) {
		// Could be without extension if input from command line
		ArrayList<RegrCase> cases = new ArrayList<RegrCase>();
		if (files != null && files.length > 0) {
			for (String filename : files) {
				if (isCaseName(filename))
					cases.add(new RegrCase(runtime, stripExtension(filename), this));
			}
		}
		return cases.toArray(new RegrCase[cases.size()]);
	}

	public Directory toDirectory() {
		return directory;
	}

	public File getCommandsFile() {
		return directory.getFile(COMMANDS_FILE_NAME);
	}

	public boolean hasOutputFile(String caseName) {
		return directory.hasFile(caseName+OUTPUT_EXTENSION);
	}

	public boolean hasExpectedFile(String caseName) {
		return directory.hasFile(caseName+EXPECTED_EXTENSION);
	}

	public boolean hasSuspendedFile(String caseName) {
		return directory.hasFile(caseName+SUSPENDED_EXTENSION);
	}

	public File getOutputFile(String caseName) {
		return directory.getFile(caseName+OUTPUT_EXTENSION);
	}

	public File getExpectedFile(String caseName) {
		return directory.getFile(caseName+EXPECTED_EXTENSION);
	}

	public boolean hasCaseFile(String caseName) {
		return directory.hasFile(caseName+case_extension);
	}

	private boolean isCaseName(String name) {
		if (case_extension.length() == 0)
			return false;
		else if (name.endsWith(case_extension))
			return true;
		else
			return directory.hasFile(name+case_extension);
	}

}

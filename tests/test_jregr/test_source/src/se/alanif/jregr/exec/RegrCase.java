package se.alanif.jregr.exec;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;

import se.alanif.jregr.CommandsDecoder;
import se.alanif.jregr.RegrDirectory;
import se.alanif.jregr.io.Directory;
import se.alanif.jregr.io.File;

public class RegrCase {

	// Status values for cases
	public enum State {VIRGIN, PENDING, FAIL, FATAL, PASS, SUSPENDED, SUSPENDED_FAIL, SUSPENDED_PASS}

	private Runtime runtime;
	private String caseName;
	private RegrDirectory regrDirectory;
	private boolean fatal = false;

	public RegrCase(Runtime runtime, String caseName, RegrDirectory directory) {
		this.caseName = caseName;
		this.runtime = runtime;
		this.regrDirectory = directory;
	}

	public void run(Directory binDirectory, CommandsDecoder decoder, PrintWriter outputWriter, CaseRunner caseRunner, ProcessBuilder processBuilder) {
		int linenumber = 1;
		outputWriter.printf("########## %s ##########\n", caseName);
		try {
			do {
				Process process = processBuilder.exec(binDirectory, decoder, regrDirectory.toDirectory(), runtime, caseName);
				final String stdin = decoder.getStdin(caseName);
				final StreamPusher inputPusher;
				if (stdin == null)
					inputPusher = null;
				else {
					final FileReader inputReader = new FileReader(regrDirectory.getPath()+File.separator+stdin);
					inputPusher = new StreamPusher(process.getOutputStream(), inputReader);
				}
				String output = caseRunner.run(process, new StreamGobbler(process.getErrorStream()), new StreamGobbler(process.getInputStream()), inputPusher);
				outputWriter.print(output);
				linenumber++;
			} while (decoder.advance());
		} catch (FileNotFoundException e) {
			// did not find the .input file, but that might not be a problem, could be a virgin test case
			// but it could also be a mistake in the .jregr file, so print it unless we are using built in...
			if (!decoder.usingDefault())
				outputWriter.print("Could not find input file for command line " + linenumber + " in .jregr file\n");
		} catch (IOException e) {
			fatal  = true;
			outputWriter.print(e.getMessage());
		} finally {
			outputWriter.close();
		}
	}

	private void removeOutputFile() {
		if (!getOutputFile().delete())
			System.out.println("Error : could not delete output file");
	}

	private boolean outputSameAsExpected() {
		BufferedReader expectedReader = null;
		BufferedReader outputReader = null;
		try {
			File expectedFile = getExpectedFile();
			if (expectedFile.exists()) {
				File outputFile = getOutputFile();
				try {
					expectedReader = new BufferedReader(new FileReader(expectedFile));
					outputReader = new BufferedReader(new FileReader(outputFile));
				} catch (FileNotFoundException e1) {
					return false;
				}

				String outputLine = "";
				String expectedLine = "";
				while (outputLine != null && expectedLine != null) {
					if (!outputLine.equals(expectedLine))
						return false;
					try {
						outputLine = outputReader.readLine();
						expectedLine = expectedReader.readLine();
					} catch (IOException e) {
					}
				}
				return outputLine == null && expectedLine == null;
			}
			return false;
		} finally {
			try {
				if (outputReader != null) outputReader.close();
				if (expectedReader != null) expectedReader.close();
			} catch (IOException e) {
			}
		}
	}

	public void clean() {
		if (outputSameAsExpected())
			removeOutputFile();
	}

	public String getName() {
		return caseName;
	}
	
	public String toString() {
		return getName();
	}

	public State status() {
		if (fatal) return State.FATAL;
		boolean isSuspended = false;
		if (regrDirectory.hasSuspendedFile(caseName))
			isSuspended = true;
		if (!regrDirectory.hasExpectedFile(caseName) && !regrDirectory.hasOutputFile(caseName))
			return isSuspended?State.SUSPENDED:State.VIRGIN;
		if (!regrDirectory.hasExpectedFile(caseName) && regrDirectory.hasOutputFile(caseName))
			return isSuspended?State.SUSPENDED:State.PENDING;
		if (regrDirectory.hasExpectedFile(caseName) && !regrDirectory.hasOutputFile(caseName))
			return isSuspended?State.SUSPENDED_PASS:State.PASS;
		return isSuspended?State.SUSPENDED_FAIL:State.FAIL;
	}

	public boolean failed() {
		return fatal || status() == State.FAIL;
	}

	public boolean exists() {
		return regrDirectory.hasCaseFile(caseName);
	}

	public File getOutputFile() {
		return regrDirectory.getOutputFile(caseName);
	}

	public File getExpectedFile() {
		return regrDirectory.getExpectedFile(caseName);
	}

}

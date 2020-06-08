package se.alanif.jregr.reporters;

import org.apache.commons.cli.CommandLine;

import se.alanif.jregr.exec.RegrCase;
import se.alanif.jregr.exec.RegrCase.State;

public interface RegrReporter {

	public void start(String suite, int numberOfTests, CommandLine commandLine);
	public void starting(RegrCase theCase, long millis);
	public void fatal();
	public void virgin();
	public void pending();
	public void pass();
	public void fail();
	public void suspended();
	public void end();
	public void suspendedAndFailed();
	public void suspendedAndPassed();
	public void report(State status);

}

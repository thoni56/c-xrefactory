package se.alanif.jregr.reporters;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintStream;

import org.apache.commons.cli.CommandLine;

import se.alanif.jregr.diff.Diff;
import se.alanif.jregr.exec.RegrCase;
import se.alanif.jregr.io.Directory;
import se.alanif.jregr.io.File;

// TODO XMLReporter should really use some XML library like JDOM

public class XMLReporter extends AbstractRegrReporter {

	private String suiteName;
	private RegrCase theCase;
	private PrintStream out = System.out;
	private long millis;

	public XMLReporter(Directory regrDirectory) throws FileNotFoundException {
		File xmlFile = regrDirectory.getFile("TEST-jregr.xml");
		OutputStream xmlStream = new FileOutputStream(xmlFile);
		out = new PrintStream(xmlStream);
	}

	public void start(String suiteName, int numberOfTests, CommandLine commandLine) {
		this.suiteName = suiteName;
		out.println("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>");
		out.println("<testsuite name=\""+suiteName+"\">");
	}

	public void starting(RegrCase caseName, long millis) {
		this.theCase = caseName;
		this.millis = millis;
	}

	public void fatal() {
		fatal++;
		header(theCase.getName(), millis);
		out.println("    <error type=\"Fatal\" message=\"Case '"+theCase+"' failed to complete\">");
		out.println("    </error>");
		tail();
	}

	public void virgin() {
		virgin++;
		header(theCase.getName(), millis);
		out.println("    <error type=\"Virgin\" message=\"No input defined for case '"+theCase+"'\">");
		out.println("      The file '"+theCase+".input' does not exist");
		out.println("    </error>");
		tail();
	}

	public void pending() {
		pending++;
		header(theCase.getName(), millis);
		out.println("    <error type=\"Pending\" message=\"No expected output defined for case '"+theCase+"'\">");
		out.println("      The file '"+theCase+".expected' does not exist");
		out.println("    </error>");
		tail();
	}

	public void fail() {
		failing++;
		header(theCase.getName(), millis);
		out.println("    <failure message=\"Output does not match expected\">");
		insertDiff(theCase, out);
        out.println("    </failure>");
		tail();
	}

	private void insertDiff(RegrCase theCase, PrintStream outputStream) {
		out.println("        <![CDATA[Compared to the expected output, the actual has");
		new Diff(out).doDiff(theCase.getExpectedFile(), theCase.getOutputFile());
		outputStream.println("]]>");
	}

	public void pass() {
		passing++;
		header(theCase.getName(), millis);
		insertExpectedOutput(theCase, out);
		tail();
	}

	private void insertExpectedOutput(RegrCase theCase, PrintStream outputStream) {
		outputStream.println("        <![CDATA[");
		try {
			BufferedReader expectedReader = new BufferedReader(new FileReader(theCase.getExpectedFile()));
			String line = "";
			while ((line = expectedReader.readLine()) != null)
					outputStream.println(line);
			expectedReader.close();
		} catch (FileNotFoundException e) {
		} catch (IOException e) {
		}
		outputStream.println("]]>");
	}

	public void suspended() {
		suspended++;
		header(theCase.getName(), 0);
		out.println("    <skipped />");
		tail();
	}

	public void suspendedAndFailed() {
		suspended();
	}

	public void suspendedAndPassed() {
		suspended();
	}

	public void end() {
		out.println("</testsuite>");
	}

	private void header(String caseName, long millis) {
		out.println("  <testcase classname=\""+suiteName+"\" name=\""+caseName+"\" time=\""+(float)millis/1000+"\">");
	}

	private void tail() {
		out.println("  </testcase>");
	}

}

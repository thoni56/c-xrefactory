package se.alanif.jregr.exec;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;

import se.alanif.jregr.CommandsDecoder;
import se.alanif.jregr.RegrDirectory;
import se.alanif.jregr.exec.RegrCase.State;
import se.alanif.jregr.io.Directory;
import se.alanif.jregr.io.File;
import junit.framework.TestCase;
import org.junit.Test;
import static org.mockito.Mockito.*;

public class RegrCaseTest extends TestCase {

	private static final String CASENAME = "theCase";

	private static final String BIN_DIRECTORY_PATH = "directory";
	private static final String BIN_DIRECTORY_PATH_WITH_SEPARATOR = BIN_DIRECTORY_PATH+Directory.separator;

	private static final String COMMAND1 = "alan";
	private static final String COMMAND1_PREPENDED_WITH_BIN_DIRECTORY = BIN_DIRECTORY_PATH_WITH_SEPARATOR+COMMAND1;
	private static final String ARGUMENT1 = "arg11";

	private static final String[] COMMAND1_AND_ARGUMENTS = new String[] {COMMAND1_PREPENDED_WITH_BIN_DIRECTORY, ARGUMENT1};
	private static final String[] COMMAND1_AND_CASENAME = new String[] {COMMAND1_PREPENDED_WITH_BIN_DIRECTORY, CASENAME};

	private static final String COMMAND2 = "command2";
	private static final String COMMAND2_PREPENDED_WITH_BIN_DIRECTORY = BIN_DIRECTORY_PATH_WITH_SEPARATOR+COMMAND2;
	private static final String ARGUMENT2_1 = "arg1";
	private static final String ARGUMENT2_2 = "-opt";
	private static final String[] COMMAND2_AND_ARGUMENTS = {COMMAND2_PREPENDED_WITH_BIN_DIRECTORY, ARGUMENT2_1, ARGUMENT2_2};


	private CommandsDecoder mockedDecoder = mock(CommandsDecoder.class);
	private Runtime mockedRuntime = mock(Runtime.class);
	private Directory binDirectory = mock(Directory.class);

	private Directory mockedDirectory = mock(Directory.class);
	private RegrDirectory mockedRegrDirectory = mock(RegrDirectory.class);
	private CaseRunner mockedCaseRunner = mock(CaseRunner.class);
	private PrintWriter mockedPrinter = mock(PrintWriter.class);

	private Process mockedProcess = mock(Process.class);
	private ProcessBuilder mockedProcessBuilder = mock(ProcessBuilder.class);

	private RegrCase theCase = new RegrCase(mockedRuntime, CASENAME, mockedRegrDirectory);

	private InputStream mockedInputStream = mock(InputStream.class);

	private OutputStream mockedOutputStream = mock(OutputStream.class);

	private File mockedExpectedFile = mock(File.class);
	
	public void setUp() throws Exception {
		when(binDirectory.getAbsolutePath()).thenReturn(BIN_DIRECTORY_PATH);
		when(mockedRuntime.exec((String[])anyObject())).thenReturn(mockedProcess);
		when(mockedProcessBuilder.exec(eq(binDirectory), eq(mockedDecoder), (Directory)anyObject(), eq(mockedRuntime), eq(CASENAME))).thenReturn(mockedProcess);
		when(mockedProcess.getErrorStream()).thenReturn(mockedInputStream);
		when(mockedProcess.getInputStream()).thenReturn(mockedInputStream);
		when(mockedProcess.getOutputStream()).thenReturn(mockedOutputStream);
		when(mockedRegrDirectory.getExpectedFile(CASENAME)).thenReturn(mockedExpectedFile);
		when(mockedRegrDirectory.toDirectory()).thenReturn(mockedDirectory);
	}

	@Test
	public void testShouldExecTheCommandAndArgumentsFromTheDecoder() throws Exception {
		when(mockedDecoder.buildCommandAndArguments(binDirectory, CASENAME)).thenReturn(COMMAND1_AND_CASENAME);
		
		theCase.run(binDirectory, mockedDecoder, mockedPrinter, mockedCaseRunner, mockedProcessBuilder);
		
		verify(mockedProcessBuilder).exec(binDirectory, mockedDecoder, mockedDirectory, mockedRuntime, CASENAME);
	}
	
	@Test
	public void testShouldRunByExecutingEveryCommand() throws Exception {
		when(mockedDecoder.buildCommandAndArguments(binDirectory, CASENAME))
			.thenReturn(COMMAND1_AND_ARGUMENTS)
			.thenReturn(COMMAND2_AND_ARGUMENTS);
		when(mockedDecoder.advance())
			.thenReturn(true)
			.thenReturn(false);

		theCase.run(binDirectory, mockedDecoder, mockedPrinter, mockedCaseRunner, mockedProcessBuilder);

		verify(mockedProcessBuilder, times(2)).exec(binDirectory, mockedDecoder, mockedDirectory, mockedRuntime, CASENAME);
	}

	@Test
	public void testShouldReturnPassIfExpectedButNoOutputFileExists() throws Exception {
		when(mockedRegrDirectory.hasExpectedFile(theCase.getName())).thenReturn(true);
		when(mockedRegrDirectory.hasOutputFile(theCase.getName())).thenReturn(false);
		
		assertEquals(State.PASS, theCase.status());
	}

	@Test
	public void testShouldReturnPendingIfNoExpectedButOutputFileExists() throws Exception {
		when(mockedRegrDirectory.hasExpectedFile(theCase.getName())).thenReturn(false);
		when(mockedRegrDirectory.hasOutputFile(theCase.getName())).thenReturn(true);
		
		assertEquals(State.PENDING, theCase.status());
	}

	@Test
	public void testShouldReturnFailIfExpectedAndOutputFileExists() throws Exception {
		when(mockedRegrDirectory.hasSuspendedFile(theCase.getName())).thenReturn(false);
		when(mockedRegrDirectory.hasExpectedFile(theCase.getName())).thenReturn(true);
		when(mockedRegrDirectory.hasOutputFile(theCase.getName())).thenReturn(true);
		
		assertEquals(State.FAIL, theCase.status());
	}
	
	@Test
	public void testShouldReturnSuspendedFailIfSuspendedAndOutputFileExists() throws Exception {
		when(mockedRegrDirectory.hasExpectedFile(theCase.getName())).thenReturn(true);
		when(mockedRegrDirectory.hasSuspendedFile(theCase.getName())).thenReturn(true);
		when(mockedRegrDirectory.hasOutputFile(theCase.getName())).thenReturn(true);
		
		assertEquals(State.SUSPENDED_FAIL, theCase.status());
	}

	@Test
	public void testShouldReturnSuspendedPassIfSuspendedAndOutputFileDoesntExists() throws Exception {
		when(mockedRegrDirectory.hasSuspendedFile(theCase.getName())).thenReturn(true);
		when(mockedRegrDirectory.hasExpectedFile(theCase.getName())).thenReturn(true);
		when(mockedRegrDirectory.hasOutputFile(theCase.getName())).thenReturn(false);
		
		assertEquals(State.SUSPENDED_PASS, theCase.status());
	}

	@Test
	public void testShouldWriteToOutputFileAndCloseIt() throws Exception {
		PrintWriter mockedWriter = mock(PrintWriter.class);
		when(mockedCaseRunner.run((Process)anyObject(), (StreamGobbler)anyObject(), (StreamGobbler)anyObject(), (StreamPusher)anyObject())).thenReturn("");
		
		theCase.run(binDirectory, mockedDecoder, mockedWriter, mockedCaseRunner, mockedProcessBuilder);
		
		verify(mockedWriter, atLeastOnce()).print("");
		verify(mockedWriter).close();
	}
	
	@Test
	public void testCanSeeIfACaseExists() throws Exception {
		when(mockedRegrDirectory.hasCaseFile(CASENAME)).thenReturn(true);
		assertTrue(theCase.exists());
	}
	
	@Test
	public void testCanGetOutputFile() throws Exception {
		File mockedOutputFile = mock(File.class);
		when(mockedRegrDirectory.getOutputFile(CASENAME)).thenReturn(mockedOutputFile);
		assertEquals(mockedOutputFile, theCase.getOutputFile());
	}
	
}

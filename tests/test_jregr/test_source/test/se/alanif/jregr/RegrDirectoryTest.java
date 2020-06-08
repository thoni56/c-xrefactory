package se.alanif.jregr;

import static org.mockito.Matchers.anyObject;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.io.BufferedReader;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;

import junit.framework.TestCase;

import org.junit.Test;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import se.alanif.jregr.exec.RegrCase;
import se.alanif.jregr.io.Directory;
import se.alanif.jregr.io.File;

public class RegrDirectoryTest extends TestCase {

	private static final String DEFAULT_EXTENSION = ".alan";
	private static final String EXTENSION = ".extension";

	private static final String CASENAME1 = "test1";
	
	private static final String CASEFILENAME_WITH_DEFAULT_EXTENSION_1 = CASENAME1+DEFAULT_EXTENSION;
	private static final String CASEFILENAME_WITH_EXTENSION_1 = CASENAME1+EXTENSION;
	private static final String FILENAME1 = "test1";
	private static final String FILENAME2 = "file2";

	private static final String[] NO_FILENAMES = new String[] {};
	private static final String[] ONE_FILENAME_WITH_DEFAULT_EXTENSION = new String[] { CASEFILENAME_WITH_DEFAULT_EXTENSION_1 };
	private static final String[] ONE_FILENAME_WITH_EXTENSION = new String[] { CASEFILENAME_WITH_EXTENSION_1 };
	private static final String[] TWO_FILENAMES_ONE_MATCHING_DEFAULT_EXTENSION = new String[] { CASEFILENAME_WITH_DEFAULT_EXTENSION_1, FILENAME2 };
	private static final String[] TWO_FILENAMES_NONE_MATCHING_DEFAULT_EXTENSION = new String[] { FILENAME1, FILENAME2 };
	private static final String[] TWO_FILENAMES_NONE_MATCHING_EXTENSION = new String[] { FILENAME1, FILENAME2 };

	private final Directory mockedDirectoryWithoutCommandsFile = mock(Directory.class);
	private final Directory mockedDirectoryWithCommandsFile = mock(Directory.class);

	private final Runtime mockedRuntime = mock(Runtime.class);
	private final File mockedFile = new File("mockedFile");

	private RegrDirectory regrDirectoryWithoutCommandsFile;
	private RegrDirectory regrDirectoryWithCommandsFile;
	
	private File jregrFile = new File(RegrDirectory.COMMANDS_FILE_NAME);	

	private static final RegrCase[] NO_CASES = new RegrCase[] {};


	protected void setUp() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.getFile(CASENAME1+".output")).thenReturn(mockedFile);
		when(mockedDirectoryWithoutCommandsFile.hasFile(CASENAME1+DEFAULT_EXTENSION)).thenReturn(true);
		when(mockedDirectoryWithoutCommandsFile.getFile(RegrDirectory.COMMANDS_FILE_NAME)).thenReturn(null);

		when(mockedDirectoryWithCommandsFile.getFile(RegrDirectory.COMMANDS_FILE_NAME)).thenReturn(jregrFile);

		mockedFile.deleteOnExit();
		
		BufferedReader mockedBufferReader = mock(BufferedReader.class);
		when(mockedDirectoryWithCommandsFile.getBufferedReaderForFile((File)anyObject())).thenReturn(mockedBufferReader);
		when(mockedBufferReader.readLine()).thenReturn(EXTENSION+" : command");

		regrDirectoryWithoutCommandsFile = new RegrDirectory(mockedDirectoryWithoutCommandsFile, mockedRuntime);
		regrDirectoryWithCommandsFile = new RegrDirectory(mockedDirectoryWithCommandsFile, mockedRuntime);
	}
	
	private class TestMatcher implements Answer<String[]> {
		private String[] filenames;

		public TestMatcher(String[] filenames) {
			this.filenames = filenames;
		}

		public String[] answer(InvocationOnMock invocation) {
			FilenameFilter filter = (FilenameFilter) invocation.getArguments()[0];
			ArrayList<String> matchedFilenames = new ArrayList<String>();
			for (String name : filenames)
				if (filter.accept(null, name))
					matchedFilenames.add(name);
			String[] stringArray = new String[matchedFilenames.size()];
			return matchedFilenames.toArray(stringArray);
		}
	}

	@Test
	public void testDirectoryReturnsNoCasesForEmptyDirectoryWithoutCommandsFile() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(NO_FILENAMES));
		
		assertTrue(Arrays.equals(NO_CASES, regrDirectoryWithoutCommandsFile.getCases()));
	}

	@Test
	public void testDirectoryReturnsNoCasesForEmptyDirectoryWithCommandsFile() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(NO_FILENAMES));
		
		assertTrue(Arrays.equals(NO_CASES, regrDirectoryWithCommandsFile.getCases()));
	}

	@Test
	public void testDirectoryReturnsNoCasesForDirectoryWithNoFileMatchingDefaultExtension() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(TWO_FILENAMES_NONE_MATCHING_DEFAULT_EXTENSION));
	
		assertTrue(Arrays.equals(NO_CASES, regrDirectoryWithoutCommandsFile.getCases()));
	}

	@Test
	public void testDirectoryReturnsNoCasesForDirectoryWithNoFileMatchingExtension() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(TWO_FILENAMES_NONE_MATCHING_EXTENSION));
		
		assertTrue(Arrays.equals(NO_CASES, regrDirectoryWithCommandsFile.getCases()));
	}

	@Test
	public void testDirectoryReturnsOneCaseForDirectoryWithSingleFileMatchingDefaultExtension() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(ONE_FILENAME_WITH_DEFAULT_EXTENSION));

		final RegrCase[] cases = regrDirectoryWithoutCommandsFile.getCases();
		assertEquals(1, cases.length);
		assertEquals(CASENAME1, cases[0].getName());
	}

	@Test
	public void testDirectoryReturnsOneCaseForDirectoryWithSingleFileMatchingExtension() throws Exception {
		when(mockedDirectoryWithCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(ONE_FILENAME_WITH_EXTENSION));

		final RegrCase[] cases = regrDirectoryWithCommandsFile.getCases();
		assertEquals(1, cases.length);
		assertEquals(CASENAME1, cases[0].getName());
	}

	@Test
	public void testDirectoryReturnsOneCaseForDirectoryWithOneFileMatchingAndOneFileNotMatchingDefaultExtension() throws Exception {
		when(mockedDirectoryWithoutCommandsFile.list((FilenameFilter) anyObject())).thenAnswer(new TestMatcher(TWO_FILENAMES_ONE_MATCHING_DEFAULT_EXTENSION));
	
		final RegrCase[] cases = regrDirectoryWithoutCommandsFile.getCases();
		assertEquals(1, cases.length);
		assertEquals(CASENAME1, cases[0].getName());
	}

	public void testRegrDirectoryReturnsJRegrFile() throws Exception {
		assertEquals(jregrFile, regrDirectoryWithCommandsFile.getCommandsFile());
	}
	
	@Test
	public void testRegrDirectoryShouldReturnOutputFile() throws Exception {
		assertEquals(mockedFile, regrDirectoryWithoutCommandsFile.getOutputFile(CASENAME1));
	}
	
	@Test
	public void testCanSeeIfCaseFileExists() throws Exception {
		assertTrue(regrDirectoryWithoutCommandsFile.hasCaseFile(CASENAME1));
	}

}

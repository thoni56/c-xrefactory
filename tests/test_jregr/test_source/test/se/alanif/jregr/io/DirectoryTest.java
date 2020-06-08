package se.alanif.jregr.io;

import java.io.BufferedReader;

import se.alanif.jregr.CommandsDecoder;
import se.alanif.jregr.io.Directory;
import junit.framework.TestCase;
import org.junit.Test;
import static org.mockito.Mockito.*;

public class DirectoryTest extends TestCase {

	private static final String EXISTING_COMMAND_NAME_WITHOUT_EXE = "ThisCommandExistsWithoutExeExtensionEGOnAMac";
	private static final String EXISTING_COMMAND_NAME_WITH_EXE = "ThisCommandExistsButRequiresExeExtensionTypicallyForWindows";
	private static final String NON_EXISTING_COMMAND_NAME = "ThisCommandDoesNotExistsAtAll";
	
	private Directory mockedDirectory = new Directory("someDirectory") {
		private static final long serialVersionUID = 1L;

		public boolean hasFile(String fileName) {
			return fileName.equals(EXISTING_COMMAND_NAME_WITHOUT_EXE) || fileName.equals(EXISTING_COMMAND_NAME_WITH_EXE+".exe");
		}
	};
	
	@Test
	public void testCanSeeIfExecutableWithoutExeExist() throws Exception {
		assertTrue(mockedDirectory.executableExist(EXISTING_COMMAND_NAME_WITHOUT_EXE));
	}

	@Test
	public void testCanSeeIfExecutableWithExeExist() throws Exception {
		assertTrue(mockedDirectory.executableExist(EXISTING_COMMAND_NAME_WITH_EXE));
	}
	
	@Test
	public void testCanSeeIfFirstExecutableFromJRegrFileExists() throws Exception {
		CommandsDecoder mockedDecoder = mock(CommandsDecoder.class);
		when(mockedDecoder.getCommand()).thenReturn(EXISTING_COMMAND_NAME_WITHOUT_EXE);
		assertTrue(mockedDirectory.executablesExist(mockedDecoder));
	}
	
	@Test
	public void testWillSayExecutablesDoNotExistsIfNoneExistsInBinDir() throws Exception {
		CommandsDecoder mockedDecoder = mock(CommandsDecoder.class);
		when(mockedDecoder.getCommand())
			.thenReturn(NON_EXISTING_COMMAND_NAME)
			.thenReturn(NON_EXISTING_COMMAND_NAME)
			.thenReturn(NON_EXISTING_COMMAND_NAME);
		when(mockedDecoder.advance())
			.thenReturn(true)
			.thenReturn(true)
			.thenReturn(false);
		assertFalse(mockedDirectory.executablesExist(mockedDecoder));
	}
	
	@Test
	public void testWillSayExecutablesExistIfLastExecutableFromJRegrFileDoes() throws Exception {
		CommandsDecoder mockedDecoder = mock(CommandsDecoder.class);
		when(mockedDecoder.getCommand())
			.thenReturn(NON_EXISTING_COMMAND_NAME)
			.thenReturn(NON_EXISTING_COMMAND_NAME)
			.thenReturn(EXISTING_COMMAND_NAME_WITH_EXE);
		when(mockedDecoder.advance())
		.thenReturn(true)
		.thenReturn(true)
		.thenReturn(false);
		assertTrue(mockedDirectory.executablesExist(mockedDecoder));
	}
	
	@Test
	public void testCanSeeIfFileExists() throws Exception {
		Directory directory = new Directory("test");
		java.io.File file = directory.getFile("file");
		file.createNewFile();
		
		assertTrue(directory.hasFile("file"));
		
		file.delete();
		assertTrue(!directory.hasFile("file"));
	}
	
	@Test
	public void testCanReturnBufferedReaderForFile() throws Exception {
		Directory directory = new Directory("test");
		java.io.File file = directory.getFile("file");
		file.createNewFile();
		BufferedReader reader = directory.getBufferedReaderForFile(directory.getFile("file"));
		assertTrue(reader instanceof BufferedReader);
		reader.close();
		
		while (!file.delete())
			;
		reader = directory.getBufferedReaderForFile(directory.getFile("file"));
		assertNull(reader);
	}
	
}

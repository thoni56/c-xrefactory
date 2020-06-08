package se.alanif.jregr.io;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;

import se.alanif.jregr.io.File;

import junit.framework.TestCase;
import org.junit.Test;

public class FileTest extends TestCase {

	private static final String ARBITRARY_TEXT1 = "Some arbitrary text";
	private static final String ARBITRARY_TEXT2 = "on two lines";

	@Test
	public void testCanReadContentStrippingCarriageReturnCharacters() throws Exception {
		String contentTestFilename = this.getClass().getName() + ".content";

		fillFileWithContent(contentTestFilename, ARBITRARY_TEXT1 + "\r\n" + ARBITRARY_TEXT2);

		File file = new File(contentTestFilename);
		assertEquals(ARBITRARY_TEXT1+"\n"+ARBITRARY_TEXT2, file.getContent());
		file.delete();
	}

	private void fillFileWithContent(String filename, String content) throws IOException {
		File contentTestFile = new File(filename);
		Writer output = new BufferedWriter(new FileWriter(contentTestFile));
		try {
			output.write(content);
		} finally {
			output.close();
		}
	}


}

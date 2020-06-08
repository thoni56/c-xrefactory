package se.alanif.jregr.exec;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import junit.framework.TestCase;
import org.junit.Test;


public class CaseRunnerTest extends TestCase {

	StreamGobbler mockedErrorGobbler = mock(StreamGobbler.class);
	StreamGobbler mockedOutputGobbler = mock(StreamGobbler.class);
	StreamPusher mockedInputPusher = mock(StreamPusher.class);
	Process p = mock(Process.class);

	CaseRunner caseRunner = new CaseRunner();

	@Test
	public void testShouldReturnEmptyOutputIfGobblersReturnNothing() throws Exception {
		when(mockedErrorGobbler.output()).thenReturn("");
		when(mockedOutputGobbler.output()).thenReturn("");

		assertEquals("", caseRunner.run(p, mockedErrorGobbler, mockedOutputGobbler, mockedInputPusher));
	}
	
	@Test
	public void testShouldReturnResultFromErrorAndOutput() throws Exception {
		when(mockedErrorGobbler.output()).thenReturn("error");
		when(mockedOutputGobbler.output()).thenReturn("output");

		assertTrue(caseRunner.run(p, mockedErrorGobbler, mockedOutputGobbler, mockedInputPusher).contains("error"));
		assertTrue(caseRunner.run(p, mockedErrorGobbler, mockedOutputGobbler, mockedInputPusher).contains("output"));
	}
}

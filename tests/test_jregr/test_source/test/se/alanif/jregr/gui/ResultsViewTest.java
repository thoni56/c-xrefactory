package se.alanif.jregr.gui;

import java.awt.Frame;

import javax.swing.JTable;

import junit.extensions.abbot.ComponentTestFixture;
import org.junit.Test;

public class ResultsViewTest extends ComponentTestFixture {

	@Test
	public void testHasResultsWindow() throws Exception {
		ResultsView resultsWindow = givenAResultsView();
		GuiTestUtilities gui = showGui(resultsWindow);
		gui.findJPanel("Results");
	}
	
	@Test
	public void testHasTestCasesTable() throws Exception {
		ResultsView resultsWindow = givenAResultsView();
		GuiTestUtilities gui = showGui(resultsWindow);
		assertNotNull(gui.findJTable("TestCases"));
	}

	private ResultsView givenAResultsView() {
		JTable jTable = new JTable();
		jTable.setName("TestCases");
		ResultsView resultsWindow = new ResultsView(jTable);
		return resultsWindow;
	}
	
	private GuiTestUtilities showGui(ResultsView view) {
		Frame frame = showFrame(view);
		GuiTestUtilities gui = new GuiTestUtilities(getFinder(), frame);
		return gui;
	}

}

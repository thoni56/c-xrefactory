package se.alanif.jregr.gui;

import java.awt.Frame;

import javax.swing.DefaultListModel;

import abbot.tester.JListTester;

import static org.mockito.Mockito.*;

import se.alanif.jregr.exec.RegrCase;

import junit.extensions.abbot.ComponentTestFixture;

import org.junit.Test;

public class RegrCaseListViewTest extends ComponentTestFixture {

	private static final String REGR_CASE_NAME = "One RegrCase Name";

	@Test
	public void testCanDisplayAnEmptyModel() throws Exception {
		DefaultListModel emptyModel = givenAnEmptyModel();
		RegrCaseListView view = givenARegrCaseListViewWith(emptyModel);
		GuiTestUtilities gui = showGui(view);
		gui.findJList("TestCases");
	}
	
	@Test
	public void testCanDisplayASingleRegrCase() throws Exception {
		DefaultListModel modelWithOneRegrCase = new DefaultListModel();
		RegrCase mockedCase = mock(RegrCase.class);
		when(mockedCase.toString()).thenReturn(REGR_CASE_NAME);
		modelWithOneRegrCase.addElement(mockedCase);
		RegrCaseListView view = givenARegrCaseListViewWith(modelWithOneRegrCase);
		JListTester tester = new JListTester();
		assertEquals(1, tester.getSize(view));
		assertEquals(REGR_CASE_NAME, (String)tester.getElementAt(view, 0).toString());
	}

	private GuiTestUtilities showGui(RegrCaseListView view) {
		Frame frame = showFrame(view);
		GuiTestUtilities gui = new GuiTestUtilities(getFinder(), frame);
		return gui;
	}

	private RegrCaseListView givenARegrCaseListViewWith(DefaultListModel model) {
		RegrCaseListView view = new RegrCaseListView(model);
		return view;
	}

	private DefaultListModel givenAnEmptyModel() {
		DefaultListModel model = new DefaultListModel();
		return model;
	}
}

package se.alanif.jregr.gui;

import javax.swing.JPanel;
import javax.swing.JTable;


public class ResultsView extends JPanel {

	private static final long serialVersionUID = 1L;

	public ResultsView() {
		this.setName("Results");
	}
	
	public ResultsView(JTable testCasesView) {
		this.setName("Results");
		this.add(testCasesView);
	}
}

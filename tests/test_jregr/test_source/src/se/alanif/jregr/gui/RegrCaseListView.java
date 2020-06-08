package se.alanif.jregr.gui;

import java.awt.Color;
import java.awt.Component;

import javax.swing.DefaultListModel;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.ListCellRenderer;

import se.alanif.jregr.exec.RegrCase;

public class RegrCaseListView extends JList {

	public class RegrCaseCellRenderer extends JLabel implements ListCellRenderer {

		private static final long serialVersionUID = 1L;

		public Component getListCellRendererComponent(JList list, Object value,
				int index, boolean isSelected, boolean cellHasFocus) {
			RegrCase theCase = (RegrCase)value; 
			setOpaque(true);
			switch (theCase.status()) {
			case PASS:
				setBackground(Color.WHITE);
				setForeground(Color.GREEN);
				break;
			case FAIL:
				setBackground(Color.WHITE);
				setForeground(Color.RED);
				break;
			case VIRGIN:
				setBackground(Color.WHITE);
				break;
			case SUSPENDED:
				setBackground(Color.YELLOW);
				setForeground(Color.BLACK);
				break;
			case SUSPENDED_FAIL:
				setBackground(Color.YELLOW);
				setForeground(Color.RED);
				break;
			case SUSPENDED_PASS:
				setBackground(Color.YELLOW);
				setForeground(Color.GREEN);
				break;
			}
			setText(theCase.getName());
			return this;
		}

	}

	private static final long serialVersionUID = 1L;

	public RegrCaseListView(DefaultListModel model) {
		setModel(model);
		setName("TestCases");
		setCellRenderer(new RegrCaseCellRenderer());
	}

}

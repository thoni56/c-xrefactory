package se.alanif.jregr.gui;

import java.awt.Container;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.WindowConstants;

public class RegrView extends JFrame implements Runnable {

	private static final long serialVersionUID = 1L;
	
	public RegrView(JComponent resultsView) {
		this.setTitle("JRegr");
		Container contentPane = this.getContentPane();
		contentPane.setName("JRegr");
		contentPane.setLayout(new BoxLayout(contentPane, BoxLayout.PAGE_AXIS));
		contentPane.add(new JButton("Run"));
		resultsView.setName("Results");
		contentPane.add(resultsView);
	}

	public void run() {
		setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
	    pack();
	    setVisible(true);
	}

}

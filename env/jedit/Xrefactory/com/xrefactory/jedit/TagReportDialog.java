package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.text.html.*;
import javax.swing.event.*;

public class TagReportDialog extends JDialog {
	public static class TagReportPanel extends JPanel {
		class ButtonClose extends JButton implements ActionListener {
			ButtonClose() {super("Close"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				JDialog dd =  s.getParentDialog(this);
				if (dd!=null) dd.setVisible(false);
			}
		}

		class HyperLinkListener implements HyperlinkListener {
			public void hyperlinkUpdate(HyperlinkEvent e) {
				if (e.getEventType() == HyperlinkEvent.EventType.ACTIVATED) {
					String url = e.getURL().toString();
					if (url.substring(0, 7).equals("file://")) {
						url = url.substring(7);
					}
					int i = url.length();
					while (i>0 && Character.isDigit(url.charAt(i-1))) i--;
					int line = Integer.parseInt(url.substring(i));
					String file = url.substring(0, i-1);
					s.moveToPosition(s.view, file, line, 0);
				}
			}
		}

		JEditorPane		tt = new JEditorPane("text/html","");

		TagReportPanel(String report) {
			int y;
			y= -1;
			JButton buttons[] = {
				new ButtonClose(),
			};
			setLayout(new GridBagLayout());

			tt.setEditable(false);
			tt.addHyperlinkListener(new HyperLinkListener());
			tt.setText(report);

			/*&
			  y++;
			  s.addGbcComponent(this, 0,y, 1,1, 10,1, 
			  GridBagConstraints.HORIZONTAL,
			  new JPanel());
			  y++;
			  s.addGbcComponent(this, 0,y, 1,1, 10,1, 
			  GridBagConstraints.HORIZONTAL,
			  new JLabel("Report:", SwingConstants.CENTER));
			  &*/
			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1000,1000, 
							  GridBagConstraints.BOTH,
							  new JScrollPane(tt));
			/*&
			  y++;
			  s.addGbcComponent(this, 0,y, 1,1, 10,1, 
			  GridBagConstraints.HORIZONTAL,
			  new JPanel());
			  &*/
			y++;
			s.addExtraButtonLine(this, 0,y, 1,1, 1,1, buttons, true);
		}
	}

	void init(String report) {
		TagReportPanel ttt = new TagReportPanel(report);
		setContentPane(ttt);
		setSize(600,400);
		setLocationRelativeTo(s.view);
		setVisible(true);
	}

	TagReportDialog(String report, JDialog parent) {
		super(parent, "Xrefactory Tag Report");
		init(report);
	}
	TagReportDialog(String report, JFrame parent) {
		super(parent, "Xrefactory Tag Report");
		init(report);
	}
	/*&
	TagReportDialog(String report) {
		super("Xrefactory Tag Report");
		init(report);
	}
	&*/
}

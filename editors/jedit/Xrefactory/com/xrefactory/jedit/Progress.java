package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import org.gjt.sp.jedit.io.*;

class Progress extends JDialog {
	public static class ProgressPanel extends JPanel {
		public class ProgressBar extends JPanel {
			public void paint(Graphics g) {
				Dimension frameDimension  = getSize();
				double    percentComplete = (double)count * 100.0 /(double)max;
				int       barPixelWidth   = ((frameDimension.width-2) * count)/ max;
				g.setColor(Color.red);
				g.fillRect(1, 1, barPixelWidth, frameDimension.height-2);
				g.setColor(Color.black);
				g.drawRect(0, 0, frameDimension.width-1, frameDimension.height-1);
			}
			public ProgressBar (int totalItems) {
				count = 0;
				max   = totalItems;
				setLayout(null);
				addNotify();
				setPreferredSize(new Dimension(400, 10));
			}
		}
		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				s.getParentDialog(this).setVisible(false);
			}
		}

		private int 		count;
		private int 		max;
		ProgressBar 		bar;

		void setProgress(int v) {
			count = v;
		}

		ProgressPanel(String message) {
			int y = -1;
			JButton buttons[] = {
				new ButtonCancel(),
			};
			bar = new ProgressBar(100);

			setLayout(new GridBagLayout());

			y++;
			s.addGbcComponent(this, 0,y, 3,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 1,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel(message));
			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(this, 1,y, 1,1, 100,1, 
							  GridBagConstraints.BOTH,
							  bar);
			s.addGbcComponent(this, 2,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 3,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addExtraButtonLine(this, 0,y, 3,1, 1,1, buttons, true);

			y++;
			s.addGbcComponent(this, 0,y, 3,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

		}
	}

	ProgressPanel 		panel;
	Component 			parent;

	public boolean setProgress(int val) {
		panel.setProgress(val);		
		paint(getGraphics());
		if (! isVisible()) {
			return(false);
		} else {
			AWTPump.pumpEventsForHierarchy(s.getParentDialog(this));
			return(isVisible());
		}
	}

	public boolean handleEvent(Event event) { 
		if (event.id == Event.WINDOW_DESTROY) { 
			dispose();
			return true;
		}
		return super.handleEvent(event);
	}

	public static Progress crNew(Component parent, String message) {
		Progress res;
		JDialog parentd = s.getParentDialog(parent);
		if (parentd!=null) {
			res = new Progress(parentd, message);
		} else {
			JFrame parentf = s.getParentFrame(parent);
			res = new Progress(parentf, message);
		}
		return(res);
	}

	void init(Component parent, String message) {
		//& if (s.javaVersion.compareTo("1.4.0") >= 0) setUndecorated(true);
		this.parent = parent;
		panel = new ProgressPanel(message);
		setContentPane(panel);
		pack();
		//& setResizable(false);
		setLocationRelativeTo(parent);
		setVisible(true);
		//& paint(getGraphics());
	}
	
	Progress(Dialog parent, String message) {
		super(parent, "Progress");
		init(parent, message);
	}

	Progress(Frame parent, String message) {
		super(parent, "Progress");
		init(parent, message);
	}
}

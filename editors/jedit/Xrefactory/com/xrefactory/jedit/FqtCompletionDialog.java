package com.xrefactory.jedit;

import java.awt.*;
import org.gjt.sp.jedit.*;
import com.xrefactory.jedit.s.*;
import javax.swing.*;
import java.awt.event.*;

public class FqtCompletionDialog extends JDialog {
	class FqtCompletionPanel extends JPanel implements KeyListener {
		JList 	list;
		String 	pack;
		String 	cl;
		Buffer 	buffer;
		int 	offset;

		public void keyPressed(KeyEvent e) {
			if (e.getKeyCode() == KeyEvent.VK_ESCAPE) {
				s.getParentDialog(this).setVisible(false);
			} else if (e.getKeyCode() == KeyEvent.VK_ENTER) {
				s.getParentDialog(this).setVisible(false);
				switch (list.getSelectedIndex()) {
				case 0:
					CompletionDialog.addImportAndInsertCompletion(pack+".*;", buffer, offset, cl);
					break;
				case 1:
					CompletionDialog.addImportAndInsertCompletion(pack+"."+cl+";", buffer, offset, cl);
					break;
				case 2:
					s.insertCompletion(buffer, offset, pack+"."+cl);
					break;
				}
			}
		}
		public void keyReleased(KeyEvent e) {e.consume();}
		public void keyTyped(KeyEvent e) {e.consume();}

		FqtCompletionPanel(String pack, String cl, Buffer buffer, int offset) {
			super();
			int y;

			setLayout(new GridBagLayout());

			this.pack = pack;
			this.cl = cl;
			this.buffer = buffer;
			this.offset = offset;

			JTextArea label = new JTextArea("Fully qualified type completion will: ");
			label.setEditable(false);
			label.setBackground(s.light_gray);
			label.setForeground(Color.black);
			if (s.javaVersion.compareTo("1.4.0") >= 0) label.setFocusable(false);

			list = new JList(new String[] {
				" - import "+pack+".* and complete "+cl+" ",
				" - import "+pack+"."+cl+" and complete "+cl+" ",
				" - complete "+pack+"."+cl+" "
			});
			list.addKeyListener(this);
			list.setBackground(s.light_gray);
			list.setForeground(Color.black);
			list.setSelectedIndex(0);
			SwingUtilities.invokeLater(new FocusRequester(list));
				
			y=0;
			s.addGbcComponent(this, 0,y, 1,1, 1000,1000, 
							  GridBagConstraints.BOTH,
							  label);
			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1000,1000, 
							  GridBagConstraints.BOTH,
							  list);
		}
	}

	FqtCompletionDialog(String pack, String cl, Buffer buffer, int offset) {
		super(s.view,"",true);
		Component 	cc;
		enableEvents(AWTEvent.KEY_EVENT_MASK);
		setContentPane(new JScrollPane(new FqtCompletionPanel(pack, cl, buffer, offset)));
		if (s.javaVersion.compareTo("1.4.0") >= 0) setUndecorated(true);
		pack();
		setLocation(s.recommendedLocation(s.getTextArea()));
		s.moveOnScreen(this);
		setVisible(true);
	}
}


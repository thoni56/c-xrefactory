package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.event.*;
import java.awt.*;


public class MoveParameterDialog  extends JDialog {

	public static class MoveParameterPanel extends JPanel implements KeyListener {
		JTextField position;
		JTextField position2;

		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionCancel();
			}
		}
		class ButtonContinue extends JButton implements ActionListener {
			ButtonContinue() {super("Continue"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionContinue();
			}
		}

		void actionContinue() {
			s.getParentDialog(this).setVisible(false);
		}

		void actionCancel() {
			s.getParentDialog(this).setVisible(false);
			position.setText("0");
			position2.setText("0");
		}

		public void keyTyped(KeyEvent e) {}
		public void keyReleased(KeyEvent e) {}
		public void keyPressed(KeyEvent e) {
			int code = e.getKeyCode();
			if (code == KeyEvent.VK_ESCAPE) {
				e.consume();
				actionCancel();
			} else if (code == KeyEvent.VK_ENTER) {
				e.consume();
				actionContinue();
			}
		}

		MoveParameterPanel(String fname) {
			super();
			int y,py;
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue()};
			position = new JTextField("1");
			position.addKeyListener(this);
			position2 = new JTextField("2");
			position2.addKeyListener(this);
			setLayout(new GridBagLayout());

			JPanel mainpanel = new JPanel();
			mainpanel.setLayout(new GridBagLayout());

			py = 0;
			s.addGbcComponent(mainpanel, 0,py, 4,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("  Move parameter of "+fname+"  "));

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 4,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("from position :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  position);
			s.addGbcComponent(mainpanel, 3,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("  on position :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  position2);
			s.addGbcComponent(mainpanel, 3,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y=0;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  mainpanel);
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addButtonLine(this, y, buttons,true);

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
		}
	}

	public String getPosition() {
		return((String)((MoveParameterPanel)getContentPane()).position.getText());
	}

	public String getPosition2() {
		return((String)((MoveParameterPanel)getContentPane()).position2.getText());
	}

	public MoveParameterDialog(String oldname) {
		super(s.view, "Move Parameter");
		setContentPane(new MoveParameterPanel(oldname));
		setModal(true);
		pack();
		setLocationRelativeTo(s.view);
		setVisible(true);
	}
}

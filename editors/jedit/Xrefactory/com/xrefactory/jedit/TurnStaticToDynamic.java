package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

class TurnStaticToDynamicDialog extends JDialog {

	public static class TurnStaticToDynamicPanel extends JPanel implements KeyListener {

		JTextField 	param;
		JTextField 	field;

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
			if (! param.equals("")) {
				XrefStringArray xroption = new XrefStringArray();
				xroption.add("-rfct-static-to-dynamic");
				xroption.add("-rfct-param1="+param.getText());
				xroption.add("-rfct-param2="+field.getText());
				Refactorings.mainRefactorerInvocation(xroption,false);
			}
		}

		void actionCancel() {
			s.getParentDialog(this).setVisible(false);
			param.setText("");
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

		TurnStaticToDynamicPanel(String name) {
			super();
			int y,py;
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue(),
			};

			param = new JTextField("1");
			param.addKeyListener(this);

			field = new JTextField("");
			field.addKeyListener(this);

			setLayout(new GridBagLayout());

			JPanel mainpanel = new JPanel();
			mainpanel.setLayout(new GridBagLayout());

			JComponent label = new JOptionPane(
"Enter position of the parameter used to determine target object and \n (optionaly) the field used to get target object from this parameter. ", 
JOptionPane.INFORMATION_MESSAGE, JOptionPane.DEFAULT_OPTION, null, new String[0]);


			py = 0;
			s.addGbcComponent(mainpanel, 0,py, buttons.length+2,1, 1,1, 
							  GridBagConstraints.NONE,
							  label);

			py ++;
			s.addGbcComponent(mainpanel, 0,py, buttons.length+2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, buttons.length+2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("parameter position :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  param);
			s.addGbcComponent(mainpanel, 3,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, buttons.length+2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("             field :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  field);
			s.addGbcComponent(mainpanel, 3,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, buttons.length+2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;

			y=0;
			s.addGbcComponent(this, 0,y, buttons.length+2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, buttons.length+2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  mainpanel);
			y++;
			s.addGbcComponent(this, 0,y, buttons.length+2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addButtonLine(this, y, buttons,true);

			y ++;
			s.addGbcComponent(this, 0,y, buttons.length+2,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

		}
	}

	public TurnStaticToDynamicDialog(String name) {
		super(s.view);
		TurnStaticToDynamicPanel panel = new TurnStaticToDynamicPanel(name);
		setContentPane(panel);
		pack();
		setLocationRelativeTo(s.view);
		pack();
		setVisible(true);
	}

}

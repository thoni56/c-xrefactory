package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

class TurnDynamicToStaticDialog extends JDialog {

	public static class TurnDynamicToStaticPanel extends JPanel implements KeyListener {

		JTextField 	newParam;

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
			if (! newParam.equals("")) {
				XrefStringArray xroption = new XrefStringArray();
				xroption.add("-rfct-dynamic-to-static");
				xroption.add("-rfct-param1="+newParam.getText());
				Refactorings.mainRefactorerInvocation(xroption,false);
			}
		}

		void actionCancel() {
			s.getParentDialog(this).setVisible(false);
			newParam.setText("");
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

		TurnDynamicToStaticPanel(String name) {
			super();
			int y,py;
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue(),
			};

			String np = s.computeSomeInformationInXref(s.view, "-olcxcurrentclass");
			if (np==null) {
				np="param";
			} else {
				int i, j;
				i = 0;
				while((j=np.indexOf('.', i)) != -1) i = j+1;
				np = s.lowerCaseFirstLetter(np.substring(i));
			}
			newParam = new JTextField(np);
			newParam.addKeyListener(this);

			setLayout(new GridBagLayout());

			JPanel mainpanel = new JPanel();
			mainpanel.setLayout(new GridBagLayout());

			JComponent label = new JOptionPane(
				"Enter the name of the new parameter. \nThis parameter will be added to paramater list \nand will be used to pass member class to the method.", JOptionPane.INFORMATION_MESSAGE, JOptionPane.DEFAULT_OPTION, null, new String[0]);


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
							  new JLabel("  new parameter :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  newParam);
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

	public TurnDynamicToStaticDialog(String name) {
		super(s.view);
		TurnDynamicToStaticPanel panel = new TurnDynamicToStaticPanel(name);
		setContentPane(panel);
		pack();
		setLocationRelativeTo(s.view);
		pack();
		setVisible(true);
	}

}

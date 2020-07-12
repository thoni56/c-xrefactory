package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import org.gjt.sp.jedit.*;

class MoveDialog extends JDialog {

	public static class MovePanel extends JPanel implements KeyListener,FocusListener {

		String		continuationOption;
		String		contTestOption;

		String 		stext=""; // just to verify if is not modified

		String 		sfile;
		int			sline;
		int			scaret;

		JTextField 	ssfile;
		JTextField	ssline;

		JTextField 	tfile;
		JTextField	tline;

		JTextField	tfield;

		class ButtonImport extends JButton implements ActionListener {
			ButtonImport() {super("Import position from jEdit"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionImport();
			}
		}
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
			if (tfile.getText().equals("") || Integer.parseInt(tline.getText()) <= 0) {
				JOptionPane.showMessageDialog(s.view, "Invalid target place. In order to set target position, move \nto the target place, invoke refactoring function and select \n'Set target for next moving refactoring' item.", "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
			}
			// check target position
			DispatchData ndata = new DispatchData(s.xbTask, this);			
			XrefCharBuffer receipt = ndata.xTask.callProcessOnFileSingleOpt(contTestOption, ndata);
			Dispatch.dispatch(receipt, ndata);
			if (! s.panic) {
				s.getParentDialog(this).setVisible(false);
				s.targetFile = tfile.getText();
				s.targetLine = Integer.parseInt(tline.getText());
				s.moveToPosition(s.view, sfile, scaret);
				if (stext.equals(s.getTextArea().getText())) {
					stext = "";	// free memory
					String fieldOpt = null;
					if (tfield!=null) fieldOpt = tfield.getText();
					s.performMovingRefactoring(continuationOption, fieldOpt);
				} else {
					JOptionPane.showMessageDialog(s.view, "Source file was modified during target selection, please retry.", "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
				}
			}
		}

		void actionCancel() {
			s.getParentDialog(this).setVisible(false);
			tfile.setText("");
			tline.setText("0");
		}
		void actionImport() {
			tfile.setText(s.getBuffer().getPath());
			tline.setText(""+(s.getTextArea().getCaretLine()+1));
			//repaint();
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
		public void focusLost(FocusEvent e) {
		}
		public void focusGained(FocusEvent e) {
			//&actionImport();
			s.getParentDialog(this).pack();
			s.getParentDialog(this).repaint();
		}
		MovePanel(String kind, boolean fieldMoving, String continuationOption, String contTestOption) {
			super();
			int y,py;
			JButton buttons[] = {new ButtonCancel(), 
								 //&new ButtonImport(),
								 new ButtonContinue(),
			};

			this.continuationOption = continuationOption;
			this.contTestOption = contTestOption;

			sfile = s.getBuffer().getPath();
			ssfile = new JTextField(sfile);
			ssfile.setEditable(false);
			if (s.javaVersion.compareTo("1.4.0") >= 0) ssfile.setFocusable(false);
			sline = s.getTextArea().getCaretLine()+1;
			ssline = new JTextField(""+sline);
			ssline.setEditable(false);
			if (s.javaVersion.compareTo("1.4.0") >= 0) ssline.setFocusable(false);
			scaret = s.getTextArea().getCaretPosition();
			stext = s.getTextArea().getText();

//&			if (s.targetFile!=null && s.targetLine>0) {
//&				s.moveToPosition(s.view, s.targetFile, s.targetLine, 0);
//&			}
			tfile = new JTextField(s.targetFile);
			tfile.setEditable(false);
			if (s.javaVersion.compareTo("1.4.0") >= 0) tfile.setFocusable(false);
			tline = new JTextField(""+s.targetLine);
			tline.setEditable(false);
			if (s.javaVersion.compareTo("1.4.0") >= 0) tline.setFocusable(false);

			for(int i=0; i<buttons.length; i++) {
				buttons[i].addKeyListener(this);
				buttons[i].addFocusListener(this);
			}
			tfile.addKeyListener(this);
			tfile.addFocusListener(this);
			tline.addKeyListener(this);
			tline.addFocusListener(this);

			if (fieldMoving) {
				tfield = new JTextField("");
				tfield.setEditable(true);
				//&if (s.javaVersion.compareTo("1.4.0") >= 0) tfield.setFocusable(false);
				tfield.addKeyListener(this);
				tfield.addFocusListener(this);
			} else {
				tfield = null;
			}

			setLayout(new GridBagLayout());

			JPanel mainpanel = new JPanel();
			mainpanel.setLayout(new GridBagLayout());

			py = 0;
			s.addGbcComponent(mainpanel, 0,py, 5,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py++;
			s.addGbcComponent(mainpanel, 0,py, 5,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("With current settings, Xrefactory will move definition"));

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 5,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("  from file :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  ssfile);
			s.addGbcComponent(mainpanel, 4,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("   and line :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 2,1, 1000,1, 
							  GridBagConstraints.BOTH,
							  ssline);
			s.addGbcComponent(mainpanel, 4,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			py ++;
			s.addGbcComponent(mainpanel, 0,py, 5,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("    to file :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  tfile);
			s.addGbcComponent(mainpanel, 4,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(mainpanel, 1,py, 1,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("    on line :", SwingConstants.RIGHT));
			s.addGbcComponent(mainpanel, 2,py, 2,1, 1000,1, 
							  GridBagConstraints.BOTH,
							  tline);
			s.addGbcComponent(mainpanel, 4,py, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			py ++;
			s.addGbcComponent(mainpanel, 0,py, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

/*&
			py ++;
			s.addGbcComponent(mainpanel, 0,py, 5,1, 1,1, 
							  GridBagConstraints.NONE,
							  new JLabel("(to change the target, open the target file in jEdit and set caret on target line)"));
&*/

			if (fieldMoving) {
				py ++;
				s.addGbcComponent(mainpanel, 2,py, 1,1, 100,1, 
								  GridBagConstraints.BOTH,
								  new JPanel());

				py ++;
				s.addGbcComponent(mainpanel, 0,py, 1,1, 1,1, 
								  GridBagConstraints.BOTH,
								  new JPanel());
				s.addGbcComponent(mainpanel, 1,py, 2,1, 1000,1, 
								  GridBagConstraints.HORIZONTAL,
								  new JLabel("Field linking target object from source object: ", SwingConstants.RIGHT));
				s.addGbcComponent(mainpanel, 3,py, 1,1, 1000,1, 
								  GridBagConstraints.BOTH,
								  tfield);
				s.addGbcComponent(mainpanel, 4,py, 1,1, 1,1, 
								  GridBagConstraints.BOTH,
								  new JPanel());
			}

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

			y++;
			s.addGbcComponent(this, 0,y, buttons.length+2,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			//&if (s.targetFile.equals("") || s.targetLine==0) actionImport();
		}
	}

	public String getFile() {
		return(((MovePanel)getContentPane()).tfile.getText());
	}

	public String getLine() {
		return(((MovePanel)getContentPane()).tline.getText());
	}

	public MoveDialog(String kind, boolean fieldMoving, String continuationOption, String contTestOption) {
		super(s.view, kind);
		MovePanel panel = new MovePanel(kind, fieldMoving, continuationOption, contTestOption);
		setContentPane(panel);
		pack();
		setLocationRelativeTo(s.view);
		setVisible(true);
		pack();
		repaint();
	}

}

package com.xrefactory.jedit;

import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import org.gjt.sp.jedit.textarea.*;
import org.gjt.sp.jedit.*;


public class ExtractMethodDialog  extends JDialog {

	public static class ExtractMethodPanel extends JPanel implements KeyListener {

		String 		bodyString;
		String 		invocationString;
		JTextField 	name;
		int 		definitionLine;
		int			invocationLine;

		class ButtonCancel extends JButton implements ActionListener {
			ButtonCancel() {super("Cancel"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionCancel();
			}
		}
		class ButtonContinue extends JButton implements ActionListener {
			ButtonContinue() {super("Apply"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
				actionContinue();
			}
		}

		void actionContinue() {
			int sb, sl, sbline;
			JEditTextArea text = s.getTextArea();
			Buffer buff = s.getBuffer();
			s.getParentDialog(this).setVisible(false);
			Selection [] sels = text.getSelection();
			if (sels.length == 0) {
				SwingUtilities.invokeLater(new s.MessageDisplayer("No selection found",true));
				return;
			}
			Selection sel = sels[0];
			if (sel.getStart() < sel.getEnd()) {
				sb = sel.getStart();
				sl = sel.getEnd() - sel.getStart();
			} else {
				sb = sel.getEnd();
				sl = sel.getStart() - sel.getEnd();
			}
			buff.remove(sb, sl);
			buff.insert(sb, invocationString);
			sbline = buff.getLineOfOffset(sb);
			int invlines = s.getNumberOfCharOccurences(invocationString,'\n');
			buff.indentLines(sbline, sbline+invlines-1);
			invocationLine = sbline+1;

			int bodylines = s.getNumberOfCharOccurences(bodyString,'\n');
			buff.insert(buff.getLineStartOffset(definitionLine-1), bodyString);
			invocationLine += bodylines;
			buff.indentLines(definitionLine-1, definitionLine-1+bodylines-1);

		}

		void actionCancel() {
			s.xExtractMethod = null;
			s.getParentDialog(this).setVisible(false);
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

		ExtractMethodPanel(String invocation, String theHead, String theTail, int definitionLine, String xtype) {
			super();
			int y;

			this.definitionLine = definitionLine;
			s.xExtractMethod = this;
			
			JButton buttons[] = {new ButtonCancel(), 
								 new ButtonContinue()};
			setLayout(new GridBagLayout());

			XrefCharBuffer mm = new XrefCharBuffer();
			mm.append("\n");
			mm.append(theHead);
			mm.append("\t//original code starts here\n");
			mm.append(s.getTextArea().getSelectedText());
			if (mm.lastChar()!='\n') mm.append("\n");
			mm.append("\t//original code ends here\n");
			mm.append(theTail);
			JTextArea newMethod = new JTextArea(mm.toString());

			newMethod.setEditable(false);
			newMethod.setBackground(s.light_gray);
			newMethod.setTabSize(4);
			JScrollPane newMethodSc = new JScrollPane(newMethod);
			//& newMethodSc.setMaximumSize(new Dimension(800,600));

			mm.clear();
			mm.append(theHead);
			mm.append(s.getTextArea().getSelectedText());
			if (mm.lastChar()!='\n') mm.append("\n");
			mm.append(theTail);
			bodyString = mm.toString();
			
			mm.clear();
			mm.append("\n");
			mm.append(invocation);
			JTextArea invoked = new JTextArea(mm.toString());
			invoked.setEditable(false);
			invoked.setBackground(s.light_gray);
			invoked.setTabSize(4);

			mm.clear();
			mm.append(invocation);
			invocationString = mm.toString();

			if (xtype.length()>1) {
				name = new JTextField(xtype.substring(0, xtype.length()-1));
			} else {
				name = new JTextField("newMethod");
			}
			name.addKeyListener(this);

			y=0;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel("Xrefactory Proposes Following New Definition:", SwingConstants.CENTER));
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(this, 1,y, 2,1, 100,100, 
							  GridBagConstraints.BOTH,
							  newMethodSc);
			s.addGbcComponent(this, 3,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
/*&
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
							  &*/
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel("with Invocation:", SwingConstants.CENTER));

			y++;
			s.addGbcComponent(this, 0,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			s.addGbcComponent(this, 1,y, 2,1, 100,10, 
							  GridBagConstraints.BOTH,
							  new JScrollPane(invoked));
			s.addGbcComponent(this, 3,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JSeparator());

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

/*&
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel("To apply this extraction enter name for the new method and press Continue.", SwingConstants.CENTER));
&*/

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

			y++;
			s.addGbcComponent(this, 1,y, 1,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JLabel("To be renamed to: ", SwingConstants.LEFT));

			s.addGbcComponent(this, 2,y, 1,1, 1000,1, 
							  GridBagConstraints.HORIZONTAL,
							  name);
			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());
			y++;
			s.addExtraButtonLine(this, 0,y, 4,1, 1,1, buttons, true);

			y++;
			s.addGbcComponent(this, 0,y, 4,1, 1,1, 
							  GridBagConstraints.BOTH,
							  new JPanel());

		}
	}

	public ExtractMethodDialog(String invocation, String theHead, String theTail, int targetLine, String xtype) {
		super(s.view, "Xrefactory Extraction");
		setContentPane(new ExtractMethodPanel(invocation, theHead, theTail, targetLine, xtype));
		setModal(true);
		//&setSize(500,400);
		pack();
		setLocationRelativeTo(s.view);
		setVisible(true);
	}
}

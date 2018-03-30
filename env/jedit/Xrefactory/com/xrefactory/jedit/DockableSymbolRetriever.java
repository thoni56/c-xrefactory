package com.xrefactory.jedit;

import javax.swing.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.gui.*;
import java.awt.*;
import java.awt.event.*;

public class DockableSymbolRetriever extends JPanel {

	static int							currentId = 0;
	
    public JTextField	   				string;
    public JCheckBox    				scanJars;
	public static XrefStringArray		history = new XrefStringArray();
    XrefSelectableLinesTextPanel        text;
  	View								view;
	int									id = 0;
	

	class SearchButtonListener implements ActionListener {
		public void actionPerformed(ActionEvent e) {
			actionContinue();
		}
	}
	class BackButtonListener implements ActionListener {
		public void actionPerformed(ActionEvent e) {
			actionForwardBack(new String[] {"-olcxtagsearchback"});
		}
	}
	class ForwardButtonListener implements ActionListener {
		public void actionPerformed(ActionEvent e) {
			actionForwardBack(new String[] {"-olcxtagsearchforward"});
		}
	}

    public Component getComponent() {return(this);}
    public String getName() {return(s.dockableRetrieverWindowName);}

    public void setResult(XrefCharBuffer text, DispatchData data) {
        this.text.data = data;
        this.text.setText(text.toString());
        this.text.setCaretPosition(0);
        this.text.renewSelection();	
	}
	
    public void setResult(XrefStringArray res, DispatchData data) {
        XrefCharBuffer text = new XrefCharBuffer();
        for(int i=0; i<res.optionsi; i++) {
            if (i!=0) text.append("\n");
            text.append(res.options[i]);
        }
		setResult(text, data);
    }

    void actionContinue() {
        s.setGlobalValues(view,true);
        String[] options;
        String string = (String) this.string.getText();
/*&
		if (! history.getLast().equals(string)) {
			this.string.addItem(string);
			history.add(string);
		}
&*/
        if (! string.equals("")) {
            DispatchData ndata = new DispatchData(s.xbTask, view);
			ndata.viewId = id;
            if (scanJars.isSelected()) {
                options = new String[] {"-olcxtagsearch=" + string};
            } else {
                options = new String[] {"-olcxtagsearch=" + string, "-searchdef"};
            }
            XrefCharBuffer receipt = ndata.xTask.callProcessOnFile(options, ndata);
            Dispatch.dispatch(receipt, ndata);
            if (ndata.symbolList!=null && ! ndata.panic) {
                setResult(ndata.symbolList, ndata);
            }
			text.data = ndata;
        }
    }

    void actionForwardBack(String [] options) {
        s.setGlobalValues(view,true);
		DispatchData ndata = new DispatchData(s.xbTask, view);
		ndata.viewId = id;
		XrefCharBuffer receipt = ndata.xTask.callProcessOnFile(options, ndata);
		Dispatch.dispatch(receipt, ndata);
		if (ndata.symbolList!=null && ! ndata.panic) {
			setResult(ndata.symbolList, ndata);
		}
		text.data = ndata;
    }

	public void resetTextProperties() {
		text.setFont(jEdit.getFontProperty(s.optRetrieverFont, s.defaultFont));
		text.setBackground(jEdit.getColorProperty(s.optRetrieverBgColor, s.light_gray));
		text.setForeground(jEdit.getColorProperty(s.optRetrieverFgColor, Color.black));
	}

    public DockableSymbolRetriever(View view, String position) {
        super();
		this.view = view;
		this.id = currentId ++;
		s.setGlobalValuesNoActiveProject(view);

        int y = -1;
        setLayout(new GridBagLayout());
        text = new XrefSelectableLinesTextPanel("-olcxtaggoto");
		resetTextProperties();
        string = new JTextField();
		string.setEditable(true);
		string.addActionListener(new SearchButtonListener());

        scanJars = new JCheckBox("Scan jars", false);
		JScrollPane textScrollPane = new JScrollPane(text);
		textScrollPane.setPreferredSize(new Dimension(400,300));
		
		RolloverButton search = new RolloverButton(
			GUIUtilities.loadIcon(jEdit.getProperty(s.optRetrieverSearchButton + ".icon"))
			);
		search.setToolTipText(
			jEdit.getProperty(s.optRetrieverSearchButton + ".label")
			);
		search.addActionListener(new SearchButtonListener());
		RolloverButton back = new RolloverButton(
			GUIUtilities.loadIcon(jEdit.getProperty(s.optRetrieverBackButton + ".icon"))
			);
		back.setToolTipText(
			jEdit.getProperty(s.optRetrieverBackButton + ".label")
			);
		back.addActionListener(new BackButtonListener());
		RolloverButton forward = new RolloverButton(
			GUIUtilities.loadIcon(jEdit.getProperty(s.optRetrieverForwardButton + ".icon"))
			);
		forward.setToolTipText(
			jEdit.getProperty(s.optRetrieverForwardButton + ".label")
			);
		forward.addActionListener(new ForwardButtonListener());

        y++;
        s.addGbcComponent(this, 0, y, 10, 1, 1000, 1000, 
                          GridBagConstraints.BOTH, 
                          textScrollPane);
        y++;
        s.addGbcComponent(this, 0, y, 1, 1, 1, 1, 
                          GridBagConstraints.VERTICAL, 
                          new JSeparator(SwingConstants.VERTICAL));
        s.addGbcComponent(this, 1,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          new JLabel("Search expression:"));
        s.addGbcComponent(this, 2,y, 1,1, 1000,1, 
                          GridBagConstraints.HORIZONTAL,
                          string);
        s.addGbcComponent(this, 3,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          new JPanel());
        s.addGbcComponent(this, 4,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          search);
        s.addGbcComponent(this, 5,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          back);
        s.addGbcComponent(this, 6,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          forward);
        s.addGbcComponent(this, 7,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          new JPanel());
        s.addGbcComponent(this, 8,y, 1,1, 1,1, 
                          GridBagConstraints.HORIZONTAL,
                          scanJars);
        s.addGbcComponent(this, 9, y, 1, 1, 1, 1, 
                          GridBagConstraints.VERTICAL, 
                          new JSeparator(SwingConstants.VERTICAL));

        y++;
        s.addGbcComponent(this, 0, y, 10, 1, 1, 1, 
                          GridBagConstraints.HORIZONTAL, 
                          new JSeparator());
                
    }
} 

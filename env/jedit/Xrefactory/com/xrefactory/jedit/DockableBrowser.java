package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.gui.*;
import org.gjt.sp.jedit.msg.*;
import org.gjt.sp.util.*;


public class DockableBrowser extends JPanel {

	DispatchData		data;
	BrowserTopPanel		browser;
	JToolBar 			toolBar;
	JTextField 			browseName;
	static int			idCounter = 0;
	int					id;
	View				view;

	
	public Component getComponent() {return(this);}
	public String getName() {return(s.dockableBrowserWindowName);}

	void actionBrowseName() {
		String name = (String)browseName.getText();
		Buffer buffer = s.getBuffer();
		int caret = s.getCaretPosition();
		int line = buffer.getLineOfOffset(caret);
		int col = caret - buffer.getLineStartOffset(line);
		DispatchData ndata = new DispatchData(data);
		new Push(new String[] {"-olcxpushname="+name, "-olcxlccursor="+(line+1)+":"+col, "-olnodialog"}, ndata);
	}

	class BrowseNameListener implements ActionListener {
		public void actionPerformed(ActionEvent e) {
			s.setGlobalValues(view, true);
			actionBrowseName();
		}
	}

	void addToolBar() {
		String tbpos = jEdit.getProperty(s.optBrowserToolBarPos);
		if (tbpos.equals("Left")) {
			toolBar.setOrientation(JToolBar.VERTICAL);
			add(BorderLayout.WEST, toolBar);
		} else if (tbpos.equals("Right")) {
			toolBar.setOrientation(JToolBar.VERTICAL);
			add(BorderLayout.EAST, toolBar);
		} else if (tbpos.equals("Top")) {
			toolBar.setOrientation(JToolBar.HORIZONTAL);
			add(BorderLayout.NORTH, toolBar);
		} else if (tbpos.equals("Bottom")) {
			toolBar.setOrientation(JToolBar.HORIZONTAL);
			add(BorderLayout.SOUTH, toolBar);
		} else {
			Log.log(Log.ERROR, DockableBrowser.class, "unknown toolbar position "+tbpos);
		}
	}

	void repositionToolBar() {
		remove(toolBar);
		addToolBar();
	}

	void init(DispatchData data, int split) {
		int i,y;

		this.data = data;

		y= -1;

		toolBar = s.loadToolBar("xrefactory.browser.toolbar");
		s.xbrowser = browser = new BrowserTopPanel(data, true, split);

		JPanel browserPanel = new JPanel();
		browserPanel.setLayout(new GridBagLayout());

		browseName = new JTextField();
		browseName.addActionListener(new BrowseNameListener());

		s.addGbcComponent(browserPanel, 0,0, 1,1, 1,1,
						  GridBagConstraints.NONE,
						  new JLabel("Browse name:"));
		s.addGbcComponent(browserPanel, 1,0, 1,1, 1000,1,
						  GridBagConstraints.HORIZONTAL,
						  browseName);
		s.addGbcComponent(browserPanel, 0,1, 2,1, 1,10000,
						  GridBagConstraints.BOTH,
						  browser);

		setLayout(new BorderLayout());
		add(BorderLayout.CENTER, browserPanel);
		addToolBar();

		// this is to make initialisations
		browser.needToUpdate();
	}


	void initNoBrowser() {
		int i,y;

		y= -1;
		JButton buttons[] = {
			new JButton("Close"),
		};

		y++;
		s.addGbcComponent(this, 0,y, 1,1, 1,1, 
						  GridBagConstraints.BOTH,
						  new JOptionPane("Only one browser per view can be opened", JOptionPane.WARNING_MESSAGE, JOptionPane.DEFAULT_OPTION, null, new JPanel[] {new JPanel()}));
		y++;
		s.addGbcComponent(this, 0,y, 1,1, 1,1, 
						  GridBagConstraints.BOTH,
						  new JPanel());
		y++;
		s.addExtraButtonLine(this, 0,y, 1,1, 1,1, buttons, true);
	}

	public DockableBrowser(DispatchData data) {
		super();
		init(data, JSplitPane.HORIZONTAL_SPLIT);
	}

	public DockableBrowser(View view, String position) throws Exception {
		super();
		this.view = view;
		s.setGlobalValuesNoActiveProject(view);

		int split = JSplitPane.HORIZONTAL_SPLIT;
		if (position.equals("left") || position.equals("right")) {
			split = JSplitPane.VERTICAL_SPLIT;
		}

		if (false && s.browserIsDisplayed(view)) {
			initNoBrowser();
		} else {
			this.id = idCounter++;
			DispatchData ndata = new DispatchData(s.xbTask, view);
			ndata.viewId = id;
			init(ndata, split);
		}
	}

}


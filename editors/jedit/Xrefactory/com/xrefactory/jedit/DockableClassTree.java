package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.text.*;
import java.awt.event.*;
import org.gjt.sp.jedit.View;
import org.gjt.sp.jedit.*;
import org.gjt.sp.util.*;

public class DockableClassTree extends JPanel {

	public View			view;
	public XrefCtree	tree;
	public JToolBar		toolBar;

	// those two functions are copy-pasted from browser, should be common!
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

	public void setTree(XrefTreeNode tree) {
		this.tree.setTree(tree);
	}

	public DockableClassTree(View view, String position) {
        super();
		this.view = view;
		s.setGlobalValuesNoActiveProject(view);

		DispatchData ndata = new DispatchData(s.xbTask, view);
		tree = new XrefCtree(ndata, false);
		toolBar = s.loadToolBar("xrefactory.class-tree-viewer.toolbar");

		setLayout(new BorderLayout());
		add(BorderLayout.CENTER, new JScrollPane(tree));
		addToolBar();

	}
}




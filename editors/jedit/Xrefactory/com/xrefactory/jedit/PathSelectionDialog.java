package com.xrefactory.jedit;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.filechooser.*;
import javax.swing.plaf.basic.*;
import java.util.*;
import java.io.*;


class PathSelectionDialog extends JDialog {

	public String resultString;

	static FileBrowser fbrowser = null;

	Vector		paths = new Vector();
	JList 		path = new JList(paths);

	class AddButton extends JButton implements ActionListener {
		AddButton() {super("Add"); addActionListener(this);}
		public void actionPerformed( ActionEvent e) {
			String[] pp = fbrowser.tree.selection(false);
			if (pp!=null) {
				for(int i=0; i<pp.length; i++) {
					if (! paths.contains(pp[i])) {
						paths.add(pp[i]);
					}
				}
				path.setListData(paths);
			}
		}
	}

	class DelButton extends JButton implements ActionListener {
		DelButton () {super("Del"); addActionListener(this);}
		public void actionPerformed( ActionEvent e) {
			Object pp[] = path.getSelectedValues();
			if (pp!=null) {
				for(int i=0; i<pp.length; i++) {
					paths.remove(pp[i]);
				}
				path.setListData(paths);
			}
		}
	}

	class OKButton extends JButton implements ActionListener {
		OKButton () {super("Continue"); addActionListener(this);}
		public void actionPerformed( ActionEvent e) {
			int i;
			int len = paths.size();
			resultString = "";
			for(i=0; i<len; i++) {
				resultString += paths.elementAt(i);
				if (i+1<len) resultString += s.classPathSeparator;
			}	
			PathSelectionDialog.this.setVisible(false);
		}
	}

	class CancelButton extends JButton implements ActionListener {
		CancelButton () {super("Cancel"); addActionListener(this);}
		public void actionPerformed( ActionEvent e) {
			PathSelectionDialog.this.setVisible(false);
		}
	}

	class Stc extends Container {

		Stc() {
			int height;
			setLayout(new GridBagLayout());
			GridBagConstraints gbc = new GridBagConstraints();
			int y = -1;

			y++;
			gbc.gridx = 1; gbc.gridy = y; 
			gbc.gridwidth = 1; gbc.gridheight = 1;
			gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.NONE;
			add(new JPanel(), gbc);
			gbc.gridx = 2; gbc.gridy = y; 
			gbc.gridwidth = 1; gbc.gridheight = 1;
			gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.NONE;
			add(new JLabel("Path:"), gbc);

			y++;
			gbc.gridx = 1; gbc.gridy = y; 
			gbc.gridwidth = 1; gbc.gridheight = 1;
			gbc.weightx = 1; gbc.weighty = 10; gbc.fill = GridBagConstraints.BOTH;
			add(new JPanel(), gbc);
			y++;
			gbc.gridx = 1; gbc.gridy = y; 
			gbc.gridwidth = 1; gbc.gridheight = 1;
			gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
			add(new AddButton(), gbc);

			y++;
			gbc.gridx = 1; gbc.gridy = y; 
			gbc.gridwidth = 1; gbc.gridheight = 1;
			gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
			add(new DelButton(), gbc);

			y++;
			gbc.gridx = 1; gbc.gridy = y; 
			gbc.gridwidth = 1; gbc.gridheight = 1;
			gbc.weightx = 1; gbc.weighty = 10; gbc.fill = GridBagConstraints.BOTH;
			add(new JPanel(), gbc);

			height = 4;
			gbc.gridx = 2; gbc.gridy = y-height+1; 
			gbc.gridwidth = 1; gbc.gridheight = height;
			gbc.weightx = 100; gbc.weighty = 5000; gbc.fill = GridBagConstraints.BOTH;
			add(new JScrollPane(path), gbc);

		}
	}

	PathSelectionDialog(JDialog father, String path) {
		super(father, "Path selection");
		resultString = path;
		getContentPane().setLayout(new GridBagLayout());
		GridBagConstraints gbc = new GridBagConstraints();
		int y = -1;

		if (fbrowser==null) fbrowser = new FileBrowser(s.rootDir);

		Options.PathIterator ii = new Options.PathIterator(path,s.classPathSeparator);
		while (ii.hasNext()) {
			String pp = ii.next();
			paths.add(pp);
			fbrowser.tree.expandPath(pp);
		}
		this.path.setListData(paths);

		JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, fbrowser, new Stc());

		// ---------------------- source
		y++;
		gbc.gridx = 0; gbc.gridy = y; 
		gbc.gridwidth = 4; gbc.gridheight = 1;
		gbc.weightx = 1; gbc.weighty = 100; gbc.fill = GridBagConstraints.BOTH;
		getContentPane().add(splitPane, gbc);
		

		y++;
		gbc.gridx = 0; gbc.gridy = y; 
		gbc.gridwidth = 1; gbc.gridheight = 1;
		gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
		getContentPane().add(new JPanel(), gbc);
		gbc.gridx = 1; gbc.gridy = y; 
		gbc.gridwidth = 1; gbc.gridheight = 1;
		gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
		getContentPane().add(new CancelButton(), gbc);
		gbc.gridx = 2; gbc.gridy = y; 
		gbc.gridwidth = 1; gbc.gridheight = 1;
		gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
		getContentPane().add(new OKButton(), gbc);
   		gbc.gridx = 3; gbc.gridy = y; 
		gbc.gridwidth = 1; gbc.gridheight = 1;
		gbc.weightx = 1; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
		getContentPane().add(new JPanel(), gbc);

		fbrowser.drives.setMinimumSize(new Dimension(200,20));
		setSize(550,450);
		setLocationRelativeTo(father);
		splitPane.setResizeWeight(0.5);   // does not work
		//&splitPane.setDividerLocation(0.5);   // does not work
		setModal(true);
		setVisible(true);
	}

}

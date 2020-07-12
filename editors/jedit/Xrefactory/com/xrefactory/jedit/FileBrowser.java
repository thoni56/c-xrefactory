package com.xrefactory.jedit;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.filechooser.*;
import javax.swing.plaf.basic.*;
import java.util.*;
import java.io.*;

class FileBrowser extends JPanel {

	DirectoryTree tree;
	JComboBox drives;
	

	class DriveComboBoxListener implements ActionListener {
		public void  actionPerformed( ActionEvent e) {
			File drive = (File)drives.getSelectedItem();
			tree.setModel(new DefaultTreeModel(new DirectoryNode(null, drive)));
			tree.repaint();
		}
	}

	FileBrowser(String root) {
		int y, height;
		File[] ff = FileSystemView.getFileSystemView().getRoots();
		File rootdir;
		if (ff.length<=0) {
			JOptionPane.showMessageDialog(s.view, "No file system roots", 
										  "Xrefactory Error", JOptionPane.ERROR_MESSAGE);
			rootdir = new File(root);
		} else {
			rootdir = ff[0];
		}
		tree = new DirectoryTree(new DirectoryNode(null, rootdir));
		setLayout(new GridBagLayout());
		GridBagConstraints gbc = new GridBagConstraints();
		y=-1;

		// ---------------------- output
		y++;
		gbc.gridx = 0; gbc.gridy = y; 
		gbc.gridwidth = 1; gbc.gridheight = 1;
		gbc.weightx = 10000; gbc.weighty = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
		drives = new JComboBox(ff);
		drives.setSelectedItem(rootdir);
		drives.addActionListener(new DriveComboBoxListener());
		add(drives, gbc);

		y++;
		gbc.gridx = 0; gbc.gridy = y; 
		gbc.gridwidth = 1; gbc.gridheight = 1;
		gbc.weightx = 10000; gbc.weighty = 10000; gbc.fill = GridBagConstraints.BOTH;
		add(new JScrollPane(tree), gbc);

	}
}

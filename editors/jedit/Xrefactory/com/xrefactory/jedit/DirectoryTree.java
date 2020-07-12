package com.xrefactory.jedit;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.filechooser.*;
import java.io.*;
import java.util.*;


class DirectoryNode implements TreeNode, Comparable {
    File 				ff;
    DirectoryNode		parent;
    DirectoryNode[] 	subs = null;

    String              name;
    boolean             isDirectory;

    private boolean fileFilter(File ff) {
		if (ff.isDirectory()) return(true);
		String name = ff.getName();
		//& System.out.println("checking " + name);
		int len = name.length();
		if (name.lastIndexOf(".java") == len-5) return(true);
		if (name.lastIndexOf(".JAVA") == len-5) return(true);
		if (name.lastIndexOf(".jav") == len-4) return(true);
		if (name.lastIndexOf(".JAV") == len-4) return(true);
		if (name.lastIndexOf(".jar") == len-4) return(true);
		if (name.lastIndexOf(".JAR") == len-4) return(true);
		if (name.lastIndexOf(".zip") == len-4) return(true);
		if (name.lastIndexOf(".ZIP") == len-4) return(true);
		return(false);
    }

    public int  compareTo( Object o) {
		return(ff.getName().compareTo(((DirectoryNode)o).ff.getName()));
    }

    private void setSubs() {
		int j,count;
		File[] files = FileSystemView.getFileSystemView().getFiles(ff, true);
		count = 0;
		for(int i=0; i<files.length; i++) {
			if (fileFilter(files[i])) count++;
		}
		subs = new DirectoryNode[count];
		j = 0;
		for(int i=0; i<files.length; i++) {
			if (fileFilter(files[i])) {
				subs[j] = new DirectoryNode(this, files[i]);
				j++;
			}
		}
		Arrays.sort(subs);
    }

    public Enumeration children() {
		if (subs==null)  setSubs();
		return(new ArrayEnumerator(subs));
    }
    public TreeNode getChildAt(int i) {
		if (subs==null)  setSubs();
		return(subs[i]);
    }
    public TreeNode getParent( ) {
		return(parent);
    }
    public boolean getAllowsChildren( ) {
		if (subs==null)  setSubs();
		return(subs.length != 0);
    }
    public boolean isLeaf( ) {
		if (subs==null)  setSubs();
		return(subs.length == 0 && ! isDirectory);
    }
    public int getChildCount( ) {
		if (subs==null)  setSubs();
		return(subs.length);
    }
    public int getIndex(TreeNode t) {
		if (subs==null)  setSubs();
		for(int i=0; i<subs.length; i++) {
			if (t == subs[i]) return(i);
		}
		return(-1);
    }

    public String toString() {
		return(name);
    }

    DirectoryNode(DirectoryNode parent, File ff) {
		this.parent = parent;
		this.ff = ff;
		this.isDirectory = ff.isDirectory();
		String str = ff.toString();
		String pth = ff.getAbsolutePath();
		String res;
		if (str.equals(pth)) this.name = ff.getName();
		else this.name = str;
		if (this.name.equals("")) this.name = str;
    }

}
		
class DirectoryTree extends JTree {

	DirectoryNode rootNode;

	String[] selection(boolean reqDirectory) {
		TreePath[] p = getSelectionPaths();
		String[] res = null;
		if (p!=null) {
			int count = 0;
			for (int i=0; i<p.length; i++) {
				File ff = ((DirectoryNode)p[i].getLastPathComponent()).ff;
				if (ff.isDirectory() || !reqDirectory) count++;
			}
			res = new String[count];
			int j=0;
			for (int i=0; i<p.length; i++) {
				File ff = ((DirectoryNode)p[i].getLastPathComponent()).ff;
				if (ff.isDirectory() || !reqDirectory) {
					res[j++] = ff.getAbsolutePath();
				}
			}
		}
		return(res);
	}



	void expandPath(String path) {
/*
		Object[] oo = new Object[1000];
		String cpath = "";
		DirectoryNode nd;
		int ooi,i,n;
		Options.PathIterator ii = new Options.PathIterator(path, s.slash);
		if (ii.hasNext()) {
			String pp = ii.next();
			cpath += pp;
			ooi=0; nd = rootNode;
			oo[0] = nd;
			while (ii.hasNext()) {
				pp = ii.next();
				n = nd.getChildCount();
				for(int i=0; i<n; i++) {
					if (((DirectoryNode)nd.getChildAt(i)).ff.getAbsolutePath().equals(cpath)) {
						
					}
				}
			}
		}
*/
	}

	DirectoryTree(DirectoryNode rootNode) {
		super(rootNode);
		this.rootNode = rootNode;
		putClientProperty("JTree.lineStyle", "Angled");
		//& setPreferredSize(new Dimension(300,500));
	}
}


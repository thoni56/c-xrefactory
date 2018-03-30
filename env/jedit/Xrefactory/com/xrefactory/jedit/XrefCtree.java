package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import javax.swing.tree.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;

class XrefTreeNodeListEnum implements Enumeration {
	ListIterator l;
	public Object  nextElement( ){ return(l.next()); }
	public boolean  hasMoreElements( ) { return(l.hasNext()); }
	XrefTreeNodeListEnum(ListIterator l) {this.l = l;}
}

class XrefTreeNode implements TreeNode {
	String 				className = null;
	int					printedOnLine;
	int 				refNum;
	int 				drefNum;
	boolean				bestFit;
	boolean				selected;
	boolean				isinterface;
	boolean				isTopLevel;
	boolean				hasdefinition;
	boolean				uplink;
	boolean				active;
	LinkedList			subNodes = null;
	XrefTreeNode		parent = null;

	public Enumeration  children() { 
		return(new XrefTreeNodeListEnum(subNodes.listIterator())); 
	}
	public TreeNode  getChildAt(int n) { return((TreeNode)subNodes.get(n));}
	public TreeNode  getParent() {return(parent);}
	public boolean  getAllowsChildren() { return(true);}
	public boolean  isLeaf(){ return(subNodes==null || subNodes.size()==0);}
	public int  getChildCount(){return(subNodes.size());}
	public int  getIndex(TreeNode s){return(subNodes.indexOf(s));}

	void removeSubNode(XrefTreeNode nn) {
		subNodes.remove(nn);
	}

	XrefTreeNode () {
		this("",null,0,0,0,false,false,false,false,false,false);
	}

	XrefTreeNode (String name, XrefTreeNode parent,
				  int printedOnLine, int refNum, int drefNum, 
				  boolean bestFit, boolean xrefSelected, boolean isinterface, 
				  boolean isTopLevel, boolean hasdefinition, boolean uplink) {
		this.className = name;
		this.printedOnLine = printedOnLine;
		this.refNum = refNum;
		this.drefNum = drefNum;
		this.bestFit = bestFit;
		this.selected = xrefSelected;
		this.isinterface = isinterface;
		this.isTopLevel = isTopLevel;
		this.hasdefinition = hasdefinition;
		this.uplink = uplink;
		this.parent = parent;
		this.subNodes = new LinkedList();
		if (parent!=null) parent.subNodes.addLast(this);
	}

}

class XrefTreePath extends TreePath {
	public XrefTreePath(Object []p, int i) {
		super(p,i);
	}
}

class XrefBrowserTreeCellRenderer extends DefaultTreeCellRenderer implements TreeCellRenderer {

	public Component  getTreeCellRendererComponent(JTree tree, Object value, 
												   boolean selected, boolean expanded,
												   boolean leaf, int row, 
												   boolean hasFocus) {
		XrefTreeNode n = (XrefTreeNode) value;
		JComponent res;
		String name;
		name = " " + n.className;
		if (n.bestFit && ! n.isTopLevel) name = n.className + " *";
		if (n.refNum + n.drefNum > 0) {
			if (n.drefNum > 0 && n.refNum > 0) {
				name += "   (" + n.drefNum+"+"+n.refNum + ")";
			} else {
				name += "   (" + (n.refNum+n.drefNum) + ")";
			}
		}
		if (n.isTopLevel) res = new JTextArea("\n" + name);
		else res = new JCheckBox(name, n.selected);
		res.setBorder(s.emptyBorder);
		if (n.hasdefinition && n.isinterface) res.setFont(s.ctItalicBoldFont);
		else if (n.hasdefinition) res.setFont(s.ctBoldFont);
		else if (n.isinterface) res.setFont(s.ctItalicFont);
		else res.setFont(s.ctNormalFont);
		res.setBackground(tree.getBackground());
		if (n.isTopLevel) {
			if (n.className == s.XREF_NON_MEMBER_SYMBOL_NAME) {
				res.setFont(s.ctBoldFont);
				res.setForeground(s.ctNmSymbolColor);
			} else {
				res.setFont(s.ctTopLevelFont);
				res.setForeground(s.ctSymbolColor);
			}
		} else {
			res.setForeground(s.ctFgColor);
		}
		if (selected) {
			if (((XrefCtree)tree).active!=null) ((XrefCtree)tree).active.active = false;
			n.active = true;
			((XrefCtree)tree).active = n;
		}
		if (n.active && ! n.isTopLevel) {
			res.setForeground(s.ctSelectionColor);
		}
		return(res);
	}
	
}

class XrefClassTreeCellRenderer extends DefaultTreeCellRenderer implements TreeCellRenderer {

	public Component  getTreeCellRendererComponent(JTree tree, Object value, 
												   boolean selected, boolean expanded,
												   boolean leaf, int row, 
												   boolean hasFocus) {
		XrefTreeNode 	n = (XrefTreeNode) value;
		JComponent 		res;
		String 			name;
		name = " " + n.className;
		res = new JLabel(name);
		res.setBorder(s.emptyBorder);
		if (n.isinterface) res.setFont(s.ctItalicFont);
		else res.setFont(s.ctNormalFont);
		res.setBackground(s.ctBgColor);
		res.setForeground(s.ctFgColor);
		if (n.active) {
			res.setForeground(s.ctSelectionColor);
		}
		return(res);
	}
}

class XrefTreeMouseAdapter extends MouseInputAdapter {
	XrefCtree tree;
	public void  mousePressed( MouseEvent e) {
		TreePath path = tree.getPathForLocation(e.getX(), e.getY());
		if (path!=null) {
			if (tree.embeddedInBrowser) {
				if (((e.getModifiers() & InputEvent.BUTTON3_MASK) != 0)
					|| ((e.getModifiers() & InputEvent.BUTTON2_MASK) != 0)
					|| ((e.getModifiers() & InputEvent.SHIFT_MASK) != 0)) {
					tree.toggleSelection();
				} else {
					int clickCount = e.getClickCount();
					if (clickCount > 1 || (e.getModifiers() & InputEvent.CTRL_MASK) != 0) {
						tree.callProcessOnTreeLine("-olcxmenuinspectclass");
					} else {
						tree.selectUnselect(tree.tree, 0, false);
						tree.active.selected = true;
						tree.treeDidChange();
						tree.callProcessOnTreeLine("-olcxmenusingleselect");
					}
				}
			} else {
				// class tree tree
				tree.callProcessOnTreeLine("-olcxctinspectdef");
			}
		}
	}
	XrefTreeMouseAdapter(XrefCtree tree) {
		this.tree = tree;
	}
}

class XrefTreeMouseMotionAdapter extends MouseMotionAdapter {
	XrefCtree tree;
	public void  mouseMoved( MouseEvent e) {
		TreePath path = tree.getPathForLocation(e.getX(), e.getY());
		if (path!=null) {
			XrefTreeNode node = (XrefTreeNode) path.getLastPathComponent();
			if (tree.active != node) {
				if (tree.active != null) tree.active.active = false;
				node.active = true;
				tree.active = node;
				tree.setSelectionPath(path);
				tree.treeDidChange();
			}
		}
	}
	XrefTreeMouseMotionAdapter(XrefCtree tree) {
		this.tree = tree;
	}
}

class XrefTreeSelectionListener implements TreeSelectionListener {
	XrefCtree tree;
	XrefTreeNode on, nn;
	public void  valueChanged( TreeSelectionEvent e) {
		if (e.getOldLeadSelectionPath() !=null) {
			on = (XrefTreeNode) e.getOldLeadSelectionPath().getLastPathComponent();
			on.active = false;
		}
		if (e.getNewLeadSelectionPath() !=null) {
			nn = (XrefTreeNode) e.getNewLeadSelectionPath().getLastPathComponent();
			nn.active = true;
			tree.active = nn;
		}
	}
	XrefTreeSelectionListener(XrefCtree tree) {
		this.tree = tree;
	}
}

class XrefTreeKeyAdapter extends KeyAdapter {
	public void  keyTyped( KeyEvent e) {
		XrefCtree tree = (XrefCtree)e.getSource();
		if (tree.embeddedInBrowser) {
			if (e.getKeyChar() == 'a') {
				tree.selectUnselectAll(true);
			} else if (e.getKeyChar() == 'n') {
				tree.selectUnselectAll(false);
			} else if (e.getKeyChar() == ' ' || e.getKeyChar() == '\n') {
				tree.toggleSelection();
			}
		} else {
			// class tree tree
			if (e.getKeyChar() == ' ' || e.getKeyChar() == '\n') {
				tree.callProcessOnTreeLine("-olcxctinspectdef");
			}
		}
	}
}

class XrefTreeModel extends DefaultTreeModel {
	XrefTreeModel(XrefTreeNode tt) {
		super(tt);
	}
}

public class XrefCtree extends JTree {

	private static XrefTreeNode [] aa = new XrefTreeNode[s.XREF_MAX_TREE_DEEP];

	DispatchData 		data;
	public XrefTreeNode tree;
	public XrefTreeNode active;
	boolean				embeddedInBrowser;

	// call it with zero
	private void expandAndSelect(XrefTreeNode node, int index) {
//&System.out.println("index == " + index);
		aa[index] = node;
		XrefTreePath cpath = new XrefTreePath(aa, index+1);
		if (node.selected) addSelectionPath(cpath);
		expandPath(cpath);
		Enumeration ee = node.children();
		if (ee.hasMoreElements()) {
			while (ee.hasMoreElements()) {
				expandAndSelect((XrefTreeNode)ee.nextElement(), index+1);
			}
		}
	}

	public void expandAllAndSelect() {
		expandAndSelect(tree, 0);
	}

	private void expandUntilBestFit(XrefTreeNode node, int index) {
		int i;
		aa[index] = node;
		if (node.bestFit) {
			for(i=1; i<=index+1; i++) {
				XrefTreePath cpath = new XrefTreePath(aa, i);
				expandPath(cpath);
			}
		} else {
			Enumeration ee = node.children();
			while (ee.hasMoreElements()) {
				expandUntilBestFit((XrefTreeNode)ee.nextElement(), index+1);
			}
		}
	}
	public void expandUntilBestFit() {
		expandUntilBestFit(tree, 0);
	}

	public void expandAndSelect() {
		if (embeddedInBrowser) {
			expandAndSelect(tree, 0);
		} else {
			expandUntilBestFit(tree, 0);
		}
	}

	// call it with zero
	public void selectUnselect(XrefTreeNode node, int index, boolean value) {
//&System.out.println("index == " + index);
		aa[index] = node;
		node.selected = value;
		Enumeration ee = node.children();
		while (ee.hasMoreElements()) {
			selectUnselect((XrefTreeNode)ee.nextElement(), index+1, value);
		}
	}

	public void selectUnselectAll(boolean value) {
		String option;
		selectUnselect(tree, 0, value);
		treeDidChange();
		if (value) option = "-olcxmenuall";
		else option = "-olcxmenunone";
		DispatchData ndata = new DispatchData(data, this);
		XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt(option, ndata);
		Dispatch.dispatch(receipt,ndata);		
	}

	void callProcessOnTreeLine(String option) {
		DispatchData ndata = new DispatchData(data, this);
		XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt(option+active.printedOnLine, ndata);
		Dispatch.dispatch(receipt, ndata);		
	}

	public void toggleSelection() {
		active.selected = !active.selected;
		callProcessOnTreeLine("-olcxmenuselect");
		treeDidChange();
	}

	public void setTree(XrefTreeNode tt) {
		super.setModel(new XrefTreeModel(tt));
		tree = tt;
		active = null;
		expandAndSelect();
		repaint();
	}

	XrefCtree(DispatchData data, boolean embeddedInBrowser) {
		this(new XrefTreeNode(), data, embeddedInBrowser);
	}

	XrefCtree(XrefTreeNode tt, DispatchData data, boolean embeddedInBrowser) {
		super();
		s.setupCtreeFontsAndColors();
		setBackground(s.ctBgColor);
		this.data = data;
		this.embeddedInBrowser = embeddedInBrowser;
		setTree(tt);
		putClientProperty("JTree.lineStyle", "Angled");
		TreeSelectionModel sm = new DefaultTreeSelectionModel();
		sm.setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
		setSelectionModel(sm);
		expandAndSelect();

		TreeCellRenderer renderer;
		if (embeddedInBrowser) {
			renderer = new XrefBrowserTreeCellRenderer();
		} else {
			renderer = new XrefClassTreeCellRenderer();
		}
        setCellRenderer(renderer);
		setRootVisible(false);

		addMouseListener(new XrefTreeMouseAdapter(this));
		addMouseMotionListener(new XrefTreeMouseMotionAdapter(this));
		addKeyListener(new XrefTreeKeyAdapter());
		addTreeSelectionListener(new XrefTreeSelectionListener(this));
	}

}


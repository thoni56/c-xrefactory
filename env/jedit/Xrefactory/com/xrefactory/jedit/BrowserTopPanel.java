package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import javax.swing.border.*;
import java.awt.event.*;
import javax.swing.event.*;
import org.gjt.sp.jedit.*;

public class BrowserTopPanel extends JPanel {

	public static final String refViews[] = {"Filter 0", "Filter 1", "Filter 2", "Filter 3"};
	public static final String treeViews[] = {"Equal Name", "Equal Profile", "Relevant"};

	DispatchData			data;
	
	public JSplitPane		splitPanel;
	public TreePanel 		treePanel;
	public ReferencesPanel 	referencesPanel;
	public static Font		browserButtonFont = new Font(null, Font.PLAIN, 10); 

	boolean needToUpdate = false;
	//&boolean saveExcursionDuringUpdate = true;

	class ButtonPop extends JButton implements ActionListener {
		ButtonPop() {
			super("Pop");
			setToolTipText("Remove one symbol from browser stack.");
			addActionListener(this);
		}
		public void actionPerformed( ActionEvent e) {
			s.setGlobalValues(s.getParentView(data.callerComponent),true);
			DispatchData ndata = new DispatchData(data, this);
			XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxpop", ndata);
			Dispatch.dispatch(receipt, ndata);
			if (! ndata.panic) updateData();
		}
	}
	class ButtonRePush extends JButton implements ActionListener {
		ButtonRePush() {
			super("Repush"); 
			setToolTipText("Undo last pop.");
			addActionListener(this);
		}
		public void actionPerformed( ActionEvent e) {
			s.setGlobalValues(s.getParentView(data.callerComponent),true);
			s.displayProjectInformationLater();
			DispatchData ndata = new DispatchData(data, this);
			XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxrepush", ndata);
			Dispatch.dispatch(receipt, ndata);		
			if (! ndata.panic) updateData();
		}
	}
	class ButtonPush extends JButton implements ActionListener {
		ButtonPush() {
			super("Push"); 
			setToolTipText("Resolve symbol pointed by caret and push usages on the stack.");
			addActionListener(this);
		}
		public void actionPerformed( ActionEvent e) {
			s.setGlobalValues(s.getParentView(data.callerComponent),true);
			s.displayProjectInformationLater();
			DispatchData ndata = new DispatchData(data, this);
			new Push(new String[]{"-olcxpush"}, ndata);
		}
	}
/*
  class ButtonEnter extends JButton implements ActionListener {
  ButtonEnter() {super("Enter"); addActionListener(this);}
  public void actionPerformed( ActionEvent e) {
  s.setGlobalValues(s.getParentView(data.callerComponent),true);
  DispatchData ndata = new DispatchData(data, this);				
  JDialog parent = s.getParentDialog(this);
  if (parent!=null) {
  new PushSymbolDialog(parent, ndata);
  } else {
  JFrame parentf = s.getParentFrame(this);
  new PushSymbolDialog(parentf, ndata);
  }
  s.displayProjectInformation();
  }
  }
*/
	public class TreePanel extends JPanel {
		
		DispatchData				data;

		public XrefCtree 			xtree;
		public JComboBox 			treeView;
		public ComboBoxTreeFilter	treeFilter;

		class ButtonAll extends JButton implements ActionListener {
			ButtonAll() {
				super("All"); 
				setToolTipText("Select all symbols on the top.");
				addActionListener(this);
			}
			public void actionPerformed( ActionEvent e) {
				TreePanel.this.xtree.selectUnselectAll(true);
			}
		}

		class ButtonNone extends JButton implements ActionListener {
			ButtonNone() {
				super("None"); 
				setToolTipText("Unselect all symbols on the top.");
				addActionListener(this);
			}
			public void actionPerformed( ActionEvent e) {
				TreePanel.this.xtree.selectUnselectAll(false);
			}
		}

		class ButtonDefault extends JButton implements ActionListener {
			ButtonDefault() {super("Default"); addActionListener(this);}
			public void actionPerformed( ActionEvent e) {
			}
		}

		class ComboBoxTreeFilter extends JComboBox implements ActionListener {
			ComboBoxTreeFilter() {
				super(treeViews);	
				setToolTipText("Restrict displayed symbols by applying filter.");
				addActionListener(this);
			}
			public void actionPerformed( ActionEvent e) {
				Opt.browserTreeFilter = treeFilter.getSelectedIndex();
				DispatchData ndata = new DispatchData(data, TreePanel.this);
				XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxmenufilter="+Opt.browserTreeFilter, ndata);
				Dispatch.dispatch(receipt, ndata);
				referencesPanel.refFilter.setSelectedIndex(Opt.browserRefListFilter);
			}			
		}

		TreePanel(DispatchData data, boolean allButtons) {
			int i,y;
			this.data = data;
			xtree = new XrefCtree(data, true);
			setLayout(new GridBagLayout());
			treeFilter = new ComboBoxTreeFilter();
/* // old version had buttons in tree panel
   JComponent buttons[];
   if (allButtons) {
   buttons = new JComponent[] {
   treeFilter,
   new ButtonAll(), 
   new ButtonNone(), 
   //&new ButtonDefault(),
   };
   } else {
   buttons = new JComponent[] {
   new ButtonAll(), 
   new ButtonNone(), 
   //&new ButtonDefault(),
   };
   }
   y = -1;
   y++;
   s.addGbcComponent(this, 0,y, buttons.length,1, 100,100, 
   GridBagConstraints.BOTH,
   new JScrollPane(xtree));
   y++;
   s.addButtonLine(this, y, buttons, false);
*/
			y = -1;
			y++;
			s.addGbcComponent(this, 0,y, 1, 1, 100,100, 
							  GridBagConstraints.BOTH,
							  new JScrollPane(xtree));
		}
	}

	public class ReferencesPanel extends JPanel {

		DispatchData		data;

		XrefRefListArea 	reflist;
		ComboBoxRefsFilter  refFilter;

		class ComboBoxRefsFilter extends JComboBox implements ActionListener {
			ComboBoxRefsFilter() {
				super(refViews); 
				setToolTipText("Restrict displayed usages by applying filter.");
				addActionListener(this);
			}
			public void actionPerformed( ActionEvent e) {
				Opt.browserRefListFilter = refFilter.getSelectedIndex();
				ReferencesPanel.this.update();
			}			
		}

		class ButtonPrevious extends JButton implements ActionListener {
			ButtonPrevious() {
				super("Previous"); 
				setToolTipText("Move to the previous usage.");
				addActionListener(this);
			}
			public void actionPerformed( ActionEvent e) {
				reflist.previous();
			}
		}
		class ButtonNext extends JButton implements ActionListener {
			ButtonNext() {
				super("Next"); 
				setToolTipText("Move to the next usage.");
				addActionListener(this);
			}
			public void actionPerformed( ActionEvent e) {
				reflist.next();
			}
		}

		public void updateSelection() {
			DispatchData ndata = new DispatchData(data, this);
			XrefCharBuffer receipt = ndata.xTask.callProcessSingleOpt("-olcxgetcurrentrefn", ndata);
			Dispatch.dispatch(receipt, ndata);
		}

		public void update() {
			DispatchData ndata = new DispatchData(data, this);
			XrefStringArray options = new XrefStringArray();
			options.add("-olcxfilter=" + Opt.browserRefListFilter);
			// need all files to have references up to date
			ndata.xTask.addFileProcessingOptions(options);
			XrefTask.addCommonOptions(options, ndata);
			XrefCharBuffer receipt = ndata.xTask.callProcess(options, ndata);
			Dispatch.dispatch(receipt, ndata);		
		}

		ReferencesPanel(DispatchData data, boolean allButtons) {
			int i,y;

			this.data = data;
			reflist = new XrefRefListArea(data);
			reflist.setFont(jEdit.getFontProperty(s.optBrowserFont, s.browserDefaultFont));
			reflist.setBackground(jEdit.getColorProperty(s.optBrowserBgColor, Color.white));
			reflist.setForeground(jEdit.getColorProperty(s.optBrowserFgColor, Color.black));
			refFilter = new ComboBoxRefsFilter();
			JScrollPane xrefs = new JScrollPane(reflist);
			setLayout(new GridBagLayout());
			y = -1;

/* // old version
   JComponent buttons[];
   if (allButtons) {
   buttons = new JComponent[] {
   new ButtonPush(),
   new ButtonPop(),
   new ButtonRePush(),
   new ButtonPrevious(), 
   new ButtonNext(), 
   refFilter,
   };
   } else {
   buttons = new JComponent[] {
   new ButtonPrevious(), 
   new ButtonNext(), 
   };
   }
   y++;
   s.addGbcComponent(this, 0,y, buttons.length,1, 100,100, 
   GridBagConstraints.BOTH,
   xrefs);
   y++;
   s.addButtonLine(this, y, buttons, false);
*/
			y++;
			s.addGbcComponent(this, 0,y,1,1, 100,100, 
							  GridBagConstraints.BOTH,
							  xrefs);
		}
	}

	public void updateData() {
		// save excursion
		s.Position pos = s.getPosition(s.getParentView(this));
		treePanel.treeFilter.setSelectedIndex(Opt.browserTreeFilter);
		// updating reference list is useless, it is updated
		// with update of tree
		//&referencesPanel.refFilter.setSelectedIndex(Opt.browserRefListFilter);
		needToUpdate = false;
		s.moveToPosition(pos);
	}

	public void needToUpdate() {
		if (s.browserIsDisplayed(this)) {
			updateData();
		} else {
			needToUpdate = true;
		}
	}

	public void paint(Graphics g) {
		if (needToUpdate) needToUpdate();
		super.paint(g);
	}

	BrowserTopPanel(DispatchData data, boolean allButtons, int split) {
		int y;
		setLayout(new GridBagLayout());
		splitPanel = new JSplitPane(split, false);
		this.data = data;
		treePanel = new TreePanel(data, allButtons);
		splitPanel.setLeftComponent(treePanel);
		referencesPanel = new ReferencesPanel(data, allButtons);
		splitPanel.setRightComponent(referencesPanel);

		JComponent buttons[] = {};
		/*&
		if (allButtons) {
			buttons = new JComponent[] {
				treePanel.treeFilter,
				treePanel.new ButtonAll(), 
				treePanel.new ButtonNone(), 
				//&new ButtonDefault(),
				new ButtonPush(),
				new ButtonPop(),
				new ButtonRePush(),
				referencesPanel.new ButtonPrevious(), 
				referencesPanel.new ButtonNext(), 
				referencesPanel.refFilter,
			};
		} else {
			buttons = new JComponent[] {
				treePanel.new ButtonAll(), 
				treePanel.new ButtonNone(), 
				//&new ButtonDefault(),
				referencesPanel.new ButtonPrevious(), 
				referencesPanel.new ButtonNext(), 
			};
		}
		s.buttonsSetFont(buttons, browserButtonFont);
		&*/

		y = -1;

		y++;
		s.addGbcComponent(this, 0,y, buttons.length+2, 1, 100,100, 
						  GridBagConstraints.BOTH,
						  splitPanel);

		/*&
		y++;
		s.addButtonLine(this, y, buttons, true);
		&*/

		// TODO! do this seriously
		int tdp = Opt.browserTreeDividerPosition();
		treePanel.setPreferredSize(new Dimension(tdp,200));
		int rdp = 500 - tdp;
		if (rdp < 100) rdp = 100;
		referencesPanel.setPreferredSize(new Dimension(rdp,200));

	}

}


/*
  This file is hacked HelpViewer originally written by Slava Pestov,
  all credits for this code belong to him.

  This file will be removed when the InfoViewer will become more
  appropriate.
*/

package com.xrefactory.jedit;

import javax.swing.*;
import org.gjt.sp.jedit.*;
import org.gjt.sp.jedit.gui.*;
import java.awt.*;
import java.net.*;
import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.text.html.*;
import org.gjt.sp.util.*;
import java.io.*;
import javax.swing.border.*;


public class DockableUrlBrowser extends JPanel {

	View			view;
	JButton			back;
	JButton			forward;
	JEditorPane 	viewer;
	JTextField 		urlField;
	String[] 		history;
	int 			historyPos;
	String 			baseURL;


	class ActionHandler implements ActionListener
	{
		public void actionPerformed(ActionEvent evt)
		{
			Object source = evt.getSource();
			if(source == back)
			{
				if(historyPos <= 1)
					getToolkit().beep();
				else
				{
					String url = history[--historyPos - 1];
					gotoURL(url,false);
				}
				
			}
			else if(source == forward)
			{
				if(history.length - historyPos <= 1)
					getToolkit().beep();
				else
				{
					String url = history[historyPos];
					if(url == null)
						getToolkit().beep();
					else
					{
						historyPos++;
						gotoURL(url,false);
					}
				}
			}
		}
	}

	class LinkHandler implements HyperlinkListener
	{
		public void hyperlinkUpdate(HyperlinkEvent evt)
		{
			if(evt.getEventType() == HyperlinkEvent.EventType.ACTIVATED)
			{
				if(evt instanceof HTMLFrameHyperlinkEvent)
				{
					((HTMLDocument)viewer.getDocument())
						.processHTMLFrameHyperlinkEvent(
						(HTMLFrameHyperlinkEvent)evt);
				}
				else
				{
					URL url = evt.getURL();
					if(url != null)
						gotoURL(url.toString(),true);
				}
			}
			else if (evt.getEventType() == HyperlinkEvent.EventType.ENTERED) {
				viewer.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
			}
			else if (evt.getEventType() == HyperlinkEvent.EventType.EXITED) {
				viewer.setCursor(Cursor.getDefaultCursor());
			}
		}
	}

	class KeyHandler extends KeyAdapter
	{
		public void keyPressed(KeyEvent evt)
		{
			if(evt.getKeyCode() == KeyEvent.VK_ENTER)
			{
				gotoURL(urlField.getText(),true);
			}
		}
	}

	public void gotoURL(String url, boolean addToHistory)
	{
		// reset default cursor so that the hand cursor doesn't
		// stick around
		viewer.setCursor(Cursor.getDefaultCursor());

		URL _url = null;
		try
		{
			_url = new URL(url);

			urlField.setText(_url.toString());
			viewer.setPage(_url);

			//&viewer.scrollToReference(_url.getRef());
			// does not work anyway
			//&viewer.setFont(new Font("Monospaced",Font.ITALIC,9));
			if(addToHistory)
			{
				history[historyPos] = url;
				if(historyPos + 1 == history.length)
				{
					System.arraycopy(history,1,history,
						0,history.length - 1);
					history[historyPos] = null;
				}
				else
					historyPos++;
			}
		}
		catch(MalformedURLException mf)
		{
			Log.log(Log.ERROR,this,mf);
			String[] args = { url, mf.getMessage() };
			GUIUtilities.error(this,"badurl",args);
			return;
		}
		catch(IOException io)
		{
			Log.log(Log.ERROR,this,io);
			String[] args = { url, io.toString() };
			GUIUtilities.error(this,"read-error",args);
			return;
		}

	}	

	public DockableUrlBrowser(View view, String position) {
        super();
		this.view = view;

		try
		{
			baseURL = new File(MiscUtilities.constructPath(
				jEdit.getJEditHome(),"doc")).toURL().toString();
		}
		catch(MalformedURLException mu)
		{
			Log.log(Log.ERROR,this,mu);
			// what to do?
		}

		history = new String[25];

		ActionHandler actionListener = new ActionHandler();

		JToolBar toolBar = new JToolBar(JToolBar.VERTICAL);
		toolBar.setFloatable(false);

		JLabel label = new JLabel(jEdit.getProperty("helpviewer.url"));
		label.setBorder(new EmptyBorder(0,12,0,12));
		//&toolBar.add(label);
		Box box = new Box(BoxLayout.Y_AXIS);
		box.add(Box.createGlue());
		urlField = new JTextField();
		urlField.addKeyListener(new KeyHandler());
		Dimension dim = urlField.getPreferredSize();
		dim.width = Integer.MAX_VALUE;
		urlField.setMaximumSize(dim);
		box.add(urlField);
		box.add(Box.createGlue());
		//&toolBar.add(box);

		//&toolBar.add(Box.createHorizontalStrut(6));

		JPanel buttons = new JPanel();
		buttons.setLayout(new BoxLayout(buttons,BoxLayout.Y_AXIS));
		buttons.setBorder(new EmptyBorder(0,12,0,0));
		back = new RolloverButton(GUIUtilities.loadIcon(
			jEdit.getProperty("helpviewer.back.icon")));
		back.setToolTipText(jEdit.getProperty("helpviewer.back.label"));
		back.addActionListener(actionListener);
		toolBar.add(back);
		forward = new RolloverButton(GUIUtilities.loadIcon(
			jEdit.getProperty("helpviewer.forward.icon")));
		forward.addActionListener(actionListener);
		forward.setToolTipText(jEdit.getProperty("helpviewer.forward.label"));
		toolBar.add(forward);
		toolBar.add(Box.createGlue());
		back.setPreferredSize(forward.getPreferredSize());


		if (s.javaVersion.compareTo("1.4.0") >= 0) {
			viewer = new JEditorPane("text/html","");
			StyleSheet styles = ((HTMLEditorKit)viewer.getEditorKit()).getStyleSheet();
			// hack to remove its default font size
			styles.removeStyle("body");
			// does not work
			//styles.setBaseFontSize("-2");
		} else {
			// with jdk1.3, it does not work with parameters, simply: Magic
			viewer = new JEditorPane();
		}
		viewer.setEditable(false);
		viewer.addHyperlinkListener(new LinkHandler());

		setLayout(new GridBagLayout());
		s.addGbcComponent(this, 0,0, 1,1, 1000,1000, 
						  GridBagConstraints.BOTH,
						  new JScrollPane(viewer));
		s.addGbcComponent(this, 1,0, 1,1, 1,1, 
						  GridBagConstraints.BOTH,
						  toolBar);
	}

}

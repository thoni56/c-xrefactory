package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;

public class ProgressBar extends JPanel
{
	private              int Count;
	private              int Max;
	private static final int FrameBottom = 24;

	public ProgressBar (int TotalItems) {
		Count = 0;
		Max   = TotalItems;
		setLayout(null);
		addNotify();
		setPreferredSize(new Dimension(400, 10));
		setMinimumSize(new Dimension(100, 10));
	}

	public synchronized void show() {
		super.show();
	}

	public void updateProgress(int downloaded) {
		Count += downloaded;
		//if (Count <= Max) paint(getGraphics());
	}

	public void paint(Graphics g) {
		Dimension FrameDimension  = getSize();
		double    PercentComplete = (double)Count * 100.0 /(double)Max;
		int       BarPixelWidth   = ((FrameDimension.width-2) * Count)/ Max;

		// Fill the bar the appropriate percent full.
		g.setColor(Color.red);
		g.fillRect(1, 1, BarPixelWidth, FrameDimension.height-2);
		g.setColor(Color.black);
		g.drawRect(0, 0, FrameDimension.width-1, FrameDimension.height-1);

		//g.setColor (Color.black);
		//FontMetrics fm = g.getFontMetrics(g.getFont());
		//int StringPixelWidth = fm.stringWidth(s);

		//g.drawString(s, (FrameDimension.width - StringPixelWidth)/2, FrameBottom+10);
		//super.paint(g);
	}

}



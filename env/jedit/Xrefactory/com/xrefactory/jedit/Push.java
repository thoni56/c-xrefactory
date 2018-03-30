package com.xrefactory.jedit;

import java.awt.*;
import javax.swing.*;
import java.io.*;
import org.gjt.sp.jedit.io.*;

public class Push implements Runnable {

	String[] 		options;
	DispatchData	data;
	XrefCharBuffer receipt;

	public void run() {
		if (Opt.updateBeforePush()) {
			boolean panic = s.synchronizedUpdateTagFile(data.callerComponent);
			if (panic) return;
		}
		receipt = data.xTask.callProcessOnFile(options, data);
		s.beforePushBrowserFiltersUpdates();
		Dispatch.dispatch(receipt, data);
		if (! data.panic) {
			BrowserTopPanel bp;
			DockableBrowser b = s.getParentBrowserPanel(data.callerComponent);
			if (b!=null) {
				b.browser.updateData();
			} else {
				bp = s.getBrowser(s.getParentView(data.callerComponent));
				if (bp!=null) bp.needToUpdate();
			}
		}
	}

	public Push(String[] options, DispatchData data) {
		this.options = options;
		this.data = data;
		//&Thread t = new Thread(this);
		this.run(); //t.start();
	}
}



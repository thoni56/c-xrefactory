package com.xrefactory.jedit;

import java.awt.*;
import org.gjt.sp.jedit.*;

public class DispatchData {
	public XrefTask 				xTask = null;
	public Component				callerComponent = null;
	public boolean					panic = false;
	public XrefCharBuffer			symbolList = null;
	public String					progressMessage = "Please wait.";
	public int						viewId = 0;
	public Runnable					continuation;
	public String					info = null;
	public boolean 					projectCreationAllowed = true;
	// When adding field, add it to copy constructor too !!!! 
	
	public DispatchData(DispatchData data) {
		this.xTask = data.xTask;
		this.callerComponent = data.callerComponent;
		this.panic = data.panic;
		this.symbolList = data.symbolList;
		this.progressMessage = data.progressMessage;
		this.viewId = data.viewId;
		this.continuation = data.continuation;
		this.info = data.info;
		this.projectCreationAllowed = data.projectCreationAllowed;
	}
	public void setViewId(Component callerComponent) {
		View v = s.getParentView(callerComponent);
		com.xrefactory.jedit.DockableBrowser db = (com.xrefactory.jedit.DockableBrowser) v.getDockableWindowManager().getDockable(s.dockableBrowserWindowName);
		if (db != null) this.viewId = db.data.viewId;			
	}
	public DispatchData(Component callerComponent) {
		this.callerComponent = callerComponent;
		setViewId(callerComponent);
	}
	public DispatchData(XrefTask xTask, Component callerComponent) {
		this.xTask = xTask;
		this.callerComponent = callerComponent;
		setViewId(callerComponent);
	}
	public DispatchData(DispatchData data, Component callerComponent) {
		this(data);
		this.callerComponent = callerComponent;
		//&setViewId(callerComponent);
	}
	public DispatchData(DispatchData data, XrefTask task) {
		this(data);
		this.xTask = task;
	}
}


package com.xrefactory.jedit;

import java.awt.*;
import java.awt.event.*;
import sun.awt.*;
import javax.swing.*;

public class AWTPump  {

	static ComponentEvent sentinel = new PaintEvent(new JPanel(), PaintEvent.UPDATE, new Rectangle(1,1));

    static void dispatchEvent(AWTEvent event) {
        Object src = event.getSource();
        if (event instanceof ActiveEvent) {
            //& setCurrentEventAndMostRecentTimeImpl(event);
            ((ActiveEvent)event).dispatch();
        } else if (src instanceof Component) {
            ((Component)src).dispatchEvent(event);
        } else if (src instanceof MenuComponent) {
            ((MenuComponent)src).dispatchEvent(event);
			//&     } else if (src instanceof AWTAutoShutdown) {
			//&        if (noEvents()) {
			//&           dispatchThread.stopDispatching();
			//&        }
        } else {
            System.err.println("unable to dispatch event: " + event);
        }
    }

    static void pumpEventsForHierarchy(Component modalComponent) {
		
		if (! EventQueue.isDispatchThread()) return;
		
		EventQueue theQueue = Toolkit.getDefaultToolkit().getSystemEventQueue();
		theQueue.postEvent(sentinel);
        try {
            AWTEvent event;
            boolean eventOK;
	
			event = null;
			while (event != sentinel) {
				do {
					event = theQueue.getNextEvent();
					if (event == sentinel) return;
					eventOK = true;
					if (modalComponent != null) {
						/*
						 * filter out MouseEvent and ActionEvent that's outside
						 * the modalComponent hierarchy.
						 * KeyEvent is handled by using enqueueKeyEvent
						 * in Dialog.show
						 */
						int eventID = event.getID();
						if ((eventID >= MouseEvent.MOUSE_FIRST &&
							 eventID <= MouseEvent.MOUSE_LAST)      ||
							(eventID >= ActionEvent.ACTION_FIRST &&
							 eventID <= ActionEvent.ACTION_LAST)) {
							Object o = event.getSource();
							if (o instanceof Component) {
								Component c = (Component) o;
								if (modalComponent instanceof Container) {
									while (c != modalComponent && c != null) {
										c = c.getParent();
									}
								}
								if (c != modalComponent) {
									eventOK = false;
								}
							}
						}
					}
				} while (eventOK == false);
				dispatchEvent(event);
			}
        } catch (Exception death) {
        }
    }

} 

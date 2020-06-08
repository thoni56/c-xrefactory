package se.alanif.jregr.gui;

import java.awt.Component;
import java.awt.Container;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import junit.framework.TestCase;

import abbot.finder.ComponentFinder;
import abbot.finder.ComponentNotFoundException;
import abbot.finder.Matcher;
import abbot.finder.MultipleComponentsFoundException;
import abbot.tester.JButtonTester;
import abbot.tester.JComboBoxTester;
import abbot.tester.JTextComponentTester;
import abbot.tester.JTextFieldTester;

public class GuiTestUtilities {

	private Container container;
	private ComponentFinder finder;

	public GuiTestUtilities(ComponentFinder finder, Container container) {
		this.finder = finder;
		this.container = container;
	}
	
	public JOptionPane findDialog(final String partialMessage) throws ComponentNotFoundException, MultipleComponentsFoundException {
		return (JOptionPane) finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JOptionPane && ((JOptionPane)c).getMessage().toString().indexOf(partialMessage)>=0;
			}
		});
	}

	public JButton findButton(final String string) throws Exception {
		return (JButton) finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JButton && ((JButton) c).getText() == string;
			}
		});
	}

	public JComboBox findComboBox(final String name) throws Exception {
		return (JComboBox)finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JComboBox && ((JComboBox)c).getName().equals(name);
			}
		});
	}

	public JTextArea findTextArea(final String fieldName) throws Exception {
		return (JTextArea)finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JTextArea && ((JTextArea)c).getName().equals(fieldName);
			}
		});
	}

	public JTextField findTextField(final String fieldName) throws Exception {
		return (JTextField)finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JTextField && ((JTextField)c).getName().equals(fieldName);
			}
		});
	}

	public JLabel findLabel(final String label) throws Exception {
		return (JLabel)finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JLabel && ((JLabel)c).getText().equals(label);
			}
		});
	}

	public void findAnyChildDialog() throws ComponentNotFoundException, MultipleComponentsFoundException {
		finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JOptionPane;
			}
		});
	}
	
	public JTable findJTable(final String name) throws ComponentNotFoundException, MultipleComponentsFoundException {
		return (JTable)finder.find(container, new Matcher() {
		    public boolean matches(Component c) {
		        return c instanceof JTable && ((JTable)c).getName() == name;
		    }
		});
	}

	public JPanel findJPanel(final String name) throws ComponentNotFoundException, MultipleComponentsFoundException {
		return (JPanel)finder.find(container, new Matcher() {
			public boolean matches(Component c) {
				return c instanceof JPanel && ((JPanel)c).getName() == name;
			}
		});
	}

	public JList findJList(final String name) throws ComponentNotFoundException, MultipleComponentsFoundException {
		return (JList)finder.find(container, new Matcher() {
		    public boolean matches(Component c) {
		        return c instanceof JList && ((JList)c).getName() == name;
		    }
		});
	}

	public void assertTextArea(String name, String text) throws Exception {
		TestCase.assertNotNull(findLabel(name));
		JTextArea notesTextArea = findTextArea(name);
		TestCase.assertNotNull(notesTextArea);
		if (text != null)
			TestCase.assertEquals(text, notesTextArea.getText());
	}

	public void assertTextField(String name, String content) throws Exception {
		TestCase.assertNotNull(findLabel(name));
		JTextField typeField = findTextField(name);
		TestCase.assertNotNull(typeField);
		if (content != null)
			TestCase.assertEquals(content, typeField.getText());
	}

	public void assertButton(String label) throws Exception {
		TestCase.assertNotNull(findButton(label));
	}

	public void updateTextField(String name, String string) throws Exception {
		JTextField textField = findTextField(name);
		JTextFieldTester tester = new JTextFieldTester();
		tester.actionFocus(textField);
		tester.actionEnterText(textField, string);
		tester.actionFocus(container);
	}

	public void clickButton(String label) throws Exception {
		new JButtonTester().actionClick(findButton(label));
	}

	public void updateTextArea(String name, String text) throws Exception {
		JTextArea textArea = findTextArea(name);
		JTextComponentTester tester = new JTextComponentTester();
		tester.actionFocus(textArea);
		tester.actionEnterText(textArea, text);
	}

	public void updateComboBox(String name, String selector) throws Exception {
		new JComboBoxTester().actionSelectItem(findComboBox(name), selector);
	}

}


package se.alanif.jregr.io;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class File extends java.io.File {

	private static final long serialVersionUID = 1L;

	public File(String pathname) {
		super(pathname);
	}

	public String getContent() {
		String content = "";

		try {
			BufferedReader input = new BufferedReader(new FileReader(this));
			try {
				int ch;
				while ((ch = input.read()) != -1) {
					if (ch != '\r')
						content += String.valueOf((char)ch);
				}
			} finally {
				input.close();
			}
		} catch (IOException ex) {
			ex.printStackTrace();
		}

		return content;
	}

}

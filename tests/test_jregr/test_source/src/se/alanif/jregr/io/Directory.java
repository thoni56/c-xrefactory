package se.alanif.jregr.io;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;

import se.alanif.jregr.CommandsDecoder;
import se.alanif.jregr.io.File;

public class Directory extends File {

	public Directory(String pathname) {
		super(pathname);
	}

	private static final long serialVersionUID = 1L;

	public boolean executableExist(String name) {
		return hasFile(name) || hasFile(name+".exe");
	}

	public boolean hasFile(String fileName) {
		return getFile(fileName).exists();
	}

	public File getFile(String fileName) {
		return new File(getAbsolutePath()+separator+fileName);
	}

	public boolean executablesExist(CommandsDecoder decoder) {
		decoder.reset();
		do {
			if (hasFile(decoder.getCommand()) || hasFile(decoder.getCommand()+".exe"))
				return true;
		} while (decoder.advance());
		return false;
	}

	public BufferedReader getBufferedReaderForFile(File file) {
		if (file.exists())
			try {
				return new BufferedReader(new FileReader(file));
			} catch (FileNotFoundException e) {
				return null;
			}
		else
			return null;
	}

}

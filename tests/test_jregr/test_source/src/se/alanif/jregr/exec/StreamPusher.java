package se.alanif.jregr.exec;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.OutputStream;
import java.io.PrintStream;

public class StreamPusher extends Thread {

	OutputStream stream;
	FileReader reader;
	
	public StreamPusher(OutputStream outputStream, FileReader inputFileReader) {
		this.stream = outputStream;
		this.reader = inputFileReader;
	}

	public StreamPusher(OutputStream outputStream, String string) {
		this.stream = outputStream;
	}

	public void run() {
		BufferedReader br = new BufferedReader(reader);
		PrintStream out = new PrintStream(stream);
		try {
			String line;
			while ((line = br.readLine()) != null)
				out.println(line);
			out.close();
		} catch (Exception ioe) {
			ioe.printStackTrace();
			System.exit(-1);
		}
	}

}

package se.alanif.jregr.exec;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

public class StreamGobbler extends Thread {

	private InputStream inputStream;
	private StringBuilder output = new StringBuilder();

	public StreamGobbler(InputStream inputStream) {
		this.inputStream = inputStream;
	}

	public void run() {
		try {
			InputStreamReader isr = new InputStreamReader(inputStream);
			BufferedReader br = new BufferedReader(isr);
			String line = null;
			while ( (line = br.readLine()) != null) {
				output.append(line).append("\n");
			}
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(-1);
		}
	}

	public String output() {
		return output.toString();
	}

}

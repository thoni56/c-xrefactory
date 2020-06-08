package se.alanif.jregr.exec;

public class CaseRunner {

	public String run(Process p, StreamGobbler errorGobbler, StreamGobbler outputGobbler, StreamPusher inputPusher) {
		errorGobbler.start();
		outputGobbler.start();

		if (inputPusher != null) {
			inputPusher.start();
		}

		try {
			p.waitFor();
			errorGobbler.join();
			outputGobbler.join();
		} catch (InterruptedException e) {
		}
		return outputGobbler.output() + errorGobbler.output();
	}

}

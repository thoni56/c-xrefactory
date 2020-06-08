package se.alanif.jregr;

import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;

public class OptionsManager extends Options {

	private static final long serialVersionUID = 1L;

	public OptionsManager() {
		super();
		addOption("help", false, "this help");
		addOption("gui", false, "run using GUI");
		addOption("xml", false, "output XML according to ANT test format (junit et al.) instead of plain text");
		addOption("noansi", false, "don't use ANSI control on the console to minimize output");
		addOption("nocolor", false, "don't use ANSI control on the console to color output");
		addOption("nocolour", false, "don't use ANSI control on the console to colour output");
		addOption("version", false, "show version");
		addOption(Option.builder("bin").longOpt("bin")
				.desc("find binaries (according to the .jregr file) in directory BINDIR").hasArg().argName("BINDIR")
				.build());
		addOption(Option.builder("dir").longOpt("dir").desc("directory of test cases to run").hasArg()
				.argName("REGRDIR").build());
	}

}

package se.alanif.jregr;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.Arrays;

import se.alanif.jregr.io.Directory;

public class CommandsDecoder {

    // The RegrDecoder decodes a file (presumably a .jregr file) into
    // <extension> ':' <command> {<arg>} [ '<' <stdin> ]
    // It will expand $1 to the case name and $2 to the full filename
    // matched by the line (case name + extension)

    // It will work on one line at a time starting with the first
    // Use advance() to advance to next line
    // Feed it a BufferedReader in the constructor

    // It also handles the case where there is no .jregr file
    // and then serves the standard commands ".alan : alan $1" + ".a3c : arun $1 < $1.input"

    private static final String[] DEFAULT_EXTENSION =   new String[]   {".alan",            ".input"};
    private static final String[] DEFAULT_COMMAND =     new String[]   {"alan",             "arun"};
    private static final String[][] DEFAULT_ARGUMENTS = new String[][] {new String[]{"$1"}, new String[]{"-n", "-r", "$1"}};
    private static final String[] STDIN =               new String[]   {null,               "$1.input"};

    private String[] words;
    private BufferedReader jregrFileReader;
    private boolean jregrFileExists;
    private int defaultSet = 0;
    private String stdin;

    public CommandsDecoder(BufferedReader fileReader) {
        if (fileReader == null)
            jregrFileExists = false;
        else {
            jregrFileExists = true;
            jregrFileReader = fileReader;
            try {
                fileReader.mark(10000);
                words = splitIntoWords(this.jregrFileReader.readLine());
            } catch (IOException e) {
                jregrFileExists = false;
            }
        }
    }

    public boolean usingDefault() {
        return !jregrFileExists;
    }

    private String[] splitIntoWords(String line) throws IOException {
        String[] split = line.split(" ");
        split = decodeStdin(split);
        return removeColonInSecondPosition(split);
    }

    private String expandSymbols(String caseName, String argument) {
        argument = argument.replaceAll("\\$1", caseName);
        argument = argument.replaceAll("\\$2", caseName+getExtension());
        return argument;
    }

    private String[] decodeStdin(String[] split) {
        if (split[split.length-2].equals("<")) {
            stdin = split[split.length-1];
            split = Arrays.copyOf(split, split.length-2);
        } else
            stdin = null;
        return split;
    }

    private String[] removeColonInSecondPosition(String[] split) {
        String[] w = Arrays.copyOf(split, split.length-1);
        for (int i=1; i<w.length; i++)
            w[i] = split[i+1];
        return w;
    }

    public String getCommand() {
        if (jregrFileExists)
            return words[1];
        else
            return DEFAULT_COMMAND[defaultSet];
    }

    private String[] getArguments() {
        if (jregrFileExists) {
            String [] arguments = new String[words.length - 2];
            for (int i = 2; i < words.length; i++)
                arguments[i-2] = words[i];
            return arguments;
        } else
            return DEFAULT_ARGUMENTS[defaultSet];
    }

    public String getExtension() {
        if (jregrFileExists)
            return words[0];
        else {
            return DEFAULT_EXTENSION[defaultSet];
        }
    }

    public String getStdin(String caseName) {
        String r;
        if (!jregrFileExists)
            r = STDIN[defaultSet];
        else
            r = stdin;
        if (r != null)
            return expandSymbols(caseName, r);
        else
            return r;
    }

    public String[] buildCommandAndArguments(Directory binDirectory, String caseName) {
        final String binPath = binDirectory != null? binDirectory.getAbsolutePath() + Directory.separator : "";
        String command;
        if (binDirectory == null || !binDirectory.executableExist(getCommand()))
            command = expandSymbols(caseName, getCommand());
        else
            command = binPath + expandSymbols(caseName, getCommand());
        String[] arguments = getArguments();
        String[] commandAndArguments = new String[arguments.length + 1];
        commandAndArguments[0] = command;
        for (int i = 0; i < arguments.length; i++) {
            String argument = arguments[i];
            argument = expandSymbols(caseName, argument);
            commandAndArguments[i + 1] = argument;
        }
        return commandAndArguments;
    }

    public boolean advance() {
        if (jregrFileExists) {
            try {
                final String line = jregrFileReader.readLine();
                if (line == null || line == "") return false;
                words = splitIntoWords(line);
                return true;
            } catch (IOException e) {
                return false;
            }
        } else
            return defaultSet++ < 1;
    }

    public void reset() {
        if (!jregrFileExists)
            defaultSet = 0;
        else
            try {
                jregrFileReader.reset();
                words = splitIntoWords(this.jregrFileReader.readLine());
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
    }
}

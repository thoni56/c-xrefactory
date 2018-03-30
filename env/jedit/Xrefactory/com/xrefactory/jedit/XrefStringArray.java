package com.xrefactory.jedit;

public class XrefStringArray {

	private static int XREF_STRING_ARRAY_ALLOC_UNIT = 32;


	String[] 	options;
	int			optionsi;

	public void add(String opt) {
		if (optionsi+1 >= options.length) {
			//&String[] no = new String[options.length+XREF_STRING_ARRAY_ALLOC_UNIT];
			//&if (s.debug) System.err.println("Allocating options["+options.length*2+"]");
			String[] no = new String[options.length*2];
			System.arraycopy(options,0, no, 0, options.length);
			options = no;
		}
		s.assertt(optionsi < options.length-1);
		options[optionsi++] = opt;
	}
	public void add(XrefStringArray opts) {
		int i;
		for(i=0; i<opts.optionsi; i++) add(opts.options[i]);
	}
	public String getLast() {
		if (optionsi == 0) return("");
		return(options[optionsi-1]);
	}
	public String[] toStringArray(boolean fromZeroToMax) {
		String[] res = new String[optionsi];
		if (fromZeroToMax) {
			for(int i=0; i<optionsi; i++) res[i] = options[i];
		} else {
			for(int i=0; i<optionsi; i++) res[optionsi-i-1] = options[i];				
		}
		return(res);
	}
	public String[] toCmdArray() {
		String[] 	res = new String[optionsi];
		String 		cmd, ss;
		int 		i,ii, index;
		for(index=0; index<optionsi; index++) {
			ss = options[index];
			ii = 0; 
			cmd = ""; 
			if (s.osCode == s.OS_WINDOWS) {
				while ((i=ss.indexOf('"',ii)) != -1) {
					cmd += ss.substring(ii, i) + "\\\"";
					ii = i+1;
				}
			}
			if (s.osCode == s.OS_OS2 && ss.indexOf(' ',0) != -1) {
				cmd += "\"" + ss.substring(ii) + "\"";
			} else {
				cmd += ss.substring(ii);
			}
			res[index] = cmd;
		}
		return(res);
		//&return(toStringArray(true));
	}
	public String toString() {
		String res = "";
		for(int i=0; i<optionsi; i++) res += s.sprintOption(options[i])+ " ";
		return(res);
	}
	public void clear() {
		for(int i=0; i<optionsi; i++) options[i] = null;
		optionsi = 0;
	}
	public XrefStringArray() {
		options = new String[XREF_STRING_ARRAY_ALLOC_UNIT];
		optionsi = 0;
	}
	public XrefStringArray(XrefStringArray copy) {
		this();
		add(copy);
	}
}



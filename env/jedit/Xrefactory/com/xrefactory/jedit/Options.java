package com.xrefactory.jedit;

import java.util.*;
import java.io.*;
import javax.swing.*;


class Options {

	public static final int OPT_GENERAL 	= 0;
	public static final int OPT_COMMON 		= 1;
	public static final int OPT_PROJECT 	= 2;
	public static final int OPT_BOTH 		= 3;

	static public class OptionDescription {
		String 		name;
		int			nameLen;
		int 		arity;
		boolean		compact;
		String		shortDescription;
		String		longDescription;
		boolean     interactive;
		int			kind;		// OPT_...
		boolean 	addAtBeginning;
		boolean		recreateTagsIfModified;
		//   'bd' - browse directories
		//   'bf' - browse files
		//   'bfd' - browse files and directories
		String		fileSystemBrowseOptions;
		
		OptionDescription(String name, boolean interactive,
						  int kind, int arity, boolean compact,
						  String shortDescription,
						  String longDescription,
						  String fileSystemBrowseOptions,
						  boolean addAtBeginning, // not needed anymore
						  boolean recreateTagsIfModified
			) {
			this.interactive = interactive;
			this.name = name;
			this.nameLen = name.length();
			this.arity = arity;
			this.kind = kind;
			this.compact = compact;
			this.shortDescription = shortDescription;
			this.longDescription = longDescription;
			this.fileSystemBrowseOptions = fileSystemBrowseOptions;
			this.addAtBeginning = false; //addAtBeginning;
			this.recreateTagsIfModified = recreateTagsIfModified;
		}
	}

	public static OptionDescription optFilesCaseUnSensitive = new OptionDescription(
		"-filescaseunsensitive", false, OPT_GENERAL, 1, true, 
		"File names are considered case unsensitive", 
		"File names are considered case unsensitive", 
		null,false,true);
	public static OptionDescription optJavaVersion = new OptionDescription(
		"-source", true, OPT_BOTH, 2, false, 
		"Sources are written in Java: ", 
		"Java version in which sources are written.", 
		null, false,true);
	public static OptionDescription optCSuffixes = new OptionDescription(
		"-csuffixes=", false, OPT_BOTH, 2, true, 
		"Suffixes determining C sources: ", 
		"Identify files with those suffixes as C sources", 
		null, true,true);
	public static OptionDescription optJavaSuffixes = new OptionDescription(
		"-javasuffixes=", false, OPT_BOTH, 2, true, 
		"Suffixes determining Java sources: ", 
		"Identify files with those suffixes as Java sources", 
		null, true,true);

	public static OptionDescription optProjectMarker = new OptionDescription( 
		"[",true, OPT_PROJECT, 2, true, 
		"Project auto detection directories: ", 
		"Project auto detection directories", 
		"p",false,false);
	public static OptionDescription optInputFile = new OptionDescription(
		"",	true,OPT_PROJECT, 2, true, 
		"Source files & directories: ", 
		"Source files & directories", 
		"p",false,true);
	public static OptionDescription optPruneDirs = new OptionDescription(
		"-prune", true,OPT_PROJECT, 2, false, 
		"Prune directories: ", 
		"Prune directories", 
		"p",false,true);
	public static OptionDescription optRecurseDirs = new OptionDescription(
		"--r",	false, OPT_PROJECT, 1, true, 
		"Do not descent into subdirectories when looking for input files", 
		"Do not descent into subdirectories when looking for input files", 
		null, true,true);
	public static OptionDescription optXrefsFile = new OptionDescription(
		"-refs",true,OPT_PROJECT, 2, false, 
		"Place to store Tags: ",
		"Place to store Tag",
		"fd",false,true);
	public static OptionDescription optXrefNum = new OptionDescription(
		"-refnum=",false,OPT_PROJECT, 2, true,
		"Number of Tag files: ",
		"Number of Tag files",
		null,false,true);
	public static OptionDescription optCommentOption = new OptionDescription(
		"//",false,OPT_BOTH, 0, true, 
		"Comment",
		"Comment",
		null,false,false);
	public static OptionDescription optClassPath = new OptionDescription(
		"-classpath",true,OPT_BOTH, 2, false, 
		"Classpath: ",
		"Top directories with .class files (classpath)",
		"p",false,true);
	public static OptionDescription optSourcePath = new OptionDescription(
		"-sourcepath",true,OPT_BOTH, 2, false, 
		"Sourcepath: ",
		"Top directories with source code (sourcepath)",
		"p",false,true);
	public static OptionDescription optJavaDocPath = new OptionDescription(
		"-javadocpath",true,OPT_BOTH, 2, false, 
		"Javadocpath: ",
		"Top directories with javadoc documentation (javadocpath)",
		"p",false,false);
	public static OptionDescription optJdkClassPath = new OptionDescription(
		"-jdkclasspath",true,OPT_BOTH, 2, false, 
		"Jdk runtime library (rt.jar): ",
		"Jdk runtime library (rt.jar)",
		"p",false,true);
	public static OptionDescription optLicense = new OptionDescription(
		"-license=",true,OPT_COMMON, 2, true, 
		"License string: ",
		"License string",
		null,false,false);

	public static OptionDescription[] allOptions = {
		optInputFile,	// must be first!
		// regular options
		optPruneDirs,
		optRecurseDirs,
		optCSuffixes,
		optJavaSuffixes,
		optLicense,
		optClassPath,
		optSourcePath,
		optJavaDocPath,
		optJavaVersion,
		optXrefNum,
		optProjectMarker,
		optXrefsFile,
		optJdkClassPath,
//&		new OptionDescription("-include",true,OPT_BOTH, 2, false, 
//&							  "Include option file",
//&							  "Include option file",
//&							  "f", false),
		// not interactive options
		optCommentOption,

		new OptionDescription("-set",false,OPT_BOTH, 3, false, 
							  "Set a user defined variable: ",
							  "Set a user defined variable",
							  "fd",false,false),
		new OptionDescription("-refs=",false,OPT_PROJECT, 2, true, 
							  "Place to store Tags: ",
							  "Place to store Tags",
							  "fd",false,true),
	};

	// --------------------- ClassPathIterator ------------------------

	public static class PathIterator {
		String 		ss;
		int 		sslen;
		int 		i;
		String 		separator;
		public String next( ) {
			String res;
			int j = ss.indexOf(separator, i);
			if (j == -1) {
				res = ss.substring(i);
				i = sslen+1;
			} else {
				res = ss.substring(i, j);
				i = j+1;
			}
			return(res);
		}
		public boolean hasNext( ) {
			return(i<sslen);
		}
		public void remove( ) {
		}
		PathIterator(String ss, String separator) {
			this.ss = ss;
			this.separator = separator;
			sslen = ss.length();
			i = 0;
		}
	}

	// ------------------------- Option -------------------------------

	static class Option {
		OptionDescription 	option;
		String[]			fulltext;		// includes option name

		Option(OptionDescription desc) {
			option = desc;
			fulltext = new String[] {desc.name};
		}
		Option(OptionDescription desc, String optval) {
			option = desc;
			fulltext = new String[] {desc.name, optval};
		}
		Option(OptionDescription desc, String[] optvals) {
			option = desc;
			fulltext = new String[optvals.length+1];
			fulltext[0] = desc.name;
			for(int i=0; i<optvals.length; i++) {
				fulltext[i+1] = optvals[i];
			}
		}
		Option(Option o) {
			int i;
			this.option = o.option;
			this.fulltext = new String[o.fulltext.length];
			for(i=0; i<o.fulltext.length; i++) {
				this.fulltext[i] = o.fulltext[i];
			}
		}
	}

	// ------------------------- OptionParser ---------------------------

	static class OptionParser {
		XrefCharBuffer 	ss;
		int 			sslen;
		int 			ind;


		private static OptionDescription getOptionDescription(String opt) {
			int i;
			// options must be tested in reverse order, because of source files!!!
			for(i=allOptions.length-1; i>=0; i--) {
				OptionDescription o = allOptions[i];
				if ((o.compact
					 && opt.length() >= o.nameLen
					 && opt.substring(0, o.nameLen).equals(o.name))
					|| (!o.compact) && opt.equals(o.name)) {
					return(o);
				}
			}
			return(null);
		}

		String parseOptItem() {
			String 		opt;
			int 		b,j;
			b = ind = s.skipBlank(ss, ind, sslen);
			if (ind<sslen && ss.buf[ind]=='"') {
				ind++;
				while (ind<sslen && ss.buf[ind]!='"') ind++;
				opt = ss.substring(b+1, ind);
				ind++;
			} else {
				while (ind<sslen && !Character.isWhitespace(ss.buf[ind])) ind++;
				opt = ss.substring(b,ind);
			}
			// following is useless, but it looks better
			while ((j=opt.indexOf("${dq}"))!=-1) {
				opt = opt.substring(0,j) + "\"" + opt.substring(j+5);
			}
			return(opt);
		}

		private Option parseOption() throws XrefErrorException {
			String opt;
			Option res=null;
			int b;
			b = ind = s.skipBlank(ss, ind, sslen);
			if (ind<sslen && ss.buf[ind]=='[') {
				while (ind<sslen && ss.buf[ind]!=']') ind++;
				opt = ss.substring(b+1, ind);
				ind++;
				return(new Option(optProjectMarker, opt));
			} else {
				opt = parseOptItem();
			}
			OptionDescription desc = getOptionDescription(opt);
			if (desc==null) throw new XrefErrorException("unknown option "+opt);
			if (desc.name.equals("//")) {
				// comment
				b = ind;
				while (ind<sslen && ss.buf[ind]!='\n') ind++;
				String oval = opt.substring(2) + ss.substring(b,ind);
				return(new Option(desc, oval));
			}
			
			if (desc.compact) {
				return(new Option(desc, opt.substring(desc.nameLen)));
			}

			String[] vals = new String[desc.arity-1];
			for(int i=1; i<desc.arity; i++) {
				vals[i-1] = parseOptItem();
			}
			return(new Option(desc, vals));
		}

		public static final int MAX_PROJECTS = 1000;

		public void reParseSingleProject(Options cp) throws Exception {
			cp.optioni = 0;
			while (ind<sslen) {
				Option oo = parseOption();
				if (oo.option == optProjectMarker) {
					cp.add(new Option(optProjectMarker, getProjectChangeDirs(oo.fulltext[1])));
				} else {
					cp.add(oo);
				}
				ind = s.skipBlank(ss, ind, sslen);
			}
		}

		public Options[] parseFile() throws Exception {
			Options[] res = new Options[MAX_PROJECTS];
			Options cp;
			int resi = 0;
			res[resi++] = cp = new Options("", "");
			while (ind<sslen) {
				Option oo = parseOption();
				if (oo.option == optProjectMarker) {
					String pname = getProjectName(oo.fulltext[1]);
					String chdirs = getProjectChangeDirs(oo.fulltext[1]);
					res[resi++] = cp = new Options(pname, chdirs);
				} else {
					cp.add(oo);
				}
				ind = s.skipBlank(ss, ind, sslen);
			}
			Options[] rres = new Options[resi];
			System.arraycopy(res, 0, rres, 0, resi);
			return(rres);
		}

		OptionParser(File ff) throws Exception {
			ss = new XrefCharBuffer();
			ss.appendFileContent(ff);
			sslen = ss.length();
			ind = 0;
		}

		OptionParser(String  s) {
			ss = new XrefCharBuffer(s);
			sslen = ss.length();
			ind = 0;
		}

	} // (OptionParser)

	// ------------------------- Options -------------------------------

    public static final int OPTIONS_ALLOCC_CHUNK = 2;

	String		projectName;
    Option[] 	option = new Option[0];
    int			optioni = 0;
    int			allocatedSize = 0;

	// copy constructor, if adding fiels, do not forget to add it here
	Options(Options op) {
		int i;
		projectName = op.projectName;
		option = new Option[op.option.length];
		for(i=0; i<op.optioni; i++) {
			option[i] = new Option(op.option[i]);
		}
		optioni = op.optioni;
		allocatedSize = op.allocatedSize;
	}

    int indexOf(Option opt) {
		for(int i=0; i<optioni; i++) {
			if (s.stringArrayEqual(option[i].fulltext, opt.fulltext)) return(i);
		}
		return(-1);
    }

    boolean contains(Option opt) {
		return(indexOf(opt)!=-1);
    }

	Option getContainedOption(OptionDescription od) {
		for(int i=0; i<optioni; i++) {
			if (option[i].option == od) return(option[i]);
		}
		return(null);
	}

	void add(Option o) {
		if (optioni >= allocatedSize) {
			Option[] n = new Option[allocatedSize+OPTIONS_ALLOCC_CHUNK];
			System.arraycopy(option, 0, n, 0, allocatedSize);
			option = n;
			allocatedSize += OPTIONS_ALLOCC_CHUNK;
		}
		if (o.option.addAtBeginning) {
			System.arraycopy(option, 0, option, 1, optioni);
			option[0] = o;			
		} else {
			option[optioni] = o;
		}
		optioni++;
	}

	void add(Option o, int preferedIndex) {
		if (optioni >= allocatedSize) {
			Option[] n = new Option[allocatedSize+OPTIONS_ALLOCC_CHUNK];
			System.arraycopy(option, 0, n, 0, allocatedSize);
			option = n;
			allocatedSize += OPTIONS_ALLOCC_CHUNK;
		}
		if (preferedIndex < 0 || preferedIndex >= optioni) {
			option[optioni] = o;
		} else {
			for(int i=optioni; i>preferedIndex; i--) option[i] = option[i-1];
			option[preferedIndex] = o;
		}
		optioni++;
	}

    void delete(int i) {
		s.assertt(optioni > 0);
		optioni --;
		for(int j=i; j<optioni; j++) option[j] = option[j+1];
    }

    int delete(Option opt) {
		int i = indexOf(opt);
		if (i != -1) delete(i);
		return(i);
    }

	String getProjectAutoDetectionOpt() {
		for(int i=0; i<optioni; i++) {
			if (option[i].option == optProjectMarker) {
				return(option[i].fulltext[1]);
			}
		}
		return("");
	}

	String getOptionText(Option o) {
		String res = "";
		if (o.option.compact) {
			String oo = "";
			for(int j=0; j<o.fulltext.length; j++) {
				oo += o.fulltext[j];
			}
			if (o.option == optCommentOption) {
				res += oo + "\n";
			} else {
				res += s.sprintOption(oo);
			}
		} else {
			for(int j=0; j<o.fulltext.length; j++) {
				if (j!=0) res += " ";
				res += s.sprintOption(o.fulltext[j]);
			}
		}
		return(res);
	}


    public String toString(boolean fileFormat) {
		String res = "\n";
		if (!projectName.equals("")) {
			res += "[" + projectName;
			String changeDirectories = getProjectAutoDetectionOpt();
			if (! changeDirectories.equals("")) res += s.classPathSeparator + changeDirectories;
			res += "]\n  ";
		} else {
			res += "  ";
		}
		for(int i=0; i<optioni; i++) {
			if (option[i].option != optProjectMarker) {
				res += getOptionText(option[i]);
				if (res.charAt(res.length()-1)=='\n') {
					res += "  ";
				} else {
					res += "\n  ";
				}

			}
		}
		return(res);
    }

    public String toString() {
		return(toString(false));
	}

	public static void printOptions(Options[] pp, PrintStream oo) {
		for(int i=0; i<pp.length; i++) {
			oo.println(pp[i].toString());
		}
	}

	public static boolean projectMarkersOverlapps(String d1, String d2) {	
		int d1len = d1.length();
		int d2len = d2.length();
		if (d1len == d2len) {
			if (d1.equals(d2)) return(true);
		} else if (d1len < d2len) {
			if (d1.equals(d2.substring(0,d1len-1))) {
				if (d2.substring(d1len, d1len+1).equals(s.slash)) return(true);
			}
		} else {
			if (d2.equals(d1.substring(0,d2len-1))) {
				if (d1.substring(d2len, d2len+1).equals(s.slash)) return(true);
			}
		}
		return(false);
	}

	public static String getProjectName(String oo) {
		PathIterator ii = new PathIterator(oo,s.classPathSeparator);
		if (! ii.hasNext()) return("");
		while (ii.hasNext()) {
			String p = ii.next();
			if (p.length()>0 && !p.substring(0,1).equals(s.slash)) return(p);
		}
		ii = new PathIterator(oo,s.classPathSeparator);
		return(ii.next());
	}

	public static String getProjectChangeDirs(String oo) {
		String pname = getProjectName(oo);
		PathIterator ii = new PathIterator(oo,s.classPathSeparator);
		if (! ii.hasNext()) return("");
		String res = "";
		while (ii.hasNext()) {
			String p = ii.next();
			if (!p.equals(pname)) {
				if (res.equals("")) res = p;
				else res += s.classPathSeparator + p;
			}
		}
		return(res);
	}

	public static String [] getProjectNames(Options [] projects, String zeroName) {
		String[] pn = new String[projects.length];
		for(int i=0; i<projects.length; i++) {
			pn[i] = projects[i].projectName;
		}
		pn[0] = zeroName;
		return(pn);
	}


	public static void saveOptions(Options[] pp, String fileName) {
		PrintStream ps = null;
		try {
			ps = new PrintStream(new FileOutputStream(fileName));
			//&PrintStream ps = System.err;
			printOptions(pp, ps);
			ps.close();
		} catch (Exception e) {
			if (s.debug) e.printStackTrace(System.err);
			JOptionPane.showMessageDialog(s.view, "While saving options: " + e, 
										  "Xrefactory Error", 
										  JOptionPane.ERROR_MESSAGE);				
			if (ps!=null) ps.close();
		}
	}

	public static void dump(Options[] pp) {
		printOptions(pp, System.err);
	}

	void addNewProjectOptions(String projectName, String changeDirectories,
							  String classPath, String sourcePath
		) {
		add(new Option(optInputFile, changeDirectories));
		add(new Option(optPruneDirs, "CVS"+s.classPathSeparator+"backup"));
		add(new Option(optXrefsFile, s.tagFilesDirectory+projectName));
		add(new Option(optXrefNum, ""+10));
		add(new Option(optClassPath, classPath));
		add(new Option(optSourcePath, sourcePath));
		add(new Option(optJavaVersion, "1.3"));
		add(new Option(optCSuffixes, "c"+s.classPathSeparator+"C"));
		add(new Option(optJavaSuffixes, "java"));
		add(new Option(optJdkClassPath, s.jdkClassPath));
		String jdkhome = new File(s.javaHome).getParent();
		if (jdkhome!=null) {
			add(new Option(optJavaDocPath, jdkhome+"/docs/api"));
		}
	}


	Options(String projectName, String changeDirectories) {
		this.projectName = projectName;
		optioni = 0;
		add(new Option(optProjectMarker, changeDirectories));
	}

}



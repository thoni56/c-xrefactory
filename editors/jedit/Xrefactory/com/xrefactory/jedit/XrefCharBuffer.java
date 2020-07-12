package com.xrefactory.jedit;
import java.io.*;
import javax.swing.*;

public class XrefCharBuffer {
	static final int ALLOCATION_UNIT_SIZE = 4096;

	public char[] 	buf;
	public int 		buflen;
	public int 		bufi;

	void increaseSize(long len) {
		if (bufi + len >= buflen) {
			int nsize = buflen;
			//& while (bufi + len >= nsize) nsize += ALLOCATION_UNIT_SIZE;
			while (bufi + len >= nsize) nsize *= 2;
			//&if (s.debug) System.err.println("allocating charbuffer of size " + nsize);
			char[] nbuf = new char[nsize];
			System.arraycopy(buf, 0, nbuf, 0, bufi);
			buflen = nsize;
			buf = nbuf;
		}
	}

	public int length() {
		return(bufi);
	}

	public char lastChar() {
		if (bufi>0) return(buf[bufi-1]);
		return(0);
	}

	public void append(char[] b, int len) {
		increaseSize(len);
		System.arraycopy(b, 0, buf, bufi, len);
		bufi += len;
	}

	public void append(String ss) {
		int len = ss.length();
		increaseSize(len);
		ss.getChars(0, len, buf, bufi);
		bufi += len;
	}

	public void append(XrefCharBuffer ss, int offset, int len) {
		increaseSize(len);
		System.arraycopy(ss.buf, offset, buf, bufi, len);
		bufi += len;
	}

	public String toString() {
		return(new String(buf, 0, bufi));
	}

	public String substring(int beginIndex, int endIndex) {
		return(new String(buf, beginIndex, endIndex-beginIndex));
	}

	public int indexOf(char c, int beginIndex) {
		for(int i=beginIndex; i<bufi; i++) {
			if (buf[i]==c) return(i);
		}
		return(-1);
	}

	public void appendFileContent(File ff) throws Exception {
		long fsize = ff.length();
		if (fsize > s.MAX_FILE_SIZE_FOR_READ) {
			int confirm = JOptionPane.YES_OPTION;
			confirm = JOptionPane.showConfirmDialog(
				s.view,
				"Internal problem: Xrefactory tries to read a document which is too long. This may run\nXrefactory out of memory. Canceling this action usually does not cause serious problems.\n Can I cancel this reading?", "Xrefactory", 
				JOptionPane.YES_NO_OPTION,
				JOptionPane.WARNING_MESSAGE);
			if (confirm == JOptionPane.YES_OPTION) return;
		}
		if (fsize > 0) {
			increaseSize(fsize);
			FileReader tf = new FileReader(ff);
			int n = tf.read(buf, bufi, buflen);
			tf.close();
			if (n!=fsize) throw new XrefErrorException("file "+ff.getName()+" of size "+fsize+" but read only "+n+" bytes");
			bufi += n;
		}
	}

	public void clear() {
		bufi = 0;
	}

	public XrefCharBuffer() {
		buf = new char[ALLOCATION_UNIT_SIZE];
		bufi = 0;
		buflen = ALLOCATION_UNIT_SIZE;
	}

	public XrefCharBuffer(String initialValue) {
		this();
		append(initialValue);
	}

}

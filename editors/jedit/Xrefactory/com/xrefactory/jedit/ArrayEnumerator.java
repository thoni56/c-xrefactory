package com.xrefactory.jedit;

import java.util.*;

class ArrayEnumerator implements Enumeration {
	Object 	array[];
	int		i;
	public Object  nextElement() {
		return(array[i++]);
	}
	public boolean  hasMoreElements() {
		return(i<array.length);
	}
	ArrayEnumerator(Object [] elems) {
		array = elems;
		i=0;
	}
}


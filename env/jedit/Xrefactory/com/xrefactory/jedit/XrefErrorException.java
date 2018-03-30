package com.xrefactory.jedit;

class XrefErrorException extends XrefException {

	String message;

	public String toString() {
		return(message);
	}

	XrefErrorException(String message) {
		super(message);
		this.message = message;
	}

}

package se.alanif.jregr.reporters;

import se.alanif.jregr.exec.RegrCase.State;

public abstract class AbstractRegrReporter implements RegrReporter {

	protected int total;
	protected int fatal;
	protected int virgin;
	protected int pending;
	protected int failing;
	protected int passing;
	protected int suspended;

	public void report(State status) {
		switch (status) {
		case FATAL: fatal(); break;
		case VIRGIN: virgin(); break;
		case PENDING: pending(); break;
		case PASS: pass(); break;
		case FAIL: fail(); break;
		case SUSPENDED: suspended(); break;
		case SUSPENDED_FAIL: suspendedAndFailed(); break;
		case SUSPENDED_PASS: suspendedAndPassed(); break;
		}
	}

}

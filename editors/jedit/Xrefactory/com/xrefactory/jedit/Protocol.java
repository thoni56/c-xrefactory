/* Xrefactory communication protocol constants          */
/* Do not edit .java source file!! Those constants are 	*/
/* primarily defined in protocol.c file.                */

package com.xrefactory.jedit;

public class Protocol {





public static final String XREF_VERSION_NUMBER = "1.6.8";
public static final String XREF_FILE_VERSION_NUMBER = "1.6.0";

public static final String PPC_NO_TAG = "-- no tag --";

public static final String PPC_VERSION_MISMATCH = "version-mismatch";

public static final String PPC_MULTIPLE_COMPLETIONS = "completions";
public static final String PPC_FULL_MULTIPLE_COMPLETIONS = "full-completions";
public static final String PPC_ALL_COMPLETIONS = "all-completions";
public static final String PPC_MULTIPLE_COMPLETION_LINE = "cl";
public static final String PPC_SINGLE_COMPLETION = "single-completion";
public static final String PPC_FQT_COMPLETION = "fqt-completion";
public static final String PPC_SYMBOL_LIST = "symbol-list";

public static final String PPC_CALL_MACRO = "macro";
public static final String PPC_REFACTORING_PRECHECK = "precheck";

public static final String PPC_REFACTORING_REPLACEMENT = "replacement";
public static final String PPC_REFACTORING_CUT_BLOCK = "cut-block";
public static final String PPC_REFACTORING_COPY_BLOCK = "copy-block";
public static final String PPC_REFACTORING_PASTE_BLOCK = "past-block";




public static final String PPC_ASK_CONFIRMATION = "ask-confirmation";
public static final String PPC_ADD_TO_IMPORTS_DIALOG = "add-to-imports-dialog";
public static final String PPC_EXTRACTION_DIALOG = "extraction-dialog";
public static final String PPC_DISPLAY_CLASS_TREE = "display-class-tree";

public static final String PPC_DISPLAY_RESOLUTION = "display-resolution";
public static final String PPC_DISPLAY_OR_UPDATE_BROWSER = "display-or-update-browser";
public static final String PPC_SYMBOL_RESOLUTION = "resolution";
public static final String PPC_SYMBOL = "symbol";
public static final String PPC_VIRTUAL_SYMBOL = "virtual-symbol";
public static final String PPC_CLASS = "class";

public static final String PPC_REFERENCE_LIST = "reference-list";
public static final String PPC_SRC_LINE = "src-line";
public static final String PPC_UPDATE_CURRENT_REFERENCE = "update-current-refn";

public static final String PPC_NO_PROJECT = "no-project-found";
public static final String PPC_NO_SYMBOL = "no-symbol-found";
public static final String PPC_SET_INFO = "set-info";
public static final String PPC_WARNING = "warning";
public static final String PPC_BOTTOM_WARNING = "bottom-warning";
public static final String PPC_ERROR = "error";
public static final String PPC_LICENSE_ERROR = "license-error";
public static final String PPC_FATAL_ERROR = "fatalError";
public static final String PPC_INFORMATION = "information";
public static final String PPC_BOTTOM_INFORMATION = "bottom-information";
public static final String PPC_DEBUG_INFORMATION = "debug-information";

public static final String PPC_GOTO = "goto";
public static final String PPC_BROWSE_URL = "browse-url";
public static final String PPC_FILE_SAVE_AS = "file-save-as";
public static final String PPC_MOVE_FILE_AS = "file-move-as";
public static final String PPC_KILL_BUFFER_REMOVE_FILE = "kill-buffer-remove-file";
public static final String PPC_MOVE_DIRECTORY = "move-directory";
public static final String PPC_INDENT = "indent-block";

public static final String PPC_IGNORE = "ignore";
public static final String PPC_PROGRESS = "progress";

public static final String PPC_UPDATE_REPORT = "update-report";

public static final String PPC_AVAILABLE_REFACTORINGS = "available-refactorings";

public static final String PPC_SYNCHRO_RECORD = "sync";



public static final String PPC_LC_POSITION = "position-lc";
public static final String PPC_OFFSET_POSITION = "position-off";
public static final String PPC_STRING_VALUE = "str";
public static final String PPC_INT_VALUE = "int";



public static final String PPCA_LEN = "len";
public static final String PPCA_LINE = "line";
public static final String PPCA_COL = "col";
public static final String PPCA_OFFSET = "off";
public static final String PPCA_VALUE = "val";
public static final String PPCA_LEVEL = "lev";
public static final String PPCA_VCLASS = "vcl";
public static final String PPCA_SELECTED = "selected";
public static final String PPCA_BASE = "base";
public static final String PPCA_DEF_REFN = "drefn";
public static final String PPCA_REFN = "refn";
public static final String PPCA_TYPE = "type";
public static final String PPCA_MTYPE = "mtype";
public static final String PPCA_CONTINUE = "continue";
public static final String PPCA_INTERFACE = "interface";
public static final String PPCA_INDENT = "indent";
public static final String PPCA_TREE_DEPS = "deps";
public static final String PPCA_TREE_UP = "up";
public static final String PPCA_DEFINITION = "definition";
public static final String PPCA_SYMBOL = "symbol";
public static final String PPCA_BEEP = "beep";
public static final String PPCA_NUMBER = "number";
public static final String PPCA_NO_FOCUS = "nofocus";


public static final int PPCV_BROWSER_TYPE_INFO = 0;
public static final int PPCV_BROWSER_TYPE_WARNING = 1;


public static final int PPC_AVR_NO_REFACTORING = 00;
public static final int PPC_AVR_RENAME_SYMBOL = 10;
public static final int PPC_AVR_RENAME_CLASS = 20;
public static final int PPC_AVR_RENAME_PACKAGE = 30;
public static final int PPC_AVR_ADD_PARAMETER = 40;
public static final int PPC_AVR_DEL_PARAMETER = 50;
public static final int PPC_AVR_MOVE_PARAMETER = 60;
public static final int PPC_AVR_MOVE_STATIC_FIELD = 70;
public static final int PPC_AVR_MOVE_STATIC_METHOD = 80;
public static final int PPC_AVR_MOVE_FIELD = 90;
public static final int PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC = 100;
public static final int PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC = 110;
public static final int PPC_AVR_PULL_UP_FIELD = 120;
public static final int PPC_AVR_PULL_UP_METHOD = 130;
public static final int PPC_AVR_PUSH_DOWN_FIELD = 140;
public static final int PPC_AVR_PUSH_DOWN_METHOD = 150;
public static final int PPC_AVR_MOVE_CLASS = 160;
public static final int PPC_AVR_MOVE_CLASS_TO_NEW_FILE = 170;
public static final int PPC_AVR_MOVE_ALL_CLASSES_TO_NEW_FILE = 180;
public static final int PPC_AVR_ENCAPSULATE_FIELD = 190;
public static final int PPC_AVR_SELF_ENCAPSULATE_FIELD = 200;
public static final int PPC_AVR_ADD_TO_IMPORT = 210;
public static final int PPC_AVR_EXTRACT_METHOD = 220;
public static final int PPC_AVR_EXTRACT_FUNCTION = 230;
public static final int PPC_AVR_EXTRACT_MACRO = 240;
public static final int PPC_AVR_REDUCE_NAMES = 250;
public static final int PPC_AVR_EXPAND_NAMES = 260;
public static final int PPC_AVR_ADD_ALL_POSSIBLE_IMPORTS = 270;
public static final int PPC_AVR_SET_MOVE_TARGET = 280;
public static final int PPC_AVR_UNDO = 290;
public static final int PPC_MAX_AVAILABLE_REFACTORINGS = 300;






public static final String PPC_SET_PROJECT = "set-project";




}

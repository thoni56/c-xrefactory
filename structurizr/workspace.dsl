workspace {
    model {
        developer = Person "Developer" "Edits source code using an editor"
	editor = SoftwareSystem "Editor" "Allows Developer to modify source code and perform refactoring operations" Editor
	source = Element "Source Code" "a set of files stored on disk" "" DB

	cxrefactory = SoftwareSystem "C-xrefactory" "Analyses source code, receives and processes requests for navigation and refactoring" Cxrefactory {
	    editorExtension = container editorExtension "Extends the Editor with c-xref operations and interfaces to the c-xrefactory API" "Plugin"
	    cxrefCore = container cxrefCore "C Language program" "Refactoring Browser core"
	    settingsStore = container settingsStore "Non-standard format settings file" "Configuration file for project settings" DB
	    referencesDb = container referencesDb "Source code information storage" "Stores all information about the source code in the project which is updated by scanning all or parts of the source when required" DB

	    editorExtension -> settingsStore "writes" "new project wizard"
	    cxrefCore -> settingsStore "read"
	    editorExtension -> cxrefCore "API" "requests information and gets commands to modify source code"
	    cxrefCore -> referencesDb "read/write"
	    cxrefCore -> source "read/analyze"
	    
	}

	developer -> editor "usual editor/IDE operations"
	developer -> cxrefactory "configuration and command line invocations"
	editor -> source "normal editing operations"
	editor -> cxrefactory "navigation and refactoring requests"
	cxrefactory -> editor "positioning and editing responses"
	cxrefactory -> source "read/analyze"
	editor -> editorExtension "extends" "Editor extension protocol"
	developer -> settingsStore "edit"
	editorExtension -> source "extended c-xrefactory operations"
    }

    views {
        systemContext cxrefactory "SystemContext" {
	    include *
	    autolayout
	}

	container cxrefactory "ContainerView" {
	    include *
	    autolayout
	}

	styles {
	    element Editor {
	        icon https://www.emacswiki.org/emacs/EmacsSplashScreen.png
	        background #999999
		color #ffffff
	    }
	    element Cxrefactory {
	        icon https://www.emacswiki.org/emacs/EmacsSplashScreen.png
	        background #1168bd
		color #ffffff
	    }
	    element Person {
	        shape person
		background #686868
		color #ffffff
	    }
	    element DB {
	    	shape Cylinder
            }
	    element Container {
	        background #438dd5
        }
    }
}

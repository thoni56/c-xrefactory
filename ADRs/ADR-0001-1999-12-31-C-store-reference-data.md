# Where to store reference data in an effective and compact format

* Status: Current
* Deciders: probably Marián and others
* Date: somewhen in c-xrefactory early history

## Y-Statement

_In the context of_ effectively and compactly percisting reference data stored on disk,
_facing the fact_ that we need to access it quickly and that it should always reflect the current content of source files
_we decided for_ custom optimized and encoded file storage
_and neglected_ database or free text storage
_to achieve_ some level of storage
_accepting_ complexity of access code and performance hits,
_because_ it seemed right at the time.
    
## Decision Drivers

* Previous format seems to have been a more clear text version and the concern was (maybe) space on disk

## Considered Options

* Compacting the clear text format by encoding data, avoiding duplicate information?

## Decision Outcome


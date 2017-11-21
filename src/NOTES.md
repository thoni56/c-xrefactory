# NOTES about c-xref implementation, design etc. #

## Building ##

One step in the build process is generating initialization information
for all the things in standard include files, which of course becomes
very dependent on the system you are running this on.

The initial recovered c-xrefactory relied on having a working _c-xref_
for the current system. I don't really know how they managed to do
that for all the various systems they were supporting.

Modern thinking is that you should always be able to build from
source, so this is something that needed change. We also want to
distribute _c-xref_ as an el-get library which requires building from
source and should generate a version specific for the current system.

The strategy selected, until some better idea comes along, is to try
to build a _c-xref.bs_, if there isn't one already, from the sources in
the repository and then use that to re-generate the definitions and
rebuild a proper _c-xref_.

Currently that causes some [warning]:s when _c-xref.bs_ generates new
_strTdef.h_ and _strFill.h_.

## Design ##

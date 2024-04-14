#include "jsemact.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "misc.h"
#include "proto.h"
#include "usage.h"
#include "yylex.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "filedescriptor.h"
#include "editor.h"
#include "symbol.h"
#include "javafqttab.h"
#include "list.h"
#include "fileio.h"
#include "filetable.h"
#include "stackmemory.h"

#include "parsers.h"
#include "recyacc.h"
#include "jslsemact.h"



JavaStat *javaStat;
JavaStat s_initJavaStat;


void fillJavaStat(JavaStat *javaStat, IdList *className, TypeModifier *thisType, Symbol *thisClass,
                  int currentNestedIndex, char *currentPackage, char *unnamedPackagePath,
                  char *namedPackagePath, SymbolTable *locals, IdList *lastParsedName,
                  unsigned methodModifiers, CurrentlyParsedClassInfo parsingPositions, int classFileNumber,
                  JavaStat *next) {

    javaStat->className = className;
    javaStat->thisType = thisType;
    javaStat->thisClass = thisClass;
    javaStat->currentNestedIndex = currentNestedIndex;
    javaStat->currentPackage = currentPackage;
    javaStat->unnamedPackagePath = unnamedPackagePath;
    javaStat->namedPackagePath = namedPackagePath;
    javaStat->locals = locals;
    javaStat->lastParsedName = lastParsedName;
    javaStat->methodModifiers = methodModifiers;
    javaStat->cp = parsingPositions;
    javaStat->classFileNumber = classFileNumber;
    javaStat->next = next;
}

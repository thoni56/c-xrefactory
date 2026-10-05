#pragma once

#include "argumentsvector.h"


extern void buildKnowledgeOfCU(int fileNumber, ArgumentsVector baseArgs);

extern void reparseFile(int fileNumber, ArgumentsVector baseArgs);

extern void markAllCompilationUnitsStale(void);

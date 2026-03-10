#pragma once

#include "argumentsvector.h"

extern void reparseStaleFile(int fileNumber, ArgumentsVector baseArgs);

extern void parseFileWithFullInit(char *fileName, ArgumentsVector baseArgs);

#pragma once

#include "argumentsvector.h"
#include "freshness.h"


#define MAX_CUS_TO_REPARSE 128


extern int collectCUsIncluding(int fileNumber, int cuFileNumbers[], int cuCount, int maxCUs);

extern void parseUnparsedSiblingCUs(Inputs *inputs, int requestFileNumber, bool requestIsParsed,
                                    ArgumentsVector baseArgs);

extern void bringHeldFilesUpToDate(Inputs *inputs, ArgumentsVector baseArgs);

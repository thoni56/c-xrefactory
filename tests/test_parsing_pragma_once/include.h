#pragma once

#define GLUE(a,b) a##b
#define VAR(n) GLUE(var_, n)
extern int VAR(PART);

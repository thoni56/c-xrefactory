#ifndef ENUMS_H_INCLUDED
#define ENUMS_H_INCLUDED

/* Some CPP magic to be able to print enums as strings */
/* Usage example in e.g. storage.h/storage.c */
#define GENERATE_ENUM_VALUE(ENUM) ENUM,
#define GENERATE_ENUM_STRING(STRING) #STRING,

#endif

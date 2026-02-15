#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED


#define SCOPES_BITS 3

typedef enum scope {
    DefaultScope,
    GlobalScope,
    FileScope,
    AutoScope,
    MAX_SCOPES
} Scope;

#endif

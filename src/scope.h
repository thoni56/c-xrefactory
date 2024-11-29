#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED


typedef enum scope {
    DefaultScope,
    GlobalScope,
    FileScope,
    AutoScope,
    MAX_SCOPES
} Scope;

#endif

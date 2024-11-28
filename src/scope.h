#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED


typedef enum referenceScope {
    DefaultScope,
    GlobalScope,
    FileScope,
    AutoScope,
    MAX_SCOPES
} ReferenceScope;

#endif

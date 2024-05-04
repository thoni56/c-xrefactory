#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED


typedef enum referenceScope {
    ScopeDefault,
    ScopeGlobal,
    ScopeFile,
    ScopeAuto,
    MAX_SCOPES
} ReferenceScope;

#endif

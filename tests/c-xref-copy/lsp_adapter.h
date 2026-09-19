#ifndef LSP_ADAPTER_H_INCLUDED
#define LSP_ADAPTER_H_INCLUDED

#include "json_utils.h"

/* LSP adapter functions */
extern JSON *findDefinition(const char *uri, JSON *position);

#endif /* LSP_ADAPTER_H_INCLUDED */

#include "referencesstack.h"

#include <stdlib.h>
#include <assert.h>

#include "proto.h"
#include "reference.h"
#include "completion.h"
#include "menu.h"
#include "options.h"
#include "position.h"
#include "globals.h"

/* Generic stack operations for ReferencesStack and its semantic aliases
 * (BrowserStack, CompletionStack, RetrieverStack).
 *
 * Functions here operate on the stack structure itself and don't depend
 * on the semantic context (browser navigation vs completion vs retrieval).
 */

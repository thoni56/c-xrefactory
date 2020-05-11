#include "generate.h"

#include <stdbool.h>

#include "globals.h"            /* s_opt & cxOut */


/* ******************************************************************* */

static void generateEnumString(Symbol *symbol) {
    char *name;
    SymbolList *e;
    assert(symbol);
    name = symbol->name;
    e = symbol->u.enums;
    assert(name);
    if (s_opt.generate_header) {
        fprintf(cxOut,"extern char * %sEnumName[];\n",name);
    }
    if (s_opt.generate_body) {
        fprintf(cxOut,"char * %sEnumName[] = {\n",name);
        for(;e!=NULL;e=e->next) {
            assert(e->d);
            assert(e->d->name);
            /*          if (e->d->name == NULL) fprintf(fOut,"\"__ERROR__\"");*/
            /*          else */
            fprintf(cxOut,"\t\"%s\"",e->d->name);
            if (e->next!=NULL) fprintf(cxOut,",");
            fprintf(cxOut,"\n");
        }
        fprintf(cxOut,"};\n\n");
    }
}

/* ********************************************************************* */

void generate(Symbol *symbol) {
    assert(symbol);
    if (symbol->name==NULL || symbol->name[0]==0)
        return;
    if (symbol->bits.symType == TypeEnum) {
        if (s_opt.generate_enum_name)
            generateEnumString(symbol);
    }
}

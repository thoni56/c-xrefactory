#include "generate.h"

#include <stdbool.h>

#include "commons.h"
#include "globals.h"


typedef enum {
    GenerateStructureFill,
    GenerateInternalFill,
    GenerateStructureCopy,
} GenerateKind;


static bool isSubstructureToFill(Symbol *symbol) {
    if (symbol->bits.storage == StorageError) return false;
    assert(symbol->u.type);
    if (symbol->u.type->kind == TypeAnonymeField) return false;
    if (symbol->u.type->kind == TypeFunction) return false;
    if (symbol->u.type->kind == TypeArray) return false;
    return true;
}

static char *getFillArgumentName(int argument_number, int argument_count,
                                 GenerateKind action) {
    static char res[TMP_STRING_SIZE];
    if (action == GenerateInternalFill) {
        sprintf(res,"_ARG_%d_OF_%d ARGS", argument_number, argument_count);
    } else {
        sprintf(res,"ARG%d",argument_number);
    }
    return(res);
}

static int genFillStructArguments(Symbol *symbol,
                                  int i,
                                  bool fullFlag
                                  ) {
    Symbol *records;
    Symbol *p;
    static int deep=0;
    assert(symbol->u.s);
    records = symbol->u.s->records;
    deep++;

    if (deep > MAX_NESTED_DEEP) {
        fatalError(ERR_ST,"too much nested structures, probably recursive", XREF_EXIT_ERR);
    }
    for(p=records; p!=NULL; p=p->next) {
        if (isSubstructureToFill(p)) {
            if (p->u.type->kind == TypeStruct  && fullFlag) {
                i = genFillStructArguments(p->u.type->u.t,i,1);
            } else if (p->u.type->kind == TypeUnion) {
                fprintf(cxOut, ", %s", getFillArgumentName(i++, 0, GenerateStructureFill));
                fprintf(cxOut, ", %s", getFillArgumentName(i++, 0, GenerateStructureFill));
            } else {
                fprintf(cxOut, ", %s", getFillArgumentName(i++, 0, GenerateStructureFill));
            }
        }
    }
    deep--;
    return(i);
}


#define FILL_ARGUMENT_NAME "STRUCTP"

static void fillField(char *prefix, char *name, int argument_number,
                      int argument_count, GenerateKind action) {
    fprintf(cxOut,"\t(%s)->%s%s = %s;\\\n", FILL_ARGUMENT_NAME,
            prefix,name,getFillArgumentName(argument_number,argument_count,action));
}


static int genFillStructBody(Symbol *defin, int i, int argn, bool fullFlag,
                             char *pref, GenerateKind action) {
    Symbol *rec;
    Symbol *p;
    char prefix[TMP_STRING_SIZE];
    char rname[TMP_STRING_SIZE];
    int l1,l2;
    static int deep=0;
    assert(defin->u.s);
    rec = defin->u.s->records;
    deep++;
    if (deep > MAX_NESTED_DEEP) {
        fatalError(ERR_ST,"too much nested structures, probably recursive", XREF_EXIT_ERR);
    }
    for(p=rec; p!=NULL; p=p->next) {
        if (isSubstructureToFill(p)) {
            if (    (p->u.type->kind == TypeStruct && fullFlag)
                    || p->u.type->kind == TypeUnion) {
                l1 = strlen(pref);
                l2 = strlen(p->name);
                assert(l1+l2+6 < TMP_STRING_SIZE);
                strcpy(prefix,pref);
                strcpy(prefix+l1, p->name);
                prefix[l1+l2] = 0;
                if (p->u.type->kind == TypeUnion) {
                    if (fullFlag) {
                        strcpy(rname,getFillArgumentName(i,argn,action));
                        i++;
                        fprintf(cxOut,
                                "\t_FILLUREC_%s_##%s((&(%s)->%s), %s);\\\n",
                                p->u.type->u.t->name,rname,
                                FILL_ARGUMENT_NAME,
                                prefix,
                                getFillArgumentName(i,argn,action));
                        i++;
                    } else {
                        prefix[l1+l2] = '.';
                        prefix[l1+l2+1] = 0;
                        strcpy(rname,getFillArgumentName(i,argn,action));
                        i++;
                        fillField( prefix, rname, i, argn, action);
                        i++;
                    }
                } else {
                    prefix[l1+l2] = '.';
                    prefix[l1+l2+1] = 0;
                    i=genFillStructBody(p->u.type->u.t,i,argn,1,prefix,action);
                }
            } else {
                fillField( pref, p->name, i, argn, action);
                i++;
            }
        }
    }
    deep--;
    return(i);
}

/* ******************************************************************* */

static void generateTypedefForStructOrUnion(Symbol *symbol) {
    char *name;

    assert(symbol);
    name = symbol->name;
    assert(symbol->u.s);
    assert(name);
    if (symbol->bits.symType == TypeStruct) {
        fprintf(cxOut,"typedef struct %s S_%s;\n", name, name);
    } else {
        fprintf(cxOut,"typedef union %s U_%s;\n", name, name);
    }
}


static void generateStructureFillMacros(Symbol *symbol) {
    char *name;
    int argn;

    assert(symbol);
    name = symbol->name;
    assert(symbol->u.s);
    assert(name);
    fprintf(cxOut,"#define FILL_%s(%s", name, FILL_ARGUMENT_NAME);
    argn = genFillStructArguments(symbol, 0, 0);
    fprintf(cxOut,") {\\\n");
    genFillStructBody(symbol, 0, argn, 0, "", GenerateStructureFill);
    fprintf(cxOut,"}\n");
    fprintf(cxOut,"#define FILLF_%s(%s", name, FILL_ARGUMENT_NAME);
    argn = genFillStructArguments(symbol, 0, 1);
    fprintf(cxOut,") {\\\n");
    genFillStructBody(symbol, 0, argn, 1, "", GenerateStructureFill);
    fprintf(cxOut,"}\n");
    fprintf(cxOut,"#define _FILLF_%s(%s, ARGS) {\\\n", name, FILL_ARGUMENT_NAME);
    genFillStructBody(symbol, 0, argn, 1, "", GenerateInternalFill);
    fprintf(cxOut,"}\n");
}


static void generateUnionFillMacros(Symbol *symbol) {
    char *name;
    Symbol *rec,*p;
    assert(symbol);
    name = symbol->name;
    assert(symbol->u.s);
    rec = symbol->u.s->records;
    assert(name);
    for(p=rec; p!=NULL; p=p->next) {
        if (p->bits.symType == TypeDefault) {
            if (p->u.type->kind == TypeStruct) {
                fprintf(cxOut,
                        "#define _FILLUREC_%s_%s(XX,ARGS) _FILLF_%s(&(XX->%s),ARGS)\n",
                        name, p->name, p->u.type->u.t->name, p->name);
            } else {
                fprintf(cxOut,
                        "#define _FILLUREC_%s_%s(XX,ARG) XX->%s = ARG;\n",
                        name, p->name, p->name);
            }
        }
    }
}


static void generateStructCopyFunction(Symbol *symbol) {
    char *name;
    Symbol *rec;
    assert(symbol);
    name = symbol->name;
    assert(symbol->u.s);
    rec = symbol->u.s->records;
    assert(name);
    if (rec==NULL) return;
    if (s_opt.header) {
        fprintf(cxOut,
                "extern struct %s*\ncopy_%s(struct %s *, void *(*alloc)(int));\n",
                name,name,name);
    }
    if (s_opt.body) {
        fprintf(cxOut,
                "struct %s *\ncopy_%s(struct %s *s, void *(*alloc)(int n))",
                name,name,name);
        fprintf(cxOut," {\n");
        fprintf(cxOut,"  struct %s *d;\n",name);
        fprintf(cxOut,"  d = (struct %s *) (*alloc)(sizeof(struct %s));\n",
                name,name);
        fprintf(cxOut,"  memcpy(d,s,sizeof(struct %s));\n",name);
        genFillStructBody(symbol, 0, 0/*????*/, 1, "", GenerateStructureCopy);
        fprintf(cxOut,"  return(d);\n");
        fprintf(cxOut,"}\n\n");
    }
}

static void generateTypedefForEnum(Symbol *symbol) {
    char *name;

    assert(symbol);
    name = symbol->name;
    assert(name);
}

static void generateEnumString(Symbol *symbol) {
    char *name;
    SymbolList *e;
    assert(symbol);
    name = symbol->name;
    e = symbol->u.enums;
    assert(name);
    if (s_opt.header) {
        fprintf(cxOut,"extern char * %sName[];\n",name);
    }
    if (s_opt.body) {
        fprintf(cxOut,"char * %sName[] = {\n",name);
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

void generateArgumentSelectionMacros(int n) {
    int i,j,k;
    for (i=1; i<=n; i++) {
        for(j=0; j<i; j++) {
            fprintf(cxOut,"#define _ARG_%d_OF_%d",j,i);
            for (k=0;k<i;k++) fprintf(cxOut,"%sARG%d",(k?",":"("),k);
            fprintf(cxOut,") ARG%d\n",j);
        }
    }
}

/* ********************************************************************* */
/* ********************************************************************* */

void generate(Symbol *symbol) {
    assert(symbol);
    if (symbol->name==NULL || symbol->name[0]==0) return;
    if (symbol->bits.symType==TypeStruct || symbol->bits.symType==TypeUnion) {
        if (s_opt.typedefg) generateTypedefForStructOrUnion(symbol);
        if (s_opt.str_fill) {
            if (symbol->bits.symType==TypeStruct) generateStructureFillMacros(symbol);
            if (symbol->bits.symType==TypeUnion) generateUnionFillMacros(symbol);
        }
        if (s_opt.str_copy) generateStructCopyFunction(symbol);
    } else if (symbol->bits.symType == TypeEnum) {
        if (s_opt.typedefg) generateTypedefForEnum(symbol);
        if (s_opt.enum_name) generateEnumString(symbol);
    }
}

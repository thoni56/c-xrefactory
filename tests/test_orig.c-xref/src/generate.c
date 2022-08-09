/*
	$Revision: 1.1.1.1 $
	$Date: 2002/02/13 21:20:27 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/

static int subToFill(S_symbol *p, int fullFlag) {
	if (p->b.storage == StorageError) return(0);
	assert(p->u.type);
	if (p->u.type->m == TypeAnonymeField) return(0);
	if (p->u.type->m == TypeFunction) return(0);
	if (p->u.type->m == TypeArray) return(0);			/* just for now*/
//&	if (p->u.type->m == TypeUnion && ! fullFlag) return(0);
//&	if (p->u.type->m == TypeUnion) return(0);
	return(1);
}

static void genCopy(	S_symbol *defin,
						S_symbol 		*p,
						S_typeModifiers		*tt,
						char 				*pref,
						int 				pi
					) {
	char stars[200];
	int i;
	for(i=0; i<pi; i++) stars[i]='*';
	stars[i]=0;
	if (tt->m == TypePointer && tt->next->m==TypeVoid) {
		sprintf(tmpBuff,"a void * pointer in %s, wrong structre copy possible",
						defin->name);
		warning(ERR_ST,tmpBuff);
	} else if (tt->m == TypePointer && tt->next->m!=TypeStruct) {
/* TO DO BETTER !!! g++ makes an error when konverting void* to void**/
		fprintf(cxOut,"  (void*)%sd->%s%s=(*alloc)(sizeof(*%ss->%s%s));\n",
						stars,pref,p->name,stars,pref,p->name);
		if (tt->next->m==TypePointer) {
			genCopy(defin,p,tt->next,pref,pi+1);
		} else {
			fprintf(cxOut,"  memcpy(%sd->%s%s,%ss->%s%s,sizeof(*%ss->%s%s));\n",
					stars,pref,p->name,stars,pref,p->name,stars,pref,p->name);
		}
	} else if (tt->m == TypePointer && tt->next->m==TypeStruct) {
		fprintf(cxOut,"  %sd->%s%s = copy_%s(%ss->%s%s,alloc);\n",stars,
					pref,p->name,tt->next->u.t->name,stars,pref,p->name);
	}
}

static void fillGenArgHeadItem(char *name, int  *i) {
	fprintf(cxOut,",%s%d", name, *i);
	(*i)++;
}


static int genFillStructArguments(	S_symbol *defin,
									int i,
									int fullFlag
	) {
	S_symbol *rec;
	S_symbol *p;
	static int deep=0;
	assert(defin->u.s);
	rec = defin->u.s->records;
	deep++;

	if (deep > MAX_NESTED_DEEP) {
		fatalError(ERR_ST,"too much nested structures, probably recursive", XREF_EXIT_ERR);
	}
	for(p=rec; p!=NULL; p=p->next) {
		if (subToFill(p, fullFlag)) {
			if (p->u.type->m == TypeStruct	&& fullFlag) {
				i = genFillStructArguments(p->u.type->u.t,i,1);
			} else if (p->u.type->m == TypeUnion) {
					fillGenArgHeadItem("FARG", &i);
					fillGenArgHeadItem("FARG", &i);
			} else {
				fillGenArgHeadItem("FARG", &i);
			}
		}
	}
	deep--;
	return(i);
}

static char *getFillArgumentName(int i, int argn, int action) {
	static char res[TMP_STRING_SIZE];
	if (action == InternalFillGenerate) {
		sprintf(res,"_ARG_PROJECT_%d_%d ARGS", i, argn);
	} else {
		sprintf(res,"FARG%d",i);
	}
	return(res);
}

static void genFillItem(char *pref, int action,
				 char *name,	int  i, int argn) {
	fprintf(cxOut,"\t(XXX)->%s%s = %s;\\\n",
				pref,name,getFillArgumentName(i,argn,action));
}


static int genFillStructBody(S_symbol *defin, int i, int argn, int fullFlag, 
							 char *pref, int action) {
	S_symbol *rec;
	S_symbol *p;
	S_typeModifiers *tt;
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
		if (subToFill(p, fullFlag)) {
			if (	(p->u.type->m == TypeStruct && fullFlag) 
					|| p->u.type->m == TypeUnion) {
				l1 = strlen(pref);
				l2 = strlen(p->name);
				InternalCheck(l1+l2+6 < TMP_STRING_SIZE);
				strcpy(prefix,pref);
				strcpy(prefix+l1, p->name);
				prefix[l1+l2] = 0;
				if (p->u.type->m == TypeUnion) {
					if (fullFlag) {
						strcpy(rname,getFillArgumentName(i,argn,action)); 
						i++;
						fprintf(cxOut,
								"\t_FILLUREC_%s_##%s((&(XXX)->%s), %s);\\\n",
								p->u.type->u.t->name,rname,prefix,
								getFillArgumentName(i,argn,action));
						i++;
					} else {
						prefix[l1+l2] = '.';
						prefix[l1+l2+1] = 0;
						strcpy(rname,getFillArgumentName(i,argn,action)); 
						i++;
						genFillItem( prefix, action, rname, i, argn);
						i++;
					}
				} else {
					prefix[l1+l2] = '.';
					prefix[l1+l2+1] = 0;
					i=genFillStructBody(p->u.type->u.t,i,argn,1,prefix,action);
				}
			} else {
				genFillItem( pref, action, p->name, i, argn);
				i++;
			}
		}
	}
	deep--;
	return(i);
}

/* ******************************************************************* */
/* ******************************************************************* */

static void genStructTypedef(S_symbol *s) {
	char *name;
	S_symbol *rec;
	assert(s);
	name = s->name;
	assert(s->u.s);
	rec = s->u.s->records;
	assert(name);
	if (s->b.symType == TypeStruct) {
		fprintf(cxOut,"typedef struct %s S_%s;\n",name,name);
	} else {
		fprintf(cxOut,"typedef union %s U_%s;\n",name,name);
	}
}


static void genStructFill(S_symbol *s) {
	char *name;
	int argn;
	S_symbol *rec;
	assert(s);
	name = s->name;
	assert(s->u.s);
	rec = s->u.s->records;
	assert(name);
	fprintf(cxOut,"#define FILL_%s(XXX",name);
	argn = genFillStructArguments(s, 0, 0);
	fprintf(cxOut,") {\\\n");
	genFillStructBody(s, 0, argn, 0, "", FillGenerate);
	fprintf(cxOut,"}\n");
	fprintf(cxOut,"#define FILLF_%s(XXX",name);
	argn = genFillStructArguments(s, 0, 1);
	fprintf(cxOut,") {\\\n");
	genFillStructBody(s, 0, argn, 1, "", FillGenerate);
	fprintf(cxOut,"}\n");
	fprintf(cxOut,"#define _FILLF_%s(XXX,ARGS) {\\\n",name);
	genFillStructBody(s, 0, argn, 1, "", InternalFillGenerate);
	fprintf(cxOut,"}\n");
}


static void genUnionRecords(S_symbol *s) {
	char *name;
	int argn;
	S_symbol *rec,*p;
	assert(s);
	name = s->name;
	assert(s->u.s);
	rec = s->u.s->records;
	assert(name);
	for(p=rec; p!=NULL; p=p->next) {
		if (p->b.symType == TypeDefault) {
			if (p->u.type->m == TypeStruct) {
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


static void genStructCopy(S_symbol *s) {
	char *name;
	S_symbol *rec;
	assert(s);
	name = s->name;
	assert(s->u.s);
	rec = s->u.s->records;
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
		genFillStructBody(s, 0, 0/*????*/, 1, "", CopyGenerate);
		fprintf(cxOut,"  return(d);\n");
		fprintf(cxOut,"}\n\n");
	}
}

static void genEnumTypedef(S_symbol *s) {
	char *name;
	S_symbolList *e;
	assert(s);
	name = s->name;
	e = s->u.enums;
	assert(name);
/*
	fprintf(fOut,"typedef enum %s E_%s;\n",name,name);
*/
}

static void genEnumText(S_symbol *s) {
	char *name;
	S_symbolList *e;
	assert(s);
	name = s->name;
	e = s->u.enums;
	assert(name);
	if (s_opt.header) {
		fprintf(cxOut,"extern char * %sName[];\n",name);
	}
	if (s_opt.body) {
		fprintf(cxOut,"char * %sName[] = {\n",name);
		for(;e!=NULL;e=e->next) {
			assert(e->d);
			assert(e->d->name);
/*			if (e->d->name == NULL) fprintf(fOut,"\"__ERROR__\"");*/
/*			else */
			fprintf(cxOut,"\t\"%s\"",e->d->name);
			if (e->next!=NULL) fprintf(cxOut,",");
			fprintf(cxOut,"\n");
		}
		fprintf(cxOut,"};\n\n",name);
	}
}

void genProjections(int n) {
	int i,j,k;
	for (i=1; i<=n; i++) {
		for(j=0; j<i; j++) {
			fprintf(cxOut,"#define _ARG_PROJECT_%d_%d",j,i);
			for (k=0;k<i;k++) fprintf(cxOut,"%sA%d",(k?",":"("),k);
			fprintf(cxOut,") A%d\n",j);
		}
	}
}

/* ********************************************************************* */
/* ********************************************************************* */

void generate(S_symbol *s) {
	assert(s);
	if (s->name==NULL || s->name[0]==0) return;
	if (s->b.symType==TypeStruct || s->b.symType==TypeUnion) {
		if (s_opt.typedefg) genStructTypedef(s);
		if (s_opt.str_fill) {
			if (s->b.symType==TypeStruct) genStructFill(s);
			if (s->b.symType==TypeUnion) genUnionRecords(s);
		}
		if (s_opt.str_copy) genStructCopy(s);
	} else if (s->b.symType == TypeEnum) {
		if (s_opt.typedefg) genEnumTypedef(s);
		if (s_opt.enum_name) genEnumText(s);
	}
}



#ifndef _MEMMAC__H
#define _MEMMAC__H

/* ******************** a simple memory handler ************************ */

#define ALLIGNEMENT_OFF(xxx,allign) (allign-1-((((unsigned)(xxx))-1) & (allign-1)))
#define ALLIGNEMENT(xxx,allign) (((char*)(xxx))+ALLIGNEMENT_OFF(xxx,allign))


/* ********************************************************************* */

#define SM_FREE_SPACE(mem,n) (mem##i+(n) < SIZE_##mem)
#define SM_FREED_POINTER(mem,ppp) (\
	((char*)ppp) >= mem + mem##i && ((char*)ppp) < mem + SIZE_##mem \
)

#define SM_INIT(mem) {mem##i = 0;}
#define SM_ALLOCC(mem,p,n,t) {\
	assert( (n) >= 0);\
	/* memset(mem+mem##i,0,(n)*sizeof(t)); */\
	mem##i = ((char*)ALLIGNEMENT(mem+mem##i,STANDARD_ALLIGNEMENT)) - mem;\
	if (mem##i+(n)*sizeof(t) >= SIZE_##mem) {\
		fatalError(ERR_NO_MEMORY,#mem, XREF_EXIT_ERR);\
	}\
	p = (t*) (mem + mem##i);\
	/* memset(p,0,(n)*sizeof(t)); /* for detecting any bug */\
	mem##i += (n)*sizeof(t);\
}
#define SM_ALLOC(mem,p,t) {SM_ALLOCC(mem,p,1,t);}
#define SM_REALLOCC(mem,p,n,t,oldn) {\
	assert(((char *)(p)) + (oldn)*sizeof(t) == mem + mem##i);\
	mem##i = ((char*)p) - mem;\
	SM_ALLOCC(mem,p,n,t);\
}
#define SM_FREE_UNTIL(mem,p) {\
	assert((p)>=mem && (p)<= mem+mem##i);\
	mem##i = ((char*)(p))-mem;\
}



/* ********************************************************************* */

#define DM_FREE_SPACE(mem,n) (mem->i+(n) < mem->size)

#define DM_IS_BETWEEN(mem,ppp,iii,jjj) (\
	((char*)ppp) >= ((char*)&mem->b) + (iii) && ((char*)ppp) < ((char*)&mem->b) + (jjj) \
)
#define DM_FREED_POINTER(mem,ppp) DM_IS_BETWEEN(mem,ppp,mem->i,mem->size)

#define DM_INIT(mem) {mem->i = 0;}
#define DM_ALLOCC(mem,p,n,t) {\
	assert( (n) >= 0);\
	mem->i = ((char*)ALLIGNEMENT(((char*)&mem->b)+mem->i,STANDARD_ALLIGNEMENT)) - ((char*)&mem->b);\
	if (mem->i+(n)*sizeof(t) >= mem->size) {\
		if (mem->overflowHandler(n)) longjmp(s_memoryResize,1); \
		else fatalError(ERR_NO_MEMORY,#mem, XREF_EXIT_ERR);\
	}\
	p = (t*) (((char*)&mem->b) + mem->i);\
	mem->i += (n)*sizeof(t);\
}
#define DM_ALLOC(mem,p,t) {DM_ALLOCC(mem,p,1,t);}
#define DM_REALLOCC(mem,p,n,t,oldn) {\
	assert(((char *)(p)) + (oldn)*sizeof(t) == &mem->b + mem->i);\
	mem->i = ((char*)p) - &mem->b;\
	DM_ALLOCC(mem,p,n,t);\
}
#define DM_FREE_UNTIL(mem,p) {\
	assert((p)>= ((char*)&mem->b) && (p)<= ((char*)&mem->b)+mem->i);\
	mem->i = ((char*)(p)) - ((char*)&mem->b);\
}


/* ************* a suplementary level with free-lists ******************** */

#define RLM_ALLIGNEMENT STANDARD_ALLIGNEMENT

#ifdef OLD_RLM_MEMORY

#define RLM_INIT(mem) {\
	int i;\
	SM_INIT(mem);\
	for(i=0; i<MAX_BUFFERED_SIZE_##mem; i++) mem##FreeList[i] = NULL;\
}
#define RLM_SOFT_ALLOCC(mem,p,nn,t) {\
	int n = (int) ALLIGNEMENT(((nn)*sizeof(t)),RLM_ALLIGNEMENT);\
    char *rlmtmp;\
	assert(n >= sizeof(void*));\
	assert(n < MAX_BUFFERED_SIZE_##mem);\
	if ((p = (t*) mem##FreeList[n]) == NULL) {\
		p = NULL;\
		if (SM_FREE_SPACE(mem, n)) {\
			SM_ALLOCC(mem, rlmtmp, n, char);\
			p = (t *) rlmtmp;\
		}\
	} else {\
		mem##FreeList[n] = *((void **) p);\
	}\
}
#define RLM_ALLOCC(mem,p,nn,t) {\
	RLM_SOFT_ALLOCC(mem,p,nn,t);\
	if (p==NULL) SM_ALLOCC(mem, p, nn, t);\
}
#define RLM_ALLOC(mem,p,t) {RLM_ALLOCC(mem,p,1,t);}
#define RLM_SOFT_ALLOC(mem,p,t) {RLM_SOFT_ALLOCC(mem,p,1,t);}
#define RLM_FREE(mem,p,nn) {\
	int n = (int) ALLIGNEMENT((nn),RLM_ALLIGNEMENT);\
	* (void **) p = mem##FreeList[n];\
	mem##FreeList[n] = p;\
}

#define RLM_FREE_COUNT(mem) {\
	int i,count;\
	void *r;\
	count = 0;\
	for( i=0; i<MAX_BUFFERED_SIZE_##mem; i++ ) {\
		r = mem##FreeList[i];\
		while (r!=NULL) {\
			count += i;\
			r = *(void**)r;\
		}\
	}\
	fprintf(stdout,"\ntotal free %d\n",count); fflush(stdout);\
}

#define RLM_DUMP(mem) {\
	int i;\
	void *r;\
	for( i=0; i<MAX_BUFFERED_SIZE_##mem; i++ ) {\
		fprintf(stdout,"%2d: ",i);\
		r = mem##FreeList[i];\
		while (r!=NULL) {\
			fprintf(stdout,"%x ",r);\
			r = *(void**)r;\
		}\
		fprintf(stdout,"\n",i);\
	}\
}

#else

#define RLM_INIT(mem) {mem##AllocatedBytes = 0; CHECK_INIT();}

#define RLM_SOFT_ALLOCC(mem,p,nn,t) {\
	int n = (nn) * sizeof(t);\
	if (n+mem##AllocatedBytes > SIZE_##mem) {\
		p = NULL;\
	} else {\
		p = (t*) malloc(n);\
		mem##AllocatedBytes += n;\
		CHECK_ALLOC(p, n);\
	}\
}
#define RLM_FREE(mem,p,nn) {\
	CHECK_FREE(p, nn);\
	mem##AllocatedBytes -= nn; /* discount first, free after (because of nn=strlen(p)) */\
	free(p);\
}

#endif


/* ********************************************************************** */

#ifdef OLCX_MEMORY_CHECK
#define CHECK_INIT() {s_olcx_chech_arrayi=0;}
#define CHECK_ALLOC(p, n) {\
	if (s_olcx_chech_arrayi == -1) assert(0);\
	s_olcx_chech_array[s_olcx_chech_arrayi] = p;\
	s_olcx_chech_array_sizes[s_olcx_chech_arrayi] = n;\
	s_olcx_chech_arrayi ++;\
	assert(s_olcx_chech_arrayi<OLCX_CHECK_ARRAY_SIZE-2);\
}
#define CHECK_FREE(p, nn) {\
	int _itmpi, _nnlen;\
	_nnlen = nn;\
	if (nn == 0) error(ERR_INTERNAL, "Freeing chunk of size 0");\
	for (_itmpi=0; _itmpi<s_olcx_chech_arrayi; _itmpi++) {\
		if (s_olcx_chech_array[_itmpi] == p) {\
			s_olcx_chech_array[_itmpi]=NULL;\
			if (_nnlen != s_olcx_chech_array_sizes[_itmpi]) {\
				sprintf(tmpBuff, "Cell %d allocated with size %d and freed with %d\n", _itmpi, s_olcx_chech_array_sizes[_itmpi], _nnlen);\
				fatalError(ERR_INTERNAL, tmpBuff, XREF_EXIT_ERR);\
			}\
			break;\
		}\
	}\
	if (_itmpi==s_olcx_chech_arrayi) fatalError(ERR_INTERNAL, "Freeing unalocated cell", XREF_EXIT_ERR);\
}
#define CHECK_FINAL() {\
	int _itmpi;\
	for (_itmpi=0; _itmpi<s_olcx_chech_arrayi; _itmpi++) {\
		if (s_olcx_chech_array[_itmpi]!=NULL) {\
			sprintf(tmpBuff, "unfreed cell #%d\n", _itmpi);\
			error(ERR_INTERNAL, tmpBuff;\
		}\
	}\
}
#else
#define CHECK_INIT() {}
#define CHECK_ALLOC(p, n) {}
#define CHECK_FREE(p, n) {}
#define CHECK_FINAL() {}
#endif


/* ********************************************************************** */

#endif


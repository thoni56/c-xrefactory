/*
         LIST_CONS(i/o element, i/o list)      
*/
#define LIST_CONS(elem,list) {(elem)->next = (list); list = elem;}
#define CONS(elem,list) ((elem)->next = (list), elem)

/*
         LIST_APPEND(sort,i/o first list, i second list)    
*/
#define LIST_APPEND(sort,first,second) { \
	if ((first)==NULL) first = second; \
	else { \
		register sort *tempnsl; tempnsl = first; \
		while (tempnsl->next!=NULL) tempnsl=tempnsl->next; \
		tempnsl->next = second; \
	} \
}

/*
         LIST_FREE(sort, i/o list)
*/
#define LIST_FREE(sort,list) { 	\
	register sort *tempnsl1,*tempnsl2; tempnsl1 = list; \
	while (tempnsl1!=NULL) { \
	  tempnsl2=tempnsl1; tempnsl1 = tempnsl1->next; XX_FREE(tempnsl2); \
	} \
}

/*
         LIST_FIND(sort,i key value to find, keyFieldName in struct sort,
                  i initial list, o founded element)
*/
#define LIST_FIND(sort,key,keyFieldName,iList,oList) { \
	register sort *tmp; tmp = iList; \
	while (tmp!=NULL && tmp->keyFieldName!=(key) ) \
		tmp = tmp->next; \
	oList = tmp; \
}


/*
         LIST_PLACE(o struct sort **position, sort, i key value,
                          keyFieldName, i &list)
*/
#define LIST_PLACE(position,sort,key,keyFieldName,listA) { \
	register sort *tmp,**lStMp; \
	tmp = *listA; lStMp = listA; \
	while (tmp!=NULL && tmp->keyFieldName != (key)) { \
		lStMp = &(tmp->next); tmp = *lStMp; \
	} \
	position = lStMp; \
}

/*
          LIST_MAP(sort,i list,STATEMENT)
*/
#define LIST_MAP(sort,list,STATEMENT) { \
	sort *tmp, *tmptmp; tmp = list; \
	while (tmp!=NULL) {tmptmp=tmp->next; STATEMENT; tmp=tmptmp; } \
}

/*
          LIST_LEN(o int reslen, sort, i list)
*/
#define LIST_LEN(reslen,sort,list) { \
	register sort *tmp; register int co=0; tmp = list; \
	while (tmp!=NULL) {++co; tmp = tmp->next; }\
	reslen = co; \
}

/*
          LIST_MEMBER(o int res, sort, i list, i member)
*/
#define LIST_MEMBER(res,sort,list,member) { \
	register sort *tmp; tmp = list; \
	res = 0;\
	while (tmp!=NULL) {\
		if (tmp == member) {res=1; break;}\
		tmp = tmp->next;\
	}\
}

#define LIST_REVERSE(sort, iolist) {\
	register sort *list,*tmp,*revlist;\
	list = iolist;\
	revlist = NULL;\
	while (list!=NULL) {\
		tmp = list->next; list->next = revlist;\
		revlist = list;   list = tmp;\
	}\
	iolist = revlist;\
}


/*
          SORTED_LIST_FIND(o reselem, sort, i key value, keyFieldName, i list) 
*/      

#define SORTED_LIST_FIND(reselem,sort,key,keyFieldName,list) { \
	register sort *tmp; tmp = list; \
	while (tmp!=NULL && tmp->keyFieldName < (key)) tmp=tmp->next; \
	reselem=tmp; \
}

/*
          SORTED_LIST_PLACE(o struct sort **position, sort, i key value,
                          keyFieldName, i &list)
*/
#define SORTED_LIST_PLACE(position,sort,key,keyFieldName,listA) { \
	register sort *tmp,**lStMp; \
	tmp = *listA; lStMp = listA; \
	while (tmp!=NULL && tmp->keyFieldName < (key)) { \
		lStMp = &(tmp->next); tmp = *lStMp; \
	} \
	position = lStMp; \
}

/*
          SORTED_LIST_INSERT(sort,i/o elem, keyFieldName, i/o ioList)
*/
#define SORTED_LIST_INSERT(sort,elem,keyFieldName,ioList) { \
	sort **ptmp; \
	SORTED_LIST_PLACE(ptmp,sort,(elem)->keyFieldName,keyFieldName,ioList); \
	LIST_CONS(elem,(*ptmp)); \
}


/*                    SORTED LIST WITH NON_TRIVIAL ORDERING      */

/*
          SORTED_LIST_FIND2(o reselem, sort, i key value, i list) 
*/      

#define SORTED_LIST_FIND2(reselem,sort,key,list) { \
	register sort *tmp; tmp = list; \
	while (tmp!=NULL && SORTED_LIST_LESS(tmp,(key))) tmp=tmp->next; \
	reselem=tmp; \
}

/*
          SORTED_LIST_FIND2(o reselem, sort, i key value, i list) 
*/      

#define SORTED_LIST_FIND3(reselem,sort,key,list, lessfunction) { \
	register sort *tmp; tmp = list; \
	while (tmp!=NULL && lessfunction(tmp,(key))) tmp=tmp->next; \
	reselem=tmp; \
}

/*
          SORTED_LIST_PLACE2(o struct sort **position, sort, i key value,
                          i &list)
*/
#define SORTED_LIST_PLACE2(position,sort,key,listA) { \
	register sort *tmp,**lStMp; \
	tmp = *(listA); lStMp = listA; \
	while (tmp!=NULL && SORTED_LIST_LESS(tmp, (key))) { \
		lStMp = &(tmp->next); tmp = *lStMp; \
	} \
	position = lStMp; \
}

/*
          SORTED_LIST_INSERT2(sort,i/o elem, i/o ioList)
*/
#define SORTED_LIST_INSERT2(sort,elem,ioList) { \
	sort **ptmp; \
	SORTED_LIST_PLACE2(ptmp,sort,(elem),ioList); \
	LIST_CONS(elem,(*ptmp)); \
}

/*
          SORTED_LIST_PLACE3(o struct sort **position, sort, i key value,
                          i &list)
*/
#define SORTED_LIST_PLACE3(position,sort,key,listA,lessfunction) { \
	register sort *tmp,**lStMp; \
	tmp = *(listA); lStMp = listA; \
	while (tmp!=NULL && lessfunction(tmp, (key))) { \
		lStMp = &(tmp->next); tmp = *lStMp; \
	} \
	position = lStMp; \
}

/*
          SORTED_LIST_INSERT3(sort,i/o elem, i/o ioList, lessfunction)
*/
#define SORTED_LIST_INSERT3(sort,elem,ioListAddress,lessfunction) { \
	sort **ptmp; \
	SORTED_LIST_PLACE3(ptmp,sort,(elem),ioListAddress,lessfunction); \
	LIST_CONS(elem,(*ptmp)); \
}

/*
          LIST_INSERTSORT(sort, i/o ioList, lessfunction)
*/
#define LIST_SORT(sort,ioList,lessfunction) { \
	register sort *tmpInList, *tmptmp;\
	sort *tmpOutList;\
	tmpOutList = NULL;\
	tmpInList = ioList;\
	while (tmpInList!=NULL) {\
		tmptmp = tmpInList->next;\
		tmpInList->next = NULL;\
		SORTED_LIST_INSERT3(sort, tmpInList, (&tmpOutList), lessfunction);\
		tmpInList = tmptmp;\
	}\
	ioList = tmpOutList;\
}

/*
          LIST_MERGE(sort, lista, listb, sort **ioResTail, lessfunction)
*/
#define LIST_MERGE(sort, a, b, restail, lessfunction) { \
  while (a!=NULL && b!=NULL) {\
    if (lessfunction(a, b)) {\
      *restail=a;  restail= &(a->next); a=a->next;\
    } else {\
      *restail=b;  restail= &(b->next); b=b->next;\
    }\
  }\
  if (a!=NULL) *restail=a;\
  else *restail=b;\
  while (*restail!=NULL) restail= &((*restail)->next);\
}\

/*
          LIST_INSERTSORT(sort, i/o ioList, lessfunction)
*/
#define LIST_MERGE_SORT(sort, ioList, lessfunction) { \
  sort *_macrores;\
  _macrores = (ioList);\
  {\
   sort *a, *b, *todo, *t, **restail;\
   int i, n, contFlag;\
   contFlag = 1;\
   for(n=1; contFlag; n=n+n) {\
     todo=_macrores; _macrores=NULL; restail = &_macrores; contFlag=0;\
     while (todo!=NULL) {\
       a=todo;\
       for(i=1, t=a;  i<n && t!=NULL;  i++, t=t->next) ;\
       if (t==NULL) {\
         *restail = a;\
         break;\
       }\
       b=t->next; t->next=NULL;\
       for(i=1, t=b;  i<n && t!=NULL;  i++, t=t->next) ;\
       if (t==NULL) todo=NULL;\
       else {\
         todo=t->next; t->next=NULL;\
       }\
       LIST_MERGE(sort,a,b,restail,lessfunction);\
       contFlag=1;\
     }\
   }\
  }\
  (ioList) = _macrores;\
}




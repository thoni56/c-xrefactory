/* Generic (macro) handling of lists */

/* LIST_CONS(i/o element, i/o list) */
#define LIST_CONS(elem,list) {(elem)->next = (list); list = elem;}

/* LIST_APPEND(type, i/o first list, i second list) */
#define LIST_APPEND(type, first, second) {                      \
        if ((first)==NULL) first = second;                      \
        else {                                                  \
            type *tempnsl; tempnsl = first;                     \
            while (tempnsl->next!=NULL) tempnsl=tempnsl->next;  \
            tempnsl->next = second;                             \
        }                                                       \
    }

/* LIST_FREE(type, i/o list) */
#define LIST_FREE(type, list) {                                         \
        type *tempnsl1, *tempnsl2; tempnsl1 = list;                     \
        while (tempnsl1!=NULL) {                                        \
            tempnsl2=tempnsl1;                                          \
            tempnsl1 = tempnsl1->next;                                  \
            StackMemoryFree(tempnsl2);                                  \
        }                                                               \
    }

/* LIST_FIND(sort, i key value to find, keyFieldName in struct sort,
   i initial list, o found element)
*/
#define LIST_FIND(type, key, keyFieldName, iList, oList) { \
        type *tmp; tmp = iList;                            \
        while (tmp != NULL && tmp->keyFieldName != (key))  \
            tmp = tmp->next;                               \
        oList = tmp;                                       \
    }


/* LIST_LEN(o int reslen, type, i list) */
#define LIST_LEN(reslen, type, list) {              \
        type *tmp; int co=0; tmp = list;            \
        while (tmp!=NULL) {++co; tmp = tmp->next; } \
        reslen = co;                                \
    }

/* LIST_MEMBER(o int res, type, i list, i member) */
#define LIST_MEMBER(res, type, list, member) {  \
        type *tmp; tmp = list;                  \
        res = 0;                                \
        while (tmp!=NULL) {                     \
            if (tmp == member) {res=1; break;}  \
            tmp = tmp->next;                    \
        }                                       \
    }

#define LIST_REVERSE(type, iolist) {                \
        type *list, *tmp, *revlist;                 \
        list = iolist;                              \
        revlist = NULL;                             \
        while (list != NULL) {                        \
            tmp = list->next; list->next = revlist; \
            revlist = list;   list = tmp;           \
        }                                           \
        iolist = revlist;                           \
    }


/* SORTED_LIST_FIND(o reselem, type, i key value, keyFieldName, i list) */
#define SORTED_LIST_FIND(reselem, type, key, keyFieldName, list) {  \
        register type *tmp; tmp = list;                             \
        while (tmp!=NULL && tmp->keyFieldName < (key)) tmp=tmp->next;   \
        reselem=tmp;                                                    \
    }

/* SORTED_LIST_PLACE(o struct sort **position,
                     type,
                     i key value,
                     keyFieldName,
                     i &list) */
#define SORTED_LIST_PLACE(position, type, key, keyFieldName, listA) { \
        type *tmp,**lStMp;                                            \
        tmp = *listA; lStMp = listA;                                  \
        while (tmp!=NULL && tmp->keyFieldName < (key)) {              \
            lStMp = &(tmp->next); tmp = *lStMp;                       \
        }                                                             \
        position = lStMp;                                             \
    }

/* SORTED_LIST_INSERT(type, i/o elem, keyFieldName, i/o ioList) */
#define SORTED_LIST_INSERT(type, elem, keyFieldName, ioList) {          \
        type **ptmp;                                                    \
        SORTED_LIST_PLACE(ptmp,type,(elem)->keyFieldName,keyFieldName,ioList); \
        LIST_CONS(elem,(*ptmp));                                        \
    }


/* SORTED LIST WITH NON_TRIVIAL ORDERING */

/* SORTED_LIST_FIND2(o reselem, type, i key value, i list) */
#define SORTED_LIST_FIND2(reselem, type, key, list) {                   \
        type *tmp; tmp = list;                                          \
        while (tmp!=NULL && SORTED_LIST_LESS(tmp,(key))) tmp=tmp->next; \
        reselem=tmp;                                                    \
    }

/* SORTED_LIST_FIND2(o reselem, type, i key value, i list) */
#define SORTED_LIST_FIND3(reselem, type, key, list, lessfunction) { \
        type *tmp; tmp = list;                                      \
        while (tmp!=NULL && lessfunction(tmp,(key))) tmp=tmp->next; \
        reselem=tmp;                                                \
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

#define SORTED_LIST_LESS(tmp,key) (positionIsLessThan((tmp)->p, (key).p))

#define SORTED_LIST_NEQ(tmp,key) (positionsAreNotEqual((tmp)->p, (key).p))

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

/* Generic (macro) handling of lists */

/* LIST_CONS(i/o element, i/o list) */
#define LIST_CONS(elem, list) {(elem)->next = (list); (list) = (elem);}

/* LIST_APPEND(type, in/out list1, in list2) */
#define LIST_APPEND(type, list1, list2)                                                                           \
    {                                                                                                             \
        if ((list1) == NULL)                                                                                      \
            list1 = list2;                                                                                        \
        else {                                                                                                    \
            type *tempnsl;                                                                                        \
            tempnsl = list1;                                                                                      \
            while (tempnsl->next != NULL)                                                                         \
                tempnsl = tempnsl->next;                                                                          \
            tempnsl->next = list2;                                                                                \
        }                                                                                                         \
    }

/* LIST_LEN(out int length, type, in type list) */
#define LIST_LEN(length, type, list)                                                                              \
    {                                                                                                             \
        type *tmp;                                                                                                \
        length = 0;                                                                                               \
        tmp    = list;                                                                                            \
        while (tmp != NULL) {                                                                                     \
            ++length;                                                                                             \
            tmp = tmp->next;                                                                                      \
        }                                                                                                         \
    }

#define LIST_REVERSE(type, iolist) {                \
        type *list, *tmp, *revlist;                 \
        list = iolist;                              \
        revlist = NULL;                             \
        while (list != NULL) {                      \
            tmp = list->next; list->next = revlist; \
            revlist = list;   list = tmp;           \
        }                                           \
        iolist = revlist;                           \
    }


/* SORTED LIST WITH NON_TRIVIAL ORDERING */

/* SORTED_LIST_FIND2(o result, type, i key value, i list) */
#define SORTED_LIST_FIND2(result, type, key, list) {                    \
        type *tmp; tmp = list;                                          \
        while (tmp!=NULL && SORTED_LIST_LESS(tmp,(key)))                \
            tmp=tmp->next;                                              \
        result=tmp;                                                     \
    }

/* SORTED_LIST_FIND3(o result, type, i key value, i list, i lessfunction) */
#define SORTED_LIST_FIND3(result, type, key, list, lessfunction) {  \
        type *tmp = list;                                           \
        while (tmp!=NULL && lessfunction(tmp,(key)))                \
            tmp=tmp->next;                                          \
        result=tmp;                                                 \
    }

/*
  SORTED_LIST_PLACE2(o struct sort **position, i key value,
                     i &theList)
*/
#define SORTED_LIST_PLACE2(position, key, theList) {        \
        Reference *tmp, **lStMp;                            \
        tmp = *(theList);                                   \
        lStMp = theList;                                    \
        while (tmp!=NULL && SORTED_LIST_LESS(tmp, (key))) { \
            lStMp = &(tmp->next);                           \
            tmp = *lStMp;                                   \
        }                                                   \
        position = lStMp;                                   \
    }

/*
  SORTED_LIST_PLACE3(o struct sort **position, sort, i key value,
                     i &list, i lessfunction)
*/
#define SORTED_LIST_PLACE3(position, sort, key, theList, lessfunction) {  \
        register sort *tmp,**lStMp;                                     \
        tmp = *(theList); lStMp = theList;                              \
        while (tmp!=NULL && lessfunction(tmp, (key))) {                 \
            lStMp = &(tmp->next); tmp = *lStMp;                         \
        }                                                               \
        position = lStMp;                                               \
    }

/*
  SORTED_LIST_INSERT3(sort, i/o elem, i/o ioList, lessfunction)
*/
#define SORTED_LIST_INSERT3(sort, elem, ioListAddress, lessfunction) {   \
        sort **ptmp;                                                     \
        SORTED_LIST_PLACE3(ptmp,sort,(elem),ioListAddress,lessfunction); \
        LIST_CONS(elem,(*ptmp));                                         \
    }

#define SORTED_LIST_LESS(tmp,key) (positionIsLessThan((tmp)->position, (key).position))

#define SORTED_LIST_NEQ(tmp,key) (positionsAreNotEqual((tmp)->position, (key).position))

/*
  LIST_SORT(listType, i/o ioList, lessfunction)
*/
#define LIST_SORT(listType,ioList,lessfunction) {                       \
        listType *tmpInList, *tmptmp;                                   \
        listType *tmpOutList;                                           \
        tmpOutList = NULL;                                              \
        tmpInList = ioList;                                             \
        while (tmpInList!=NULL) {                                       \
            tmptmp = tmpInList->next;                                   \
            tmpInList->next = NULL;                                     \
            SORTED_LIST_INSERT3(listType, tmpInList, (&tmpOutList), lessfunction); \
            tmpInList = tmptmp;                                         \
        }                                                               \
        ioList = tmpOutList;                                            \
    }

/*
  LIST_MERGE(lista, listb, i/o **resultP, lessfunction)
*/
#define LIST_MERGE(a, b, resultP, lessfunction) {               \
        while (a!=NULL && b!=NULL) {                            \
            if (lessfunction(a, b)) {                           \
                *resultP=a;  resultP= &(a->next); a=a->next;    \
            } else {                                            \
                *resultP=b;  resultP= &(b->next); b=b->next;    \
            }                                                   \
        }                                                       \
        if (a!=NULL) *resultP=a;                                \
        else *resultP=b;                                        \
        while (*resultP!=NULL)                                  \
            resultP= &((*resultP)->next);                       \
    }                                                           \

/*
  LIST_MERGE_SORT(listType, i/o ioList, lessfunction)
*/
#define LIST_MERGE_SORT(listType, ioList, lessfunction) {               \
        listType *_macrores;                                            \
        _macrores = (ioList);                                           \
        {                                                               \
            listType *a, *b, *todo, *t, **rest;                         \
            int i;                                                      \
            bool continueFlag = true;                                   \
            for (int n=1; continueFlag; n=n+n) {                        \
                todo=_macrores; _macrores=NULL; rest = &_macrores; continueFlag=false; \
                while (todo!=NULL) {                                    \
                    a=todo;                                             \
                    for (i=1, t=a;  i<n && t!=NULL;  i++, t=t->next) \
                        ;                                               \
                    if (t==NULL) {                                      \
                        *rest = a;                                      \
                        break;                                          \
                    }                                                   \
                    b=t->next; t->next=NULL;                            \
                    for (i=1, t=b;  i<n && t!=NULL;  i++, t=t->next) \
                        ;                                               \
                    if (t==NULL)                                        \
                        todo=NULL;                                      \
                    else {                                              \
                        todo=t->next; t->next=NULL;                     \
                    }                                                   \
                    LIST_MERGE(a,b,rest,lessfunction);                  \
                    continueFlag=true;                                  \
                }                                                       \
            }                                                           \
        }                                                               \
        (ioList) = _macrores;                                           \
    }


#ifdef XREF_HUGE	  			/*SBD*/


/* HUGE only !!!!!, see normal compacted encoding after this */


#define PutLexChar(xxx,dd) {*(dd)++ = xxx;}
#define PutLexToken(xxx,dd) {\
	*(dd)++ = ((unsigned)(xxx))%256;\
	*(dd)++ = ((unsigned)(xxx))/256;\
}
#define PutLexShort(xxx,dd) PutLexToken(xxx,dd)
#define PutLexInt(xxx,dd) {\
	unsigned tmp;\
	tmp = xxx;\
	*(dd)++ = tmp%256; tmp /= 256;\
	*(dd)++ = tmp%256; tmp /= 256;\
	*(dd)++ = tmp%256; tmp /= 256;\
	*(dd)++ = tmp%256; tmp /= 256;\
}
#define PutLexFilePos(xxx,dd) PutLexInt(xxx,dd)
#define PutLexNumPos(xxx,dd) PutLexInt(xxx,dd)
#define PutLexPosition(cfile,cline,idcoll,dd) {\
	PutLexFilePos(cfile,dd);\
	PutLexNumPos(cline,dd);\
	PutLexNumPos(idcoll,dd);\
/*fprintf(dumpOut,"push idp %d %d %d\n",cfile,cline,idcoll);*/\
}
/*#define PutLexPosition(cfile,cline,idcoll,dd) {}*/


#define GetLexChar(xxx,dd) {xxx = *((unsigned char*)dd++);}
#define GetLexToken(xxx,dd) {\
	xxx = *((unsigned char*)dd++);\
	xxx += 256 * *((unsigned char*)dd++);\
}
#define GetLexShort(xxx,dd) GetLexToken(xxx,dd)
#define GetLexInt(xxx,dd) {\
	xxx = *((unsigned char*)dd++);\
	xxx += 256 * *((unsigned char*)dd++);\
	xxx += 256 * 256 * *((unsigned char*)dd++);\
	xxx += 256 * 256 * 256 * *((unsigned char*)dd++);\
}
#define GetLexFilePos(xxx,dd) GetLexInt(xxx,dd)
#define GetLexNumPos(xxx,dd) GetLexInt(xxx,dd)
#define GetLexPosition(pos,tmpcc) {\
	GetLexFilePos(pos.file,tmpcc);\
	GetLexNumPos(pos.line,tmpcc);\
	GetLexNumPos(pos.coll,tmpcc);\
}
/*#define GetLexPosition(pos,tmpcc) {}*/


#define NextLexChar(dd) (*((unsigned char*)dd))
#define NextLexToken(dd) (\
	*((unsigned char*)dd) \
	+ 256 * *(((unsigned char*)dd)+1)\
)
#define NextLexShort(dd) NextLexToken(dd)
#define NextLexInt(dd) (\
	*((unsigned char*)dd) \
	+ 256 * *(((unsigned char*)dd)+1)\
	+ 256 * 256 * *(((unsigned char*)dd)+2) \
	+ 256 * 256 * 256 * *(((unsigned char*)dd)+3)\
)
#define NextLexFilePos(dd) NextLexInt(dd)
#define NextLexNumPos(dd) NextLexInt(dd)
#define NextLexPosition(pos,tmpcc) {\
	pos.file = NextLexFilePos(tmpcc);\
	pos.line = NextLexNumPos(tmpcc+sizeof(short));\
	pos.coll = NextLexNumPos(tmpcc+2*sizeof(short));\
}




#else	  			/*SBD*/

/* not huge, but compacted */









#define PutLexChar(xxx,dd) {*(dd)++ = xxx;}
#define PutLexToken(xxx,dd) {PutLexShort(xxx,dd);}
#define PutLexShort(xxx,dd) {\
	*(dd)++ = ((unsigned)xxx)%256;\
	*(dd)++ = ((unsigned)xxx)/256;\
}
#define PutLexCompacted(xxx,dd) {\
	assert(((unsigned) xxx)<4194304);\
	if (((unsigned)xxx)>=128) {\
		if (((unsigned)xxx)>=16384) {\
			*(dd)++ = ((unsigned)xxx)%128+128;\
			*(dd)++ = ((unsigned)xxx)/128%128+128;\
			*(dd)++ = ((unsigned)xxx)/16384;\
		} else {\
			*(dd)++ = ((unsigned)xxx)%128+128;\
			*(dd)++ = ((unsigned)xxx)/128;\
		}\
	} else {\
		*(dd)++ = ((unsigned char)xxx);\
	}\
}
#define PutLexInt(xxx,dd) {\
	unsigned tmp;\
	tmp = xxx;\
	*(dd)++ = tmp%256; tmp /= 256;\
	*(dd)++ = tmp%256; tmp /= 256;\
	*(dd)++ = tmp%256; tmp /= 256;\
	*(dd)++ = tmp%256; tmp /= 256;\
}
#define PutLexPosition(cfile,cline,idcoll,dd) {\
	assert(cfile>=0 && cfile<MAX_FILES);\
	PutLexCompacted(cfile,dd);\
	PutLexCompacted(cline,dd);\
	PutLexCompacted(idcoll,dd);\
/*fprintf(dumpOut,"push idp %d %d %d\n",cfile,cline,idcoll);*/\
}
/*#define PutLexPosition(cfile,cline,idcoll,dd) {}*/


#define GetLexChar(xxx,dd) {xxx = *((unsigned char*)dd++);}
#define GetLexToken(xxx,dd) {GetLexShort(xxx,dd);}
#define GetLexShort(xxx,dd) {\
	xxx = *((unsigned char*)dd++);\
	xxx += 256 * *((unsigned char*)dd++);\
}
#define GetLexCompacted(xxx,dd) {\
	xxx = *((unsigned char*)dd++);\
	if (((unsigned)xxx)>=128) {\
		unsigned yyy = *((unsigned char*)dd++);\
        if (yyy >= 128) {\
			xxx = ((unsigned)xxx)-128 + 128 * (yyy-128) + 16384 * *((unsigned char*)dd++);\
		} else {\
			xxx = ((unsigned)xxx)-128 + 128 * yyy;\
		}\
	}\
}
#define GetLexInt(xxx,dd) {\
	xxx = *((unsigned char*)dd++);\
	xxx += 256 * *((unsigned char*)dd++);\
	xxx += 256 * 256 * *((unsigned char*)dd++);\
	xxx += 256 * 256 * 256 * *((unsigned char*)dd++);\
}
#define GetLexPosition(pos,tmpcc) {\
	GetLexCompacted(pos.file,tmpcc);\
	GetLexCompacted(pos.line,tmpcc);\
	GetLexCompacted(pos.coll,tmpcc);\
}
/*#define GetLexPosition(pos,tmpcc) {}*/


#define NextLexChar(dd) (*((unsigned char*)dd))
#define NextLexToken(dd) (NextLexShort(dd))
#define NextLexShort(dd) (\
	*((unsigned char*)dd) \
	+ 256 * *(((unsigned char*)dd)+1)\
)
#define NextLexInt(dd) (\
	*((unsigned char*)dd) \
	+ 256 * *(((unsigned char*)dd)+1)\
	+ 256 * 256 * *(((unsigned char*)dd)+2) \
	+ 256 * 256 * 256 * *(((unsigned char*)dd)+3)\
)

#define NextLexPosition(pos,tmpcc) {\
	char *tmptmpcc = tmpcc;\
	GetLexPosition(pos, tmptmpcc);\
}






#endif	  			/*SBD*/


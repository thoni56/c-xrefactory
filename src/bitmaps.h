/*
	$Date: 2002/02/13 21:20:27 $
*/


#ifndef __BITMAPS_H
#define __BITMAPS_H

/*  some constants depending on architercture	*/

#ifndef USE_LONG_BITARRAYS
typedef unsigned char bitArray;
#define BIT_ARR_NBITS 8			/* number of bits */
#define BIT_ARR_NBITSLOG 3		/* log_2(BIT_ARR_NBITS) */
#else
typedef unsigned bitArray;
#define BIT_ARR_NBITS 32			/* number of bits */
#define BIT_ARR_NBITSLOG 5		/* log_2(BIT_ARR_NBITS) */
#endif



/* auxiliary macros */
#define BIT_ARR_NBITS1 (BIT_ARR_NBITS-1)	    /* BIT_ARR_NBITS-1 */
#define MODMSK (BIT_ARR_NBITS1)
#define DIVMSK ~(MODMSK)

#define BIT_ARR_DIVNBITS(n) (((n) & DIVMSK)>> BIT_ARR_NBITSLOG)

#define BIT_ARR_N_TH_BIT(n) (1<<((n) & MODMSK))


/* main macros    */

#define BIT_ARR_DIM(nn) (((nn)+BIT_ARR_NBITS1)/BIT_ARR_NBITS)

#define THEBIT(bitarr,s) ((bitarr[BIT_ARR_DIVNBITS(s)] & BIT_ARR_N_TH_BIT(s))!=0)

#define SETBIT(bitarr,s) {bitarr[BIT_ARR_DIVNBITS(s)] |= BIT_ARR_N_TH_BIT(s);}

#define NULLBIT(bitarr,s) {bitarr[BIT_ARR_DIVNBITS(s)] &= ~(BIT_ARR_N_TH_BIT(s));}


// extern void bitmapdump(bitArray *m, int len, char *name);

#endif






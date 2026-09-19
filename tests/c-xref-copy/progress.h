#ifndef PROGRESS_H_INCLUDED
#define PROGRESS_H_INCLUDED

extern int progressFactor;
extern int progressOffset;

extern void writeRelativeProgress(int progress); /* Take care with integer division when calculation percentage */

#endif

#ifndef PROGRESS_H_INCLUDED
#define PROGRESS_H_INCLUDED

extern int progressFactor;
extern int progressOffset;

extern void initProgress(const char *format);
extern void writeProgressInformation(int value);
extern void writeRelativeProgress(int progress);

#endif

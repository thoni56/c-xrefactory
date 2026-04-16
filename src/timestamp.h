#ifndef TIMESTAMP_H_INCLUDED
#define TIMESTAMP_H_INCLUDED

#include <time.h>
#include <stdbool.h>

typedef struct timespec FileTimestamp;

#define ZERO_TIMESTAMP ((FileTimestamp){0, 0})

extern bool fileTimestampsEqual(FileTimestamp a, FileTimestamp b);
extern bool fileTimestampIsZero(FileTimestamp ts);
extern bool fileTimestampIsLessThan(FileTimestamp a, FileTimestamp b);
extern FileTimestamp fileTimestampNow(void);
extern long fileTimestampSeconds(FileTimestamp ts);
extern long fileTimestampNanoseconds(FileTimestamp ts);
extern FileTimestamp makeFileTimestamp(long seconds, long nanoseconds);

#endif

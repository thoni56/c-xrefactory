#ifndef TIMESTAMP_H_INCLUDED
#define TIMESTAMP_H_INCLUDED

#include <time.h>
#include <stdbool.h>

typedef time_t FileTimestamp;

extern bool fileTimestampsEqual(FileTimestamp a, FileTimestamp b);
extern FileTimestamp fileTimestampNow(void);
extern long fileTimestampSeconds(FileTimestamp ts);
extern long fileTimestampNanoseconds(FileTimestamp ts);
extern FileTimestamp makeFileTimestamp(long seconds, long nanoseconds);

#endif

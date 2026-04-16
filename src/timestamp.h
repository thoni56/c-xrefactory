#ifndef TIMESTAMP_H_INCLUDED
#define TIMESTAMP_H_INCLUDED

#include <time.h>
#include <stdbool.h>

typedef time_t FileTimestamp;

extern bool fileTimestampsEqual(FileTimestamp a, FileTimestamp b);
extern FileTimestamp fileTimestampNow(void);

#endif

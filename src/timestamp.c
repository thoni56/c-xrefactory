#include "timestamp.h"

bool fileTimestampsEqual(FileTimestamp a, FileTimestamp b) {
    return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

bool fileTimestampIsZero(FileTimestamp ts) {
    return ts.tv_sec == 0 && ts.tv_nsec == 0;
}

bool fileTimestampIsLessThan(FileTimestamp a, FileTimestamp b) {
    if (a.tv_sec != b.tv_sec)
        return a.tv_sec < b.tv_sec;
    return a.tv_nsec < b.tv_nsec;
}

FileTimestamp fileTimestampNow(void) {
    FileTimestamp ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
}

long fileTimestampSeconds(FileTimestamp ts) {
    return (long)ts.tv_sec;
}

long fileTimestampNanoseconds(FileTimestamp ts) {
    return ts.tv_nsec;
}

FileTimestamp makeFileTimestamp(long seconds, long nanoseconds) {
    return (FileTimestamp){ .tv_sec = seconds, .tv_nsec = nanoseconds };
}

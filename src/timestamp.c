#include "timestamp.h"

bool fileTimestampsEqual(FileTimestamp a, FileTimestamp b) {
    return a == b;
}

FileTimestamp fileTimestampNow(void) {
    return time(NULL);
}

long fileTimestampSeconds(FileTimestamp ts) {
    return (long)ts;
}

long fileTimestampNanoseconds(FileTimestamp ts) {
    (void)ts;
    return 0;
}

FileTimestamp makeFileTimestamp(long seconds, long nanoseconds) {
    (void)nanoseconds;
    return (FileTimestamp)seconds;
}

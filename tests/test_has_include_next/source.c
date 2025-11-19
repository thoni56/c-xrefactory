// Test __has_include_next() preprocessor operator

#if __has_include_next(<stdio.h>)
#define HAVE_STDIO_NEXT 1
#endif

#if __has_include_next("nonexistent.h")
#define HAVE_NONEXISTENT_NEXT 1
#endif

#if __has_include_next(<nonexistent_system.h>)
#define HAVE_NONEXISTENT_SYSTEM_NEXT 1
#endif

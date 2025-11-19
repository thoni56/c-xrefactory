// Test __has_include() preprocessor operator

#if __has_include(<stdio.h>)
#define HAVE_STDIO 1
#endif

#if __has_include("nonexistent.h")
#define HAVE_NONEXISTENT 1
#endif

#if __has_include(<nonexistent_system.h>)
#define HAVE_NONEXISTENT_SYSTEM 1
#endif

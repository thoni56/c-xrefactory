typedef int CgreenTest;

#define spec_name(contextName, testName) CgreenSpec__##contextName##__##testName##__
#define EnsureWithContextAndSpecificationName(skip, contextName, specName)                              \
    static void contextName##__##specName(void);                                                        \
    CgreenTest spec_name(contextName, specName) = {skip,                                                \
                                                   &contextFor##contextName,                            \
                                                   STRINGIFY_TOKEN(specName),                           \
                                                   &contextName##__##specName,                          \
                                                   FILENAME,                                            \
                                                   __LINE__};                                           \
    static void contextName##__##specName(void)


#define EnsureWithSpecificationName(skip, specName)                                                     \
    static void specName(void);                                                                         \
    CgreenTest spec_name(default, specName) = {skip,      &defaultContext, STRINGIFY_TOKEN(specName),   \
                                               &specName, FILENAME,        __LINE__};                   \
    static void specName(void)

#define ENSURE_VA_NUM_ARGS(...) ENSURE_VA_NUM_ARGS_IMPL_((__VA_ARGS__, _CALLED_WITH_TOO_MANY_ARGUMENTS,  WithContextAndSpecificationName,  WithSpecificationName, DummyToFillVaArgs))
#define ENSURE_VA_NUM_ARGS_IMPL_(tuple) ENSURE_VA_NUM_ARGS_IMPL tuple

#define ENSURE_VA_NUM_ARGS_IMPL(_1, _2, _3, _4, N, ...) N

// As long as the following #define is present we get a segv...

// these levels of indirecton are a work-around for variadic macro deficiencies in Visual C++ 2012 and prior
#define ENSURE_macro_dispatcher_(func, nargs)            func ## nargs

#define ENSURE_macro_dispatcher(func, ...)   ENSURE_macro_dispatcher_(func, ENSURE_VA_NUM_ARGS(__VA_ARGS__))

#define Ensure_NARG(...) ENSURE_macro_dispatcher(Ensure, __VA_ARGS__)
#define Ensure(...) Ensure_NARG(0, __VA_ARGS__)(0, __VA_ARGS__)

//Ensure(Test, expansion) {}
//Ensure_NARG(0, Test, expansion)(0, Test, expansion) {1;}
ENSURE_macro_dispatcher_(func, nargs)

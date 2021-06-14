#ifndef USAGE_H_INCLUDED
#define USAGE_H_INCLUDED

/* Because of the macro magic we can't comment near the actual values
   so here are some descriptions of some of the Usage values
   (duplication so remember to change in here too):

    UsageLvalUsed                       - == USAGE_EXTEND_USAGE
    UsageLastUselessInClassOrMethod     - last useless part of useless FQT name in
                                          current method (only in regime server)
    UsageConstructorUsed                - can be put anywhere before 'Used'
    UsageMaybeThisInClassOrMethod       - reference inside current method
    UsageAddrUsed                       - == USAGE_TOP_LEVEL_USED
    UsageMaybeQualifThisInClassOrMethod - where 'this' may be inserted
    UsageLastUseless                    - last part of useless FQT name
    UsageMethodInvokedViaSuper          - a method invoked by super.method(...)
    UsageConstructorDefinition          - Usage for Java type name, when used in constructor def.
    UsageOtherUseless                   - a useless part of useless FQT name (not last)
    UsageThisUsage                      - 'this' reference
    UsageFork                           - only in program graph for branching on label
    UsageNone                           - also forgotten rval usage of lval usage ex. l=3;
                                          also new Nested() resolved to error because of
                                          enclosing instance
    UsageMacroBaseFileUsage             - reference to input file expanding the macro
    UsageClassFileDefinition            - reference got from class file (not shown in searches)
    UsageJavaDoc                        - reference to place javadoc link
    UsageJavaDocFullEntry               - reference to placce full javadoc comment
    UsageClassTreeDefinition            - reference for class tree symbol
    UsageMaybeThis                      - reference where 'this' maybe inserted
    UsageMaybeQualifiedThis             - reference where qualified 'this' may be inserted
    UsageThrown                         - extract method exception information
    UsageCatched                        - extract method exception information

 */

/* Some CPP magic to be able to print enums as strings: */
#define GENERATE_ENUM_VALUE(ENUM) ENUM,
#define GENERATE_ENUM_STRING(STRING) #STRING,


// !!!!!!!!!!!!! All this stuff is to be removed, new way of defining usages
// !!!!!!!!!!!!! is to set various bits in usg structure
/* TODO: well, they are still used, so there must be more to this... */

#define ALL_USAGE_ENUMS(ENUM)                   \
/* filter3  == all filters */                   \
    ENUM(ureserve0)                             \
    ENUM(UsageOLBestFitDefined)                 \
    ENUM(UsageJavaNativeDeclared)               \
    ENUM(ureserve1)                             \
    ENUM(UsageDefined)                          \
    ENUM(ureserve2)                             \
    ENUM(UsageDeclared)                         \
    ENUM(ureserve3)                             \
/* filter2 */                                   \
    ENUM(UsageLvalUsed)                         \
    ENUM(UsageLastUselessInClassOrMethod)       \
    ENUM(ureserve4)                             \
/* filter1 */                                   \
    ENUM(UsageAddrUsed)                         \
    ENUM(ureserve5)                             \
    ENUM(UsageConstructorUsed)                  \
    ENUM(ureserve6)                             \
    ENUM(UsageMaybeThisInClassOrMethod)         \
    ENUM(UsageMaybeQualifThisInClassOrMethod)   \
    ENUM(UsageNotFQTypeInClassOrMethod)         \
    ENUM(UsageNotFQFieldInClassOrMethod)        \
    ENUM(UsageNonExpandableNotFQTNameInClassOrMethod)   \
    ENUM(UsageLastUseless)                      \
    ENUM(ureserve7)                             \
/* filter0 */                                   \
    ENUM(UsageUsed)                             \
    ENUM(UsageUndefinedMacro)                   \
    ENUM(UsageMethodInvokedViaSuper)            \
    ENUM(UsageConstructorDefinition)            \
    ENUM(ureserve8)                             \
    ENUM(UsageOtherUseless)                     \
    ENUM(UsageThisUsage)                        \
    ENUM(ureserve9)                             \
    ENUM(UsageFork)                             \
    ENUM(ureserve10)                            \
    ENUM(ureserve11)                            \
/* INVISIBLE USAGES */                          \
    ENUM(UsageMaxOLUsages)                      \
    ENUM(UsageNone)                             \
    ENUM(UsageMacroBaseFileUsage)               \
    ENUM(UsageClassFileDefinition)              \
    ENUM(UsageJavaDoc)                          \
    ENUM(UsageJavaDocFullEntry)                 \
    ENUM(UsageClassTreeDefinition)              \
    ENUM(UsageMaybeThis)                        \
    ENUM(UsageMaybeQualifiedThis)               \
    ENUM(UsageThrown)                           \
    ENUM(UsageCatched)                          \
    ENUM(UsageTryCatchBegin)                    \
    ENUM(UsageTryCatchEnd)                      \
    ENUM(UsageSuperMethod)                      \
    ENUM(UsageNotFQType)                        \
    ENUM(UsageNotFQField)                       \
    ENUM(UsageNonExpandableNotFQTName)          \
    ENUM(MAX_USAGES)                            \
    ENUM(USAGE_ANY)                             \
    ENUM(USAGE_FILTER)


typedef enum usage {
    ALL_USAGE_ENUMS(GENERATE_ENUM_VALUE)
} Usage;


extern const char *usageEnumName[];

#endif

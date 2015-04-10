#define HAVE_MEMMOVE 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

#define LINK_SIZE 2
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_RECURSION MATCH_LIMIT
#define MAX_NAME_COUNT 10000
#define MAX_NAME_SIZE 32
#define NEWLINE 10
#define PARENS_NEST_LIMIT 250

#define POSIX_MALLOC_THRESHOLD 10
#define SUPPORT_UCP
#define SUPPORT_UTF16

/*
    man 3 pcrejit for a list of supported platforms;
    as PCRE 8.30, stable JIT support is available for:
    - ARM v5, v7, and Thumb2 (__GNUC__ compilers only)
    - x86/x86-64
    - MIPS 32bit (__GNUC__ compilers only)
*/
#if !defined(PCRE_DISABLE_JIT) && (\
    /* ARM */ \
    (defined(__GNUC__) && (defined(__arm__) || defined(__TARGET_ARCH_ARM))) \
    /* x86 32/64 */ \
    || defined(__i386) || defined(__i386__) || defined(_M_IX86) \
    || defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64) \
    /* MIPS32 */ \
    || (defined(__GNUC__) \
       && (defined(__mips) || defined(__mips__)) \
       && !(defined(_MIPS_ARCH_MIPS64) || defined(__mips64))))
#  define SUPPORT_JIT
#endif

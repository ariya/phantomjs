/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef UTILS_H
#define UTILS_H 1

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "darray.h"

/*
 * We sometimes malloc strings and then expose them as const char*'s. This
 * macro is used when we free these strings in order to avoid -Wcast-qual
 * errors.
 */
#define UNCONSTIFY(const_ptr)  ((void *) (uintptr_t) (const_ptr))

static inline bool
streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

static inline bool
streq_not_null(const char *s1, const char *s2)
{
    if (!s1 || !s2)
        return false;
    return streq(s1, s2);
}

static inline bool
istreq(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2) == 0;
}

static inline bool
istreq_prefix(const char *s1, const char *s2)
{
    return strncasecmp(s1, s2, strlen(s1)) == 0;
}

static inline char *
strdup_safe(const char *s)
{
    return s ? strdup(s) : NULL;
}

static inline size_t
strlen_safe(const char *s)
{
    return s ? strlen(s) : 0;
}

static inline bool
isempty(const char *s)
{
    return s == NULL || s[0] == '\0';
}

static inline const char *
strnull(const char *s)
{
    return s ? s : "(null)";
}

static inline const char *
strempty(const char *s)
{
    return s ? s : "";
}

static inline void *
memdup(const void *mem, size_t nmemb, size_t size)
{
    void *p = malloc(nmemb * size);
    if (p)
        memcpy(p, mem, nmemb * size);
    return p;
}

static inline int
min(int misc, int other)
{
    return (misc < other) ? misc : other;
}

static inline int
max(int misc, int other)
{
    return (misc > other) ? misc : other;
}

/* ctype.h is locale-dependent and has other oddities. */
static inline bool
is_space(char ch)
{
    return ch == ' ' || (ch >= '\t' && ch <= '\r');
}

static inline bool
is_alpha(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static inline bool
is_digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static inline bool
is_alnum(char ch)
{
    return is_alpha(ch) || is_digit(ch);
}

static inline bool
is_xdigit(char ch)
{
    return
        (ch >= '0' && ch <= '9') ||
        (ch >= 'a' && ch <= 'f') ||
        (ch >= 'A' && ch <= 'F');
}

static inline bool
is_graph(char ch)
{
    /* See table in ascii(7). */
    return ch >= '!' && ch <= '~';
}

/*
 * Return the bit position of the most significant bit.
 * Note: this is 1-based! It's more useful this way, and returns 0 when
 * mask is all 0s.
 */
static inline unsigned
msb_pos(uint32_t mask)
{
    unsigned pos = 0;
    while (mask) {
        pos++;
        mask >>= 1u;
    }
    return pos;
}

bool
map_file(FILE *file, const char **string_out, size_t *size_out);

void
unmap_file(const char *str, size_t size);

#define ARRAY_SIZE(arr) ((sizeof(arr) / sizeof(*(arr))))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a, b, c) MIN(MIN((a), (b)), (c))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) MAX(MAX((a), (b)), (c))

#if defined(HAVE_SECURE_GETENV)
# define secure_getenv secure_getenv
#elif defined(HAVE___SECURE_GETENV)
# define secure_getenv __secure_getenv
#else
# define secure_getenv getenv
#endif

#if defined(HAVE___BUILTIN_EXPECT)
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

/* Compiler Attributes */

#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__CYGWIN__)
# define XKB_EXPORT      __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
# define XKB_EXPORT      __global
#else /* not gcc >= 4 and not Sun Studio >= 8 */
# define XKB_EXPORT
#endif

#if defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 203)
# define ATTR_PRINTF(x,y) __attribute__((__format__(__printf__, x, y)))
#else /* not gcc >= 2.3 */
# define ATTR_PRINTF(x,y)
#endif

#if (defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 205)) \
    || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
# define ATTR_NORETURN __attribute__((__noreturn__))
#else
# define ATTR_NORETURN
#endif /* GNUC  */

#if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 296)
#define ATTR_MALLOC  __attribute__((__malloc__))
#else
#define ATTR_MALLOC
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define ATTR_NULL_SENTINEL __attribute__((__sentinel__))
#else
# define ATTR_NULL_SENTINEL
#endif /* GNUC >= 4 */

#if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 295)
#define ATTR_PACKED  __attribute__((__packed__))
#else
#define ATTR_PACKED
#endif

#endif /* UTILS_H */

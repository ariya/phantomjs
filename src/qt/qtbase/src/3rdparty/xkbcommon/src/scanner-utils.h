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

#ifndef XKBCOMP_SCANNER_UTILS_H
#define XKBCOMP_SCANNER_UTILS_H

/* Point to some substring in the file; used to avoid copying. */
struct sval {
    const char *start;
    unsigned int len;
};
typedef darray(struct sval) darray_sval;

static inline bool
svaleq(struct sval s1, struct sval s2)
{
    return s1.len == s2.len && strncmp(s1.start, s2.start, s1.len) == 0;
}

static inline bool
svaleq_prefix(struct sval s1, struct sval s2)
{
    return s1.len <= s2.len && strncmp(s1.start, s2.start, s1.len) == 0;
}

struct scanner {
    const char *s;
    size_t pos;
    size_t len;
    char buf[1024];
    size_t buf_pos;
    unsigned line, column;
    /* The line/column of the start of the current token. */
    unsigned token_line, token_column;
    const char *file_name;
    struct xkb_context *ctx;
};

#define scanner_log(scanner, level, fmt, ...) \
    xkb_log((scanner)->ctx, (level), 0, \
            "%s:%u:%u: " fmt "\n", \
             (scanner)->file_name, \
             (scanner)->token_line, (scanner)->token_column, ##__VA_ARGS__)

#define scanner_err(scanner, fmt, ...) \
    scanner_log(scanner, XKB_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define scanner_warn(scanner, fmt, ...) \
    scanner_log(scanner, XKB_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)

static inline void
scanner_init(struct scanner *s, struct xkb_context *ctx,
             const char *string, size_t len, const char *file_name)
{
    s->s = string;
    s->len = len;
    s->pos = 0;
    s->line = s->column = 1;
    s->token_line = s->token_column = 1;
    s->file_name = file_name;
    s->ctx = ctx;
}

static inline char
peek(struct scanner *s)
{
    if (unlikely(s->pos >= s->len))
        return '\0';
    return s->s[s->pos];
}

static inline bool
eof(struct scanner *s)
{
    return s->pos >= s->len;
}

static inline bool
eol(struct scanner *s)
{
    return peek(s) == '\n';
}

static inline char
next(struct scanner *s)
{
    if (unlikely(eof(s)))
        return '\0';
    if (unlikely(eol(s))) {
        s->line++;
        s->column = 1;
    }
    else {
        s->column++;
    }
    return s->s[s->pos++];
}

static inline bool
chr(struct scanner *s, char ch)
{
    if (likely(peek(s) != ch))
        return false;
    s->pos++; s->column++;
    return true;
}

static inline bool
str(struct scanner *s, const char *string, size_t len)
{
    if (s->len - s->pos < len)
        return false;
    if (strncasecmp(s->s + s->pos, string, len) != 0)
        return false;
    s->pos += len; s->column += len;
    return true;
}

#define lit(s, literal) str(s, literal, sizeof(literal) - 1)

static inline bool
buf_append(struct scanner *s, char ch)
{
    if (s->buf_pos + 1 >= sizeof(s->buf))
        return false;
    s->buf[s->buf_pos++] = ch;
    return true;
}

static inline bool
oct(struct scanner *s, uint8_t *out)
{
    int i;
    for (i = 0, *out = 0; peek(s) >= '0' && peek(s) <= '7' && i < 3; i++)
        *out = *out * 8 + next(s) - '0';
    return i > 0;
}

#endif

/*
 * Copyright © 2012 Intel Corporation
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
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "atom.h"

struct xkb_context {
    int refcnt;

    ATTR_PRINTF(3, 0) void (*log_fn)(struct xkb_context *ctx,
                                     enum xkb_log_level level,
                                     const char *fmt, va_list args);
    enum xkb_log_level log_level;
    int log_verbosity;
    void *user_data;

    struct xkb_rule_names names_dflt;

    darray(char *) includes;
    darray(char *) failed_includes;

    struct atom_table *atom_table;

    /* Buffer for the *Text() functions. */
    char text_buffer[2048];
    size_t text_next;

    unsigned int use_environment_names : 1;
};

unsigned int
xkb_context_num_failed_include_paths(struct xkb_context *ctx);

const char *
xkb_context_failed_include_path_get(struct xkb_context *ctx,
                                    unsigned int idx);

/*
 * Returns XKB_ATOM_NONE if @string was not previously interned,
 * otherwise returns the atom.
 */
xkb_atom_t
xkb_atom_lookup(struct xkb_context *ctx, const char *string);

xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string, size_t len);

#define xkb_atom_intern_literal(ctx, literal) \
    xkb_atom_intern((ctx), (literal), sizeof(literal) - 1)

/**
 * If @string is dynamically allocated, NUL-terminated, free'd immediately
 * after being interned, and not used afterwards, use this function
 * instead of xkb_atom_intern to avoid some unnecessary allocations.
 * The caller should not use or free the passed in string afterwards.
 */
xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string);

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom);

char *
xkb_context_get_buffer(struct xkb_context *ctx, size_t size);

ATTR_PRINTF(4, 5) void
xkb_log(struct xkb_context *ctx, enum xkb_log_level level, int verbosity,
        const char *fmt, ...);

void
xkb_context_sanitize_rule_names(struct xkb_context *ctx,
                                struct xkb_rule_names *rmlvo);

/*
 * The format is not part of the argument list in order to avoid the
 * "ISO C99 requires rest arguments to be used" warning when only the
 * format is supplied without arguments. Not supplying it would still
 * result in an error, though.
 */
#define log_dbg(ctx, ...) \
    xkb_log((ctx), XKB_LOG_LEVEL_DEBUG, 0, __VA_ARGS__)
#define log_info(ctx, ...) \
    xkb_log((ctx), XKB_LOG_LEVEL_INFO, 0, __VA_ARGS__)
#define log_warn(ctx, ...) \
    xkb_log((ctx), XKB_LOG_LEVEL_WARNING, 0,  __VA_ARGS__)
#define log_err(ctx, ...) \
    xkb_log((ctx), XKB_LOG_LEVEL_ERROR, 0,  __VA_ARGS__)
#define log_wsgo(ctx, ...) \
    xkb_log((ctx), XKB_LOG_LEVEL_CRITICAL, 0, __VA_ARGS__)
#define log_vrb(ctx, vrb, ...) \
    xkb_log((ctx), XKB_LOG_LEVEL_WARNING, (vrb), __VA_ARGS__)

/*
 * Variants which are prefixed by the name of the function they're
 * called from.
 * Here we must have the silly 1 variant.
 */
#define log_err_func(ctx, fmt, ...) \
    log_err(ctx, "%s: " fmt, __func__, __VA_ARGS__)
#define log_err_func1(ctx, fmt) \
    log_err(ctx, "%s: " fmt, __func__)

#endif

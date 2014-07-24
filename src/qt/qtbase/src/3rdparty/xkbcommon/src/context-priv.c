/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita
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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "context.h"

unsigned int
xkb_context_num_failed_include_paths(struct xkb_context *ctx)
{
    return darray_size(ctx->failed_includes);
}

const char *
xkb_context_failed_include_path_get(struct xkb_context *ctx,
                                    unsigned int idx)
{
    if (idx >= xkb_context_num_failed_include_paths(ctx))
        return NULL;

    return darray_item(ctx->failed_includes, idx);
}

xkb_atom_t
xkb_atom_lookup(struct xkb_context *ctx, const char *string)
{
    return atom_lookup(ctx->atom_table, string, strlen(string));
}

xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string, size_t len)
{
    return atom_intern(ctx->atom_table, string, len, false);
}

xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string)
{
    return atom_intern(ctx->atom_table, string, strlen(string), true);
}

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom)
{
    return atom_text(ctx->atom_table, atom);
}

void
xkb_log(struct xkb_context *ctx, enum xkb_log_level level, int verbosity,
        const char *fmt, ...)
{
    va_list args;

    if (ctx->log_level < level || ctx->log_verbosity < verbosity)
        return;

    va_start(args, fmt);
    ctx->log_fn(ctx, level, fmt, args);
    va_end(args);
}

char *
xkb_context_get_buffer(struct xkb_context *ctx, size_t size)
{
    char *rtrn;

    if (size >= sizeof(ctx->text_buffer))
        return NULL;

    if (sizeof(ctx->text_buffer) - ctx->text_next <= size)
        ctx->text_next = 0;

    rtrn = &ctx->text_buffer[ctx->text_next];
    ctx->text_next += size;

    return rtrn;
}

#ifndef DEFAULT_XKB_VARIANT
#define DEFAULT_XKB_VARIANT NULL
#endif

#ifndef DEFAULT_XKB_OPTIONS
#define DEFAULT_XKB_OPTIONS NULL
#endif

static const char *
xkb_context_get_default_rules(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = secure_getenv("XKB_DEFAULT_RULES");

    return env ? env : DEFAULT_XKB_RULES;
}

static const char *
xkb_context_get_default_model(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = secure_getenv("XKB_DEFAULT_MODEL");

    return env ? env : DEFAULT_XKB_MODEL;
}

static const char *
xkb_context_get_default_layout(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = secure_getenv("XKB_DEFAULT_LAYOUT");

    return env ? env : DEFAULT_XKB_LAYOUT;
}

static const char *
xkb_context_get_default_variant(struct xkb_context *ctx)
{
    const char *env = NULL;
    const char *layout = secure_getenv("XKB_DEFAULT_LAYOUT");

    /* We don't want to inherit the variant if they haven't also set a
     * layout, since they're so closely paired. */
    if (layout && ctx->use_environment_names)
        env = secure_getenv("XKB_DEFAULT_VARIANT");

    return env ? env : DEFAULT_XKB_VARIANT;
}

static const char *
xkb_context_get_default_options(struct xkb_context *ctx)
{
    const char *env = NULL;

    if (ctx->use_environment_names)
        env = secure_getenv("XKB_DEFAULT_OPTIONS");

    return env ? env : DEFAULT_XKB_OPTIONS;
}

void
xkb_context_sanitize_rule_names(struct xkb_context *ctx,
                                struct xkb_rule_names *rmlvo)
{
    if (isempty(rmlvo->rules))
        rmlvo->rules = xkb_context_get_default_rules(ctx);
    if (isempty(rmlvo->model))
        rmlvo->model = xkb_context_get_default_model(ctx);
    /* Layout and variant are tied together, so don't try to use one from
     * the caller and one from the environment. */
    if (isempty(rmlvo->layout)) {
        rmlvo->layout = xkb_context_get_default_layout(ctx);
        rmlvo->variant = xkb_context_get_default_variant(ctx);
    }
    /* Options can be empty, so respect that if passed in. */
    if (rmlvo->options == NULL)
        rmlvo->options = xkb_context_get_default_options(ctx);
}

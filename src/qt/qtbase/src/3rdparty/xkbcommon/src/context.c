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

/**
 * Append one directory to the context's include path.
 */
XKB_EXPORT int
xkb_context_include_path_append(struct xkb_context *ctx, const char *path)
{
    struct stat stat_buf;
    int err;
    char *tmp;

    tmp = strdup(path);
    if (!tmp)
        goto err;

    err = stat(path, &stat_buf);
    if (err != 0)
        goto err;
    if (!S_ISDIR(stat_buf.st_mode))
        goto err;

#if defined(HAVE_EACCESS)
    if (eaccess(path, R_OK | X_OK) != 0)
        goto err;
#elif defined(HAVE_EUIDACCESS)
    if (euidaccess(path, R_OK | X_OK) != 0)
        goto err;
#endif

    darray_append(ctx->includes, tmp);
    return 1;

err:
    darray_append(ctx->failed_includes, tmp);
    return 0;
}

/**
 * Append the default include directories to the context.
 */
XKB_EXPORT int
xkb_context_include_path_append_default(struct xkb_context *ctx)
{
    const char *home;
    char *user_path;
    int err;
    int ret = 0;

    ret |= xkb_context_include_path_append(ctx, DFLT_XKB_CONFIG_ROOT);

    home = secure_getenv("HOME");
    if (!home)
        return ret;
    err = asprintf(&user_path, "%s/.xkb", home);
    if (err <= 0)
        return ret;
    ret |= xkb_context_include_path_append(ctx, user_path);
    free(user_path);

    return ret;
}

/**
 * Remove all entries in the context's include path.
 */
XKB_EXPORT void
xkb_context_include_path_clear(struct xkb_context *ctx)
{
    char **path;

    darray_foreach(path, ctx->includes)
        free(*path);
    darray_free(ctx->includes);

    darray_foreach(path, ctx->failed_includes)
        free(*path);
    darray_free(ctx->failed_includes);
}

/**
 * xkb_context_include_path_clear() + xkb_context_include_path_append_default()
 */
XKB_EXPORT int
xkb_context_include_path_reset_defaults(struct xkb_context *ctx)
{
    xkb_context_include_path_clear(ctx);
    return xkb_context_include_path_append_default(ctx);
}

/**
 * Returns the number of entries in the context's include path.
 */
XKB_EXPORT unsigned int
xkb_context_num_include_paths(struct xkb_context *ctx)
{
    return darray_size(ctx->includes);
}

/**
 * Returns the given entry in the context's include path, or NULL if an
 * invalid index is passed.
 */
XKB_EXPORT const char *
xkb_context_include_path_get(struct xkb_context *ctx, unsigned int idx)
{
    if (idx >= xkb_context_num_include_paths(ctx))
        return NULL;

    return darray_item(ctx->includes, idx);
}

/**
 * Take a new reference on the context.
 */
XKB_EXPORT struct xkb_context *
xkb_context_ref(struct xkb_context *ctx)
{
    ctx->refcnt++;
    return ctx;
}

/**
 * Drop an existing reference on the context, and free it if the refcnt is
 * now 0.
 */
XKB_EXPORT void
xkb_context_unref(struct xkb_context *ctx)
{
    if (!ctx || --ctx->refcnt > 0)
        return;

    xkb_context_include_path_clear(ctx);
    atom_table_free(ctx->atom_table);
    free(ctx);
}

static const char *
log_level_to_prefix(enum xkb_log_level level)
{
    switch (level) {
    case XKB_LOG_LEVEL_DEBUG:
        return "xkbcommon: DEBUG: ";
    case XKB_LOG_LEVEL_INFO:
        return "xkbcommon: INFO: ";
    case XKB_LOG_LEVEL_WARNING:
        return "xkbcommon: WARNING: ";
    case XKB_LOG_LEVEL_ERROR:
        return "xkbcommon: ERROR: ";
    case XKB_LOG_LEVEL_CRITICAL:
        return "xkbcommon: CRITICAL: ";
    default:
        return NULL;
    }
}

ATTR_PRINTF(3, 0) static void
default_log_fn(struct xkb_context *ctx, enum xkb_log_level level,
               const char *fmt, va_list args)
{
    const char *prefix = log_level_to_prefix(level);

    if (prefix)
        fprintf(stderr, "%s", prefix);
    vfprintf(stderr, fmt, args);
}

static enum xkb_log_level
log_level(const char *level) {
    char *endptr;
    enum xkb_log_level lvl;

    errno = 0;
    lvl = strtol(level, &endptr, 10);
    if (errno == 0 && (endptr[0] == '\0' || is_space(endptr[0])))
        return lvl;
    if (istreq_prefix("crit", level))
        return XKB_LOG_LEVEL_CRITICAL;
    if (istreq_prefix("err", level))
        return XKB_LOG_LEVEL_ERROR;
    if (istreq_prefix("warn", level))
        return XKB_LOG_LEVEL_WARNING;
    if (istreq_prefix("info", level))
        return XKB_LOG_LEVEL_INFO;
    if (istreq_prefix("debug", level) || istreq_prefix("dbg", level))
        return XKB_LOG_LEVEL_DEBUG;

    return XKB_LOG_LEVEL_ERROR;
}

static int
log_verbosity(const char *verbosity) {
    char *endptr;
    int v;

    errno = 0;
    v = strtol(verbosity, &endptr, 10);
    if (errno == 0)
        return v;

    return 0;
}

/**
 * Create a new context.
 */
XKB_EXPORT struct xkb_context *
xkb_context_new(enum xkb_context_flags flags)
{
    const char *env;
    struct xkb_context *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->refcnt = 1;
    ctx->log_fn = default_log_fn;
    ctx->log_level = XKB_LOG_LEVEL_ERROR;
    ctx->log_verbosity = 0;

    /* Environment overwrites defaults. */
    env = secure_getenv("XKB_LOG_LEVEL");
    if (env)
        xkb_context_set_log_level(ctx, log_level(env));

    env = secure_getenv("XKB_LOG_VERBOSITY");
    if (env)
        xkb_context_set_log_verbosity(ctx, log_verbosity(env));

    if (!(flags & XKB_CONTEXT_NO_DEFAULT_INCLUDES) &&
        !xkb_context_include_path_append_default(ctx)) {
        log_err(ctx, "failed to add default include path %s\n",
                DFLT_XKB_CONFIG_ROOT);
        xkb_context_unref(ctx);
        return NULL;
    }

    ctx->use_environment_names = !(flags & XKB_CONTEXT_NO_ENVIRONMENT_NAMES);

    ctx->atom_table = atom_table_new();
    if (!ctx->atom_table) {
        xkb_context_unref(ctx);
        return NULL;
    }

    return ctx;
}

XKB_EXPORT void
xkb_context_set_log_fn(struct xkb_context *ctx,
                       void (*log_fn)(struct xkb_context *ctx,
                                      enum xkb_log_level level,
                                      const char *fmt, va_list args))
{
    ctx->log_fn = (log_fn ? log_fn : default_log_fn);
}

XKB_EXPORT enum xkb_log_level
xkb_context_get_log_level(struct xkb_context *ctx)
{
    return ctx->log_level;
}

XKB_EXPORT void
xkb_context_set_log_level(struct xkb_context *ctx, enum xkb_log_level level)
{
    ctx->log_level = level;
}

XKB_EXPORT int
xkb_context_get_log_verbosity(struct xkb_context *ctx)
{
    return ctx->log_verbosity;
}

XKB_EXPORT void
xkb_context_set_log_verbosity(struct xkb_context *ctx, int verbosity)
{
    ctx->log_verbosity = verbosity;
}

XKB_EXPORT void *
xkb_context_get_user_data(struct xkb_context *ctx)
{
    if (ctx)
        return ctx->user_data;
    return NULL;
}

XKB_EXPORT void
xkb_context_set_user_data(struct xkb_context *ctx, void *user_data)
{
    ctx->user_data = user_data;
}

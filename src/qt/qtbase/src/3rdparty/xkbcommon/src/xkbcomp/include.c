/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

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

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "xkbcomp-priv.h"
#include "include.h"

/**
 * Parse an include statement. Each call returns a file name, along with
 * (possibly) a specific map in the file, an explicit group designator, and
 * the separator from the next file, used to determine the merge mode.
 *
 * @param str_inout Input statement, modified in-place. Should be passed in
 * repeatedly. If str_inout is NULL, the parsing has completed.
 *
 * @param file_rtrn Set to the name of the include file to be used. Combined
 * with an enum xkb_file_type, this determines which file to look for in the
 * include path.
 *
 * @param map_rtrn Set to the string between '(' and ')', if any. This will
 * result in the compilation of a specific named map within the file (e.g.
 * xkb_symbols "basic" { ... }) , as opposed to the default map of the file.
 *
 * @param nextop_rtrn Set to the next operation in the complete statement,
 * which is '\0' if it's the last file or '+' or '|' if there are more.
 * Separating the files with '+' sets the merge mode to MERGE_MODE_OVERRIDE,
 * while '|' sets the merge mode to MERGE_MODE_AUGMENT.
 *
 * @param extra_data Set to the string after ':', if any. Currently the
 * extra data is only used for setting an explicit group index for a symbols
 * file.
 *
 * @return true if parsing was successful, false for an illegal string.
 *
 * Example: "evdev+aliases(qwerty):2"
 *      str_inout = "aliases(qwerty):2"
 *      file_rtrn = "evdev"
 *      map_rtrn = NULL
 *      nextop_retrn = "+"
 *      extra_data = NULL
 *
 * 2nd run with "aliases(qwerty):2"
 *      str_inout = NULL
 *      file_rtrn = "aliases"
 *      map_rtrn = "qwerty"
 *      nextop_retrn = ""
 *      extra_data = "2"
 *
 */
bool
ParseIncludeMap(char **str_inout, char **file_rtrn, char **map_rtrn,
                char *nextop_rtrn, char **extra_data)
{
    char *tmp, *str, *next;

    str = *str_inout;

    /*
     * Find the position in the string where the next file is included,
     * if there is more than one left in the statement.
     */
    next = strpbrk(str, "|+");
    if (next) {
        /* Got more files, this function will be called again. */
        *nextop_rtrn = *next;
        /* Separate the string, for strchr etc. to work on this file only. */
        *next++ = '\0';
    }
    else {
        /* This is the last file in this statement, won't be called again. */
        *nextop_rtrn = '\0';
        next = NULL;
    }

    /*
     * Search for the explicit group designator, if any. If it's there,
     * it goes after the file name and map.
     */
    tmp = strchr(str, ':');
    if (tmp != NULL) {
        *tmp++ = '\0';
        *extra_data = strdup(tmp);
    }
    else {
        *extra_data = NULL;
    }

    /* Look for a map, if any. */
    tmp = strchr(str, '(');
    if (tmp == NULL) {
        /* No map. */
        *file_rtrn = strdup(str);
        *map_rtrn = NULL;
    }
    else if (str[0] == '(') {
        /* Map without file - invalid. */
        free(*extra_data);
        return false;
    }
    else {
        /* Got a map; separate the file and the map for the strdup's. */
        *tmp++ = '\0';
        *file_rtrn = strdup(str);
        str = tmp;
        tmp = strchr(str, ')');
        if (tmp == NULL || tmp[1] != '\0') {
            free(*file_rtrn);
            free(*extra_data);
            return false;
        }
        *tmp++ = '\0';
        *map_rtrn = strdup(str);
    }

    /* Set up the next file for the next call, if any. */
    if (*nextop_rtrn == '\0')
        *str_inout = NULL;
    else if (*nextop_rtrn == '|' || *nextop_rtrn == '+')
        *str_inout = next;
    else
        return false;

    return true;
}

static const char *xkb_file_type_include_dirs[_FILE_TYPE_NUM_ENTRIES] = {
    [FILE_TYPE_KEYCODES] = "keycodes",
    [FILE_TYPE_TYPES] = "types",
    [FILE_TYPE_COMPAT] = "compat",
    [FILE_TYPE_SYMBOLS] = "symbols",
    [FILE_TYPE_GEOMETRY] = "geometry",
    [FILE_TYPE_KEYMAP] = "keymap",
    [FILE_TYPE_RULES] = "rules",
};

/**
 * Return the xkb directory based on the type.
 */
static const char *
DirectoryForInclude(enum xkb_file_type type)
{
    if (type >= _FILE_TYPE_NUM_ENTRIES)
        return "";
    return xkb_file_type_include_dirs[type];
}

FILE *
FindFileInXkbPath(struct xkb_context *ctx, const char *name,
                  enum xkb_file_type type, char **pathRtrn)
{
    unsigned int i;
    FILE *file = NULL;
    char *buf = NULL;
    const char *typeDir;
    size_t buf_size = 0, typeDirLen, name_len;

    typeDir = DirectoryForInclude(type);
    typeDirLen = strlen(typeDir);
    name_len = strlen(name);

    for (i = 0; i < xkb_context_num_include_paths(ctx); i++) {
        size_t new_buf_size = strlen(xkb_context_include_path_get(ctx, i)) +
                              typeDirLen + name_len + 3;
        int ret;
        if (new_buf_size > buf_size) {
            void *buf_new = realloc(buf, new_buf_size);
            if (buf_new) {
                buf_size = new_buf_size;
                buf = buf_new;
            } else {
                log_err(ctx, "Cannot realloc for name (%s/%s/%s)\n",
                        xkb_context_include_path_get(ctx, i), typeDir, name);
                continue;
            }
        }
        ret = snprintf(buf, buf_size, "%s/%s/%s",
                       xkb_context_include_path_get(ctx, i),
                       typeDir, name);
        if (ret < 0) {
            log_err(ctx, "snprintf error (%s/%s/%s)\n",
                    xkb_context_include_path_get(ctx, i), typeDir, name);
            continue;
        }

        file = fopen(buf, "r");
        if (file)
            break;
    }

    if (!file) {
        log_err(ctx, "Couldn't find file \"%s/%s\" in include paths\n",
                typeDir, name);

        if (xkb_context_num_include_paths(ctx) > 0) {
            log_err(ctx, "%d include paths searched:\n",
                    xkb_context_num_include_paths(ctx));
            for (i = 0; i < xkb_context_num_include_paths(ctx); i++)
                log_err(ctx, "\t%s\n",
                        xkb_context_include_path_get(ctx, i));
        }
        else {
            log_err(ctx, "There are no include paths to search\n");
        }

        if (xkb_context_num_failed_include_paths(ctx) > 0) {
            log_err(ctx, "%d include paths could not be added:\n",
                    xkb_context_num_failed_include_paths(ctx));
            for (i = 0; i < xkb_context_num_failed_include_paths(ctx); i++)
                log_err(ctx, "\t%s\n",
                        xkb_context_failed_include_path_get(ctx, i));
        }

        free(buf);
        return NULL;
    }

    if (pathRtrn)
        *pathRtrn = buf;
    else
        free(buf);
    return file;
}

XkbFile *
ProcessIncludeFile(struct xkb_context *ctx, IncludeStmt *stmt,
                   enum xkb_file_type file_type)
{
    FILE *file;
    XkbFile *xkb_file;

    file = FindFileInXkbPath(ctx, stmt->file, file_type, NULL);
    if (!file)
        return false;

    xkb_file = XkbParseFile(ctx, file, stmt->file, stmt->map);
    fclose(file);
    if (!xkb_file) {
        if (stmt->map)
            log_err(ctx, "Couldn't process include statement for '%s(%s)'\n",
                    stmt->file, stmt->map);
        else
            log_err(ctx, "Couldn't process include statement for '%s'\n",
                    stmt->file);
        return NULL;
    }

    if (xkb_file->file_type != file_type) {
        log_err(ctx,
                "Include file of wrong type (expected %s, got %s); "
                "Include file \"%s\" ignored\n",
                xkb_file_type_to_string(file_type),
                xkb_file_type_to_string(xkb_file->file_type), stmt->file);
        FreeXkbFile(xkb_file);
        return NULL;
    }

    /* FIXME: we have to check recursive includes here (or somewhere) */

    return xkb_file;
}

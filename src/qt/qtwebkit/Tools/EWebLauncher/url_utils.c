/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "url_utils.h"

#include <Ecore_File.h>
#include <string.h>
#include <sys/statvfs.h>

Eina_Bool
has_scheme(const char *url)
{
    return !!strstr(url, "://");
}

char *
url_from_user_input(const char *arg)
{
    /* If it is already a URL, return the argument as is. */
    if (has_scheme(arg) || !strcasecmp(arg, "about:blank"))
        return strdup(arg);

    Eina_Strbuf *buf = eina_strbuf_manage_new(eina_file_path_sanitize(arg));

    /* Check if the path exists. */
    if (ecore_file_exists(eina_strbuf_string_get(buf))) {
        /* File exists, convert local path to a URL. */
        eina_strbuf_prepend(buf, "file://");
    } else {
        /* The path does not exist, convert it to a URL by
           prepending http:// scheme:
           www.google.com -> http://www.google.com */
         eina_strbuf_string_free(buf);
         eina_strbuf_append_printf(buf, "http://%s", arg);
    }
    char *url = eina_strbuf_string_steal(buf);
    eina_strbuf_free(buf);

    return url;
}

/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitMimeInfo.h"

#include "WebKitMimeInfoPrivate.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

struct _WebKitMimeInfo {
    _WebKitMimeInfo(const WebCore::MimeClassInfo& mimeInfo)
        : mimeInfo(mimeInfo)
    {
    }

    WebCore::MimeClassInfo mimeInfo;
    CString mimeType;
    CString description;
    GRefPtr<GPtrArray> extensions;

    int referenceCount;
};

G_DEFINE_BOXED_TYPE(WebKitMimeInfo, webkit_mime_info, webkit_mime_info_ref, webkit_mime_info_unref)

WebKitMimeInfo* webkitMimeInfoCreate(const WebCore::MimeClassInfo& mimeInfo)
{
    WebKitMimeInfo* info = g_slice_new(WebKitMimeInfo);
    new (info) WebKitMimeInfo(mimeInfo);
    return info;
}

/**
 * webkit_mime_info_ref:
 * @info: a #WebKitMimeInfo
 *
 * Atomically increments the reference count of @info by one. This
 * function is MT-safe and may be called from any thread.
 *
 * Returns: The passed in #WebKitMimeInfo
 */
WebKitMimeInfo* webkit_mime_info_ref(WebKitMimeInfo* info)
{
    g_atomic_int_inc(&info->referenceCount);
    return info;
}

/**
 * webkit_mime_info_unref:
 * @info: a #WebKitMimeInfo
 *
 * Atomically decrements the reference count of @info by one. If the
 * reference count drops to 0, all memory allocated by the #WebKitMimeInfo is
 * released. This function is MT-safe and may be called from any
 * thread.
 */
void webkit_mime_info_unref(WebKitMimeInfo* info)
{
    if (g_atomic_int_dec_and_test(&info->referenceCount)) {
        info->~WebKitMimeInfo();
        g_slice_free(WebKitMimeInfo, info);
    }
}

/**
 * webkit_mime_info_get_mime_type:
 * @info: a #WebKitMimeInfo
 *
 * Returns: the MIME type of @info
 */
const char* webkit_mime_info_get_mime_type(WebKitMimeInfo* info)
{
    if (!info->mimeType.isNull())
        return info->mimeType.data();

    if (info->mimeInfo.type.isEmpty())
        return 0;

    info->mimeType = info->mimeInfo.type.utf8();
    return info->mimeType.data();
}

/**
 * webkit_mime_info_get_description:
 * @info: a #WebKitMimeInfo
 *
 * Returns: the description of the MIME type of @info
 */
const char* webkit_mime_info_get_description(WebKitMimeInfo* info)
{
    if (!info->description.isNull())
        return info->description.data();

    if (info->mimeInfo.desc.isEmpty())
        return 0;

    info->description = info->mimeInfo.desc.utf8();
    return info->description.data();
}

/**
 * webkit_mime_info_get_extensions:
 * @info: a #WebKitMimeInfo
 *
 * Get the list of file extensions associated to the
 * MIME type of @info
 *
 * Returns: (array zero-terminated=1) (transfer none): a
 *     %NULL-terminated array of strings
 */
const char* const* webkit_mime_info_get_extensions(WebKitMimeInfo* info)
{
    if (info->extensions)
        return reinterpret_cast<gchar**>(info->extensions->pdata);

    if (info->mimeInfo.extensions.isEmpty())
        return 0;

    info->extensions = adoptGRef(g_ptr_array_new_with_free_func(g_free));
    for (size_t i = 0; i < info->mimeInfo.extensions.size(); ++i) {
        if (info->mimeInfo.extensions[i].isEmpty())
            continue;
        g_ptr_array_add(info->extensions.get(), g_strdup(info->mimeInfo.extensions[i].utf8().data()));
    }
    g_ptr_array_add(info->extensions.get(), 0);

    return reinterpret_cast<gchar**>(info->extensions->pdata);
}

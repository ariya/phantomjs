/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef ewk_favicon_database_private_h
#define ewk_favicon_database_private_h

#include "WKRetainPtr.h"
#include "ewk_favicon_database.h"
#include <WebKit2/WKBase.h>
#include <wtf/HashMap.h>

struct IconChangeCallbackData {
    Ewk_Favicon_Database_Icon_Change_Cb callback;
    void* userData;

    IconChangeCallbackData()
        : callback(0)
        , userData(0)
    { }

    IconChangeCallbackData(Ewk_Favicon_Database_Icon_Change_Cb _callback, void* _userData)
        : callback(_callback)
        , userData(_userData)
    { }
};

typedef HashMap<Ewk_Favicon_Database_Icon_Change_Cb, IconChangeCallbackData> ChangeListenerMap;

class EwkFaviconDatabase {
public:
    static PassOwnPtr<EwkFaviconDatabase> create(WKIconDatabaseRef iconDatabase)
    {
        return adoptPtr(new EwkFaviconDatabase(iconDatabase));
    }
    ~EwkFaviconDatabase();

    PassRefPtr<cairo_surface_t> getIconSurfaceSynchronously(const char* pageURL) const;
    void watchChanges(const IconChangeCallbackData& callbackData);
    void unwatchChanges(Ewk_Favicon_Database_Icon_Change_Cb callback);

private:
    explicit EwkFaviconDatabase(WKIconDatabaseRef iconDatabase);

    static void didChangeIconForPageURL(WKIconDatabaseRef iconDatabase, WKURLRef pageURL, const void* clientInfo);
    static void iconDataReadyForPageURL(WKIconDatabaseRef iconDatabase, WKURLRef pageURL, const void* clientInfo);

    WKRetainPtr<WKIconDatabaseRef> m_iconDatabase;
    ChangeListenerMap m_changeListeners;
};

#endif // ewk_favicon_database_private_h

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

#include "config.h"
#include "ewk_favicon_database.h"

#include "WKAPICast.h"
#include "WKIconDatabase.h"
#include "WKIconDatabaseCairo.h"
#include "WKURL.h"
#include "ewk_favicon_database_private.h"
#include <WebCore/CairoUtilitiesEfl.h>
#include <WebCore/RefPtrCairo.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebKit;

EwkFaviconDatabase::EwkFaviconDatabase(WKIconDatabaseRef iconDatabase)
    : m_iconDatabase(iconDatabase)
{
    WKIconDatabaseClient iconDatabaseClient;
    memset(&iconDatabaseClient, 0, sizeof(WKIconDatabaseClient));
    iconDatabaseClient.version = kWKIconDatabaseClientCurrentVersion;
    iconDatabaseClient.clientInfo = this;
    iconDatabaseClient.iconDataReadyForPageURL = iconDataReadyForPageURL;
    WKIconDatabaseSetIconDatabaseClient(m_iconDatabase.get(), &iconDatabaseClient);
}

EwkFaviconDatabase::~EwkFaviconDatabase()
{
    WKIconDatabaseSetIconDatabaseClient(m_iconDatabase.get(), 0);
}

void EwkFaviconDatabase::watchChanges(const IconChangeCallbackData& callbackData)
{
    ASSERT(callbackData.callback);
    if (m_changeListeners.contains(callbackData.callback))
        return;

    m_changeListeners.add(callbackData.callback, callbackData);
}

void EwkFaviconDatabase::unwatchChanges(Ewk_Favicon_Database_Icon_Change_Cb callback)
{
    ASSERT(callback);
    m_changeListeners.remove(callback);
}

void EwkFaviconDatabase::didChangeIconForPageURL(WKIconDatabaseRef, WKURLRef pageURLRef, const void* clientInfo)
{
    const EwkFaviconDatabase* ewkIconDatabase = static_cast<const EwkFaviconDatabase*>(clientInfo);

    if (ewkIconDatabase->m_changeListeners.isEmpty())
        return;

    CString pageURL = toWTFString(pageURLRef).utf8();

    ChangeListenerMap::const_iterator it = ewkIconDatabase->m_changeListeners.begin();
    ChangeListenerMap::const_iterator end = ewkIconDatabase->m_changeListeners.end();
    for (; it != end; ++it)
        it->value.callback(pageURL.data(), it->value.userData);
}

PassRefPtr<cairo_surface_t> EwkFaviconDatabase::getIconSurfaceSynchronously(const char* pageURL) const
{
    WKRetainPtr<WKURLRef> wkPageURL(AdoptWK, WKURLCreateWithUTF8CString(pageURL));

    RefPtr<cairo_surface_t> surface = WKIconDatabaseTryGetCairoSurfaceForURL(m_iconDatabase.get(), wkPageURL.get());
    if (!surface)
        return 0;

    return surface.release();
}

void EwkFaviconDatabase::iconDataReadyForPageURL(WKIconDatabaseRef, WKURLRef pageURL, const void* clientInfo)
{
    EwkFaviconDatabase* ewkIconDatabase = const_cast<EwkFaviconDatabase*>(static_cast<const EwkFaviconDatabase*>(clientInfo));

    WKIconDatabaseRetainIconForURL(ewkIconDatabase->m_iconDatabase.get(), pageURL);

    CString urlString = toWTFString(pageURL).utf8();
    ChangeListenerMap::const_iterator it = ewkIconDatabase->m_changeListeners.begin();
    ChangeListenerMap::const_iterator end = ewkIconDatabase->m_changeListeners.end();
    for (; it != end; ++it)
        it->value.callback(urlString.data(), it->value.userData);
}

Evas_Object* ewk_favicon_database_icon_get(Ewk_Favicon_Database* ewkIconDatabase, const char* pageURL, Evas* evas)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkIconDatabase, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(pageURL, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(evas, 0);

    RefPtr<cairo_surface_t> surface = ewkIconDatabase->getIconSurfaceSynchronously(pageURL);
    if (!surface)
        return 0;

    return WebCore::evasObjectFromCairoImageSurface(evas, surface.get()).leakRef();
}

void ewk_favicon_database_icon_change_callback_add(Ewk_Favicon_Database* ewkIconDatabase, Ewk_Favicon_Database_Icon_Change_Cb callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkIconDatabase);
    EINA_SAFETY_ON_NULL_RETURN(callback);

    ewkIconDatabase->watchChanges(IconChangeCallbackData(callback, userData));
}

void ewk_favicon_database_icon_change_callback_del(Ewk_Favicon_Database* ewkIconDatabase, Ewk_Favicon_Database_Icon_Change_Cb callback)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkIconDatabase);
    EINA_SAFETY_ON_NULL_RETURN(callback);

    ewkIconDatabase->unwatchChanges(callback);
}

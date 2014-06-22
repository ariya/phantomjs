/*
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
#include "ewk_page_group.h"

#include "WKAPICast.h"
#include "WKArray.h"
#include "WKArrayEfl.h"
#include "WKPageGroup.h"
#include "WKString.h"
#include "ewk_page_group_private.h"

using namespace WebKit;

const char EwkPageGroup::defaultIdentifier[] = "defaultPageGroupIdentifier";

typedef HashMap<WKPageGroupRef, EwkPageGroup*> PageGroupMap;

static inline PageGroupMap& pageGroupMap()
{
    DEFINE_STATIC_LOCAL(PageGroupMap, map, ());
    return map;
}

static WKTypeRef convertFromCharToWKString(void* data)
{
    return WKStringCreateWithUTF8CString(static_cast<char*>(data));
}

PassRefPtr<EwkPageGroup> EwkPageGroup::findOrCreateWrapper(WKPageGroupRef pageGroupRef)
{
    if (pageGroupMap().contains(pageGroupRef))
        return pageGroupMap().get(pageGroupRef);

    return adoptRef(new EwkPageGroup(pageGroupRef));
}

PassRefPtr<EwkPageGroup> EwkPageGroup::create(const String& identifier)
{
    WKRetainPtr<WKStringRef> identifierRef = adoptWK(toCopiedAPI(identifier.isEmpty() ? defaultIdentifier : identifier));
    WKRetainPtr<WKPageGroupRef> pageGroupRef = adoptWK(WKPageGroupCreateWithIdentifier(identifierRef.get()));

    return adoptRef(new EwkPageGroup(pageGroupRef.get()));
}

EwkPageGroup::EwkPageGroup(WKPageGroupRef pageGroupRef)
    : m_pageGroupRef(pageGroupRef)
{
    PageGroupMap::AddResult result = pageGroupMap().add(pageGroupRef, this);
    ASSERT_UNUSED(result, result.isNewEntry);
}

EwkPageGroup::~EwkPageGroup()
{
    ASSERT(pageGroupMap().get(m_pageGroupRef.get()) == this);
    pageGroupMap().remove(m_pageGroupRef.get());
}

void EwkPageGroup::addUserStyleSheet(const String& source, const String& baseURL, Eina_List* whiteList, Eina_List* blackList, bool mainFrameOnly)
{
    ASSERT(source);

    WKRetainPtr<WKStringRef> wkSource = adoptWK(toCopiedAPI(source));
    WKRetainPtr<WKURLRef> wkBaseURL = adoptWK(toCopiedURLAPI(baseURL));
    WKRetainPtr<WKArrayRef> wkWhitelist = adoptWK(WKArrayCreateWithEinaList(whiteList, convertFromCharToWKString));
    WKRetainPtr<WKArrayRef> wkBlacklist = adoptWK(WKArrayCreateWithEinaList(blackList, convertFromCharToWKString));
    WKUserContentInjectedFrames injectedFrames = mainFrameOnly ? kWKInjectInTopFrameOnly : kWKInjectInAllFrames;

    WKPageGroupAddUserStyleSheet(m_pageGroupRef.get(), wkSource.get(), wkBaseURL.get(), wkWhitelist.get(), wkBlacklist.get(), injectedFrames);
}

void EwkPageGroup::removeAllUserStyleSheets()
{
    WKPageGroupRemoveAllUserStyleSheets(m_pageGroupRef.get());
}

Ewk_Page_Group* ewk_page_group_create(const char* identifier)
{
    return EwkPageGroup::create(identifier).leakRef();
}

Eina_Bool ewk_page_group_user_style_sheet_add(Ewk_Page_Group* ewkPageGroup, const char* source, const char* baseURL, Eina_List* whiteList, Eina_List* blackList, Eina_Bool mainFrameOnly)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkPageGroup, ewkPageGroup, impl, false);

    impl->addUserStyleSheet(source, baseURL, whiteList, blackList, mainFrameOnly);

    return true;
}

Eina_Bool ewk_page_group_user_style_sheets_remove_all(Ewk_Page_Group* ewkPageGroup)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkPageGroup, ewkPageGroup, impl, false);

    impl->removeAllUserStyleSheets();

    return true;
}

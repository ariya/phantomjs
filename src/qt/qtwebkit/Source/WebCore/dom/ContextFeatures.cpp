/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ContextFeatures.h"

#include "Document.h"
#include "Page.h"
#include "RuntimeEnabledFeatures.h"

namespace WebCore {

ContextFeaturesClient* ContextFeaturesClient::empty()
{
    DEFINE_STATIC_LOCAL(ContextFeaturesClient, empty, ());
    return &empty;
}

const char* ContextFeatures::supplementName()
{
    return "ContextFeatures";
}

ContextFeatures* ContextFeatures::defaultSwitch()
{
    DEFINE_STATIC_LOCAL(RefPtr<ContextFeatures>, instance, (ContextFeatures::create(ContextFeaturesClient::empty())));
    return instance.get();
}

bool ContextFeatures::dialogElementEnabled(Document* document)
{
#if ENABLE(DIALOG_ELEMENT)
    if (!document)
        return RuntimeEnabledFeatures::dialogElementEnabled();
    return document->contextFeatures()->isEnabled(document, DialogElement, RuntimeEnabledFeatures::dialogElementEnabled());
#else
    UNUSED_PARAM(document);
    return false;
#endif
}

bool ContextFeatures::styleScopedEnabled(Document* document)
{
#if ENABLE(STYLE_SCOPED)
    if (!document)
        return RuntimeEnabledFeatures::styleScopedEnabled();
    return document->contextFeatures()->isEnabled(document, StyleScoped, RuntimeEnabledFeatures::styleScopedEnabled());
#else
    UNUSED_PARAM(document);
    return false;
#endif
}

bool ContextFeatures::htmlNotificationsEnabled(Document* document)
{
#if ENABLE(LEGACY_NOTIFICATIONS)
    if (!document)
        return false;
    return document->contextFeatures()->isEnabled(document, HTMLNotifications, false);
#else
    UNUSED_PARAM(document);
    return false;
#endif
}

bool ContextFeatures::mutationEventsEnabled(Document* document)
{
    ASSERT(document);
    if (!document)
        return true;
    return document->contextFeatures()->isEnabled(document, MutationEvents, true);
}

bool ContextFeatures::pushStateEnabled(Document* document)
{
    return document->contextFeatures()->isEnabled(document, PushState, true);
}

void provideContextFeaturesTo(Page* page, ContextFeaturesClient* client)
{
    RefCountedSupplement<Page, ContextFeatures>::provideTo(page, ContextFeatures::supplementName(), ContextFeatures::create(client));
}

void provideContextFeaturesToDocumentFrom(Document* document, Page* page)
{
    ContextFeatures* provided = static_cast<ContextFeatures*>(RefCountedSupplement<Page, ContextFeatures>::from(page, ContextFeatures::supplementName()));
    if (!provided)
        return;
    document->setContextFeatures(provided);
}

}

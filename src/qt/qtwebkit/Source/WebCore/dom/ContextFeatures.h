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

#ifndef ContextFeatures_h
#define ContextFeatures_h

#include "RefCountedSupplement.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class ContextFeaturesClient;
class Document;
class Page;

class ContextFeatures : public RefCountedSupplement<Page, ContextFeatures> {
public:
    enum FeatureType {
        DialogElement = 0,
        StyleScoped,
        HTMLNotifications,
        MutationEvents,
        PushState,
        FeatureTypeSize // Should be the last entry.
    };

    static const char* supplementName();
    static ContextFeatures* defaultSwitch();
    static PassRefPtr<ContextFeatures> create(ContextFeaturesClient*);

    static bool dialogElementEnabled(Document*);
    static bool styleScopedEnabled(Document*);
    static bool htmlNotificationsEnabled(Document*);
    static bool mutationEventsEnabled(Document*);
    static bool pushStateEnabled(Document*);

    bool isEnabled(Document*, FeatureType, bool) const;
    void urlDidChange(Document*);

private:
    explicit ContextFeatures(ContextFeaturesClient* client)
        : m_client(client)
    { }

    virtual void hostDestroyed() OVERRIDE;

    ContextFeaturesClient* m_client;
};

inline void ContextFeatures::hostDestroyed()
{
    m_client = 0;
}


class ContextFeaturesClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static ContextFeaturesClient* empty();

    virtual ~ContextFeaturesClient() { }
    virtual bool isEnabled(Document*, ContextFeatures::FeatureType, bool defaultValue) { return defaultValue; }
    virtual void urlDidChange(Document*) { }
};

void provideContextFeaturesTo(Page*, ContextFeaturesClient*);
void provideContextFeaturesToDocumentFrom(Document*, Page*);

inline PassRefPtr<ContextFeatures> ContextFeatures::create(ContextFeaturesClient* client)
{
    return adoptRef(new ContextFeatures(client));
}

inline bool ContextFeatures::isEnabled(Document* document, FeatureType type, bool defaultValue) const
{
    if (!m_client)
        return defaultValue;
    return m_client->isEnabled(document, type, defaultValue);
}

inline void ContextFeatures::urlDidChange(Document* document)
{
    if (m_client)
        return;
    m_client->urlDidChange(document);
}

} // namespace WebCore

#endif // ContextFeatures_h

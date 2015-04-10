/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#if ENABLE(ENCRYPTED_MEDIA_V2)

#include "CDM.h"

#include "CDMPrivate.h"
#include "MediaKeyError.h"
#include "MediaKeys.h"
#include <wtf/text/WTFString.h>

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#include "CDMPrivateAVFoundation.h"
#endif

namespace WebCore {

struct CDMFactory {
    WTF_MAKE_NONCOPYABLE(CDMFactory); WTF_MAKE_FAST_ALLOCATED;
public:
    CDMFactory(CreateCDM constructor, CDMSupportsKeySystem supportsKeySystem)
        : constructor(constructor)
        , supportsKeySystem(supportsKeySystem)
    {
    }

    CreateCDM constructor;
    CDMSupportsKeySystem supportsKeySystem;
};

static Vector<CDMFactory*>& installedCDMFactories()
{
    DEFINE_STATIC_LOCAL(Vector<CDMFactory*>, cdms, ());
    static bool queriedCDMs = false;
    if (!queriedCDMs) {
        queriedCDMs = true;

        // FIXME: initialize specific UA CDMs. http://webkit.org/b/109318, http://webkit.org/b/109320
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        cdms.append(new CDMFactory(CDMPrivateAVFoundation::create, CDMPrivateAVFoundation::supportsKeySytem));
#endif

    }

    return cdms;
}

void CDM::registerCDMFactory(CreateCDM constructor, CDMSupportsKeySystem supportsKeySystem)
{
    installedCDMFactories().append(new CDMFactory(constructor, supportsKeySystem));
}

static CDMFactory* CDMFactoryForKeySystem(const String& keySystem)
{
    Vector<CDMFactory*>& cdmFactories = installedCDMFactories();
    for (size_t i = 0; i < cdmFactories.size(); ++i) {
        if (cdmFactories[i]->supportsKeySystem(keySystem))
            return cdmFactories[i];
    }
    return 0;
}

bool CDM::supportsKeySystem(const String& keySystem)
{
    return CDMFactoryForKeySystem(keySystem);
}

PassOwnPtr<CDM> CDM::create(const String& keySystem)
{
    if (!supportsKeySystem(keySystem))
        return nullptr;

    return adoptPtr(new CDM(keySystem));
}

CDM::CDM(const String& keySystem)
    : m_keySystem(keySystem)
    , m_client(0)
{
    m_private = CDMFactoryForKeySystem(keySystem)->constructor(this);
}

CDM::~CDM()
{
}

bool CDM::supportsMIMEType(const String& mimeType) const
{
    return m_private->supportsMIMEType(mimeType);
}

PassOwnPtr<CDMSession> CDM::createSession()
{
    return m_private->createSession();
}

MediaPlayer* CDM::mediaPlayer() const
{
    if (!m_client)
        return 0;
    return m_client->cdmMediaPlayer(this);
}

}

#endif // ENABLE(ENCRYPTED_MEDIA_V2)

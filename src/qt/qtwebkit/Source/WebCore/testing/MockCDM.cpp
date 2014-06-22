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
#include "MockCDM.h"

#if ENABLE(ENCRYPTED_MEDIA_V2)

#include "CDM.h"
#include "MediaKeyError.h"
#include <wtf/Uint8Array.h>

namespace WebCore {

class MockCDMSession : public CDMSession {
public:
    static PassOwnPtr<MockCDMSession> create() { return adoptPtr(new MockCDMSession()); }
    virtual ~MockCDMSession() { }

    virtual const String& sessionId() const OVERRIDE { return m_sessionId; }
    virtual PassRefPtr<Uint8Array> generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode) OVERRIDE;
    virtual void releaseKeys() OVERRIDE;
    virtual bool update(Uint8Array*, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode) OVERRIDE;

protected:
    MockCDMSession();

    String m_sessionId;
};

bool MockCDM::supportsKeySytem(const String& keySystem)
{
    return equalIgnoringCase(keySystem, "com.webcore.mock");
}

bool MockCDM::supportsMIMEType(const String& mimeType)
{
    return equalIgnoringCase(mimeType, "video/mock");
}

PassOwnPtr<CDMSession> MockCDM::createSession()
{
    return MockCDMSession::create();
}

static Uint8Array* initDataPrefix()
{
    static const unsigned char prefixData[] = {'m', 'o', 'c', 'k'};
    DEFINE_STATIC_LOCAL(RefPtr<Uint8Array>, prefix, ());
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        prefix = Uint8Array::create(prefixData, sizeof(prefixData) / sizeof(prefixData[0]));
    }
    return prefix.get();
}

static Uint8Array* keyPrefix()
{
    static const unsigned char prefixData[] = {'k', 'e', 'y'};
    DEFINE_STATIC_LOCAL(RefPtr<WTF::Uint8Array>, prefix, ());
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        prefix = Uint8Array::create(prefixData, sizeof(prefixData) / sizeof(prefixData[0]));
    }
    return prefix.get();
}

static Uint8Array* keyRequest()
{
    static const unsigned char requestData[] = {'r', 'e', 'q', 'u', 'e', 's', 't'};
    DEFINE_STATIC_LOCAL(RefPtr<WTF::Uint8Array>, request, ());
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        request = Uint8Array::create(requestData, sizeof(requestData) / sizeof(requestData[0]));
    }
    return request.get();
}

static String generateSessionId()
{
    static int monotonicallyIncreasingSessionId = 0;
    return String::number(monotonicallyIncreasingSessionId++);
}

MockCDMSession::MockCDMSession()
    : m_sessionId(generateSessionId())
{
}

PassRefPtr<Uint8Array> MockCDMSession::generateKeyRequest(const String&, Uint8Array* initData, String&, unsigned short& errorCode, unsigned long&)
{
    for (unsigned i = 0; i < initDataPrefix()->length(); ++i) {
        if (!initData || i >= initData->length() || initData->item(i) != initDataPrefix()->item(i)) {
            errorCode = MediaKeyError::MEDIA_KEYERR_UNKNOWN;
            return 0;
        }
    }
    return keyRequest();
}

void MockCDMSession::releaseKeys()
{
    // no-op
}

bool MockCDMSession::update(Uint8Array* key, RefPtr<Uint8Array>&, unsigned short& errorCode, unsigned long&)
{
    for (unsigned i = 0; i < keyPrefix()->length(); ++i) {
        if (i >= key->length() || key->item(i) != keyPrefix()->item(i)) {
            errorCode = MediaKeyError::MEDIA_KEYERR_CLIENT;
            return false;
        }
    }
    return true;
}

}

#endif // ENABLE(ENCRYPTED_MEDIA_V2)

/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import "config.h"
#import "CDMPrivateAVFoundation.h"

#if ENABLE(ENCRYPTED_MEDIA_V2) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#import "CDM.h"
#import "ExceptionCode.h"
#import "MediaPlayer.h"
#import "MediaPlayerPrivateAVFoundationObjC.h"
#import "SoftLinking.h"
#import "UUID.h"
#import <AVFoundation/AVFoundation.h>
#import <objc/objc-runtime.h>

namespace WebCore {

class CDMSessionAVFoundation : public CDMSession {
public:
    CDMSessionAVFoundation(CDMPrivateAVFoundation* parent);
    virtual ~CDMSessionAVFoundation() { }

    virtual const String& sessionId() const OVERRIDE { return m_sessionId; }
    virtual PassRefPtr<Uint8Array> generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode) OVERRIDE;
    virtual void releaseKeys() OVERRIDE;
    virtual bool update(Uint8Array*, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode) OVERRIDE;

protected:
    CDMPrivateAVFoundation* m_parent;
    String m_sessionId;
    RetainPtr<AVAssetResourceLoadingRequest> m_request;
};

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)
SOFT_LINK_CLASS(AVFoundation, AVURLAsset)
SOFT_LINK_CLASS(AVFoundation, AVAssetResourceLoadingRequest)
#define AVURLAsset getAVURLAssetClass()
#define AVAssetResourceLoadingRequest getAVAssetResourceLoadingRequest()


bool CDMPrivateAVFoundation::supportsKeySytem(const String& keySystem)
{
    return equalIgnoringCase(keySystem, "com.apple.fps") || equalIgnoringCase(keySystem, "com.apple.fps.1_0");
}

bool CDMPrivateAVFoundation::supportsMIMEType(const String& mimeType)
{
    return [AVURLAsset isPlayableExtendedMIMEType:mimeType];
}

PassOwnPtr<CDMSession> CDMPrivateAVFoundation::createSession()
{
    return adoptPtr(new CDMSessionAVFoundation(this));
}

CDMSessionAVFoundation::CDMSessionAVFoundation(CDMPrivateAVFoundation* parent)
    : m_parent(parent)
    , m_sessionId(createCanonicalUUIDString())
{
}

PassRefPtr<Uint8Array> CDMSessionAVFoundation::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode)
{
    UNUSED_PARAM(mimeType);

    MediaPlayer* mediaPlayer = m_parent->cdm()->mediaPlayer();
    if (!mediaPlayer) {
        errorCode = NOT_SUPPORTED_ERR;
        return 0;
    }

    String keyURI;
    String keyID;
    RefPtr<Uint8Array> certificate;
    if (!MediaPlayerPrivateAVFoundationObjC::extractKeyURIKeyIDAndCertificateFromInitData(initData, keyURI, keyID, certificate)) {
        errorCode = INVALID_STATE_ERR;
        return 0;
    }

    m_request = MediaPlayerPrivateAVFoundationObjC::takeRequestForPlayerAndKeyURI(mediaPlayer, keyURI);
    if (!m_request) {
        errorCode = INVALID_STATE_ERR;
        return 0;
    }

    m_sessionId = createCanonicalUUIDString();

    RetainPtr<NSData> certificateData = adoptNS([[NSData alloc] initWithBytes:certificate->baseAddress() length:certificate->byteLength()]);
    NSString* assetStr = keyID;
    RetainPtr<NSData> assetID = [NSData dataWithBytes: [assetStr cStringUsingEncoding:NSUTF8StringEncoding] length:[assetStr lengthOfBytesUsingEncoding:NSUTF8StringEncoding]];
    NSError* error = 0;
    RetainPtr<NSData> keyRequest = [m_request.get() streamingContentKeyRequestDataForApp:certificateData.get() contentIdentifier:assetID.get() options:nil error:&error];

    if (!keyRequest) {
        NSError* underlyingError = [[error userInfo] objectForKey:NSUnderlyingErrorKey];
        errorCode = CDM::DomainError;
        systemCode = [underlyingError code];
        return 0;
    }

    errorCode = 0;
    systemCode = 0;
    destinationURL = String();

    RefPtr<ArrayBuffer> keyRequestBuffer = ArrayBuffer::create([keyRequest.get() bytes], [keyRequest.get() length]);
    return Uint8Array::create(keyRequestBuffer, 0, keyRequestBuffer->byteLength());
}

void CDMSessionAVFoundation::releaseKeys()
{
}

bool CDMSessionAVFoundation::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode)
{
    if (!key)
        return false;

    RetainPtr<NSData> keyData = adoptNS([[NSData alloc] initWithBytes:key->baseAddress() length:key->byteLength()]);
    [[m_request.get() dataRequest] respondWithData:keyData.get()];
    [m_request.get() finishLoading];
    errorCode = 0;
    systemCode = 0;
    nextMessage = nullptr;

    return true;
}

}

#endif

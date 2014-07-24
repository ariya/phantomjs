/*
 * Copyright (C) 2003, 2005, 2008, 2011 Apple Inc. All rights reserved.
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
#include "SSLKeyGenerator.h"

#include "KURL.h"
#include "LocalizedStrings.h"
#include "WebCoreSystemInterface.h"
#include <wtf/RetainPtr.h>

namespace WebCore {

void getSupportedKeySizes(Vector<String>& supportedKeySizes)
{
    ASSERT(supportedKeySizes.isEmpty());
    supportedKeySizes.append(keygenMenuItem2048());
    supportedKeySizes.append(keygenMenuItem1024());
    supportedKeySizes.append(keygenMenuItem512());
}

String signedPublicKeyAndChallengeString(unsigned keySizeIndex, const String& challengeString, const KURL& url)
{   
    // This switch statement must always be synced with the UI strings returned by getSupportedKeySizes.
    UInt32 keySize;
    switch (keySizeIndex) {
    case 0:
        keySize = 2048;
        break;
    case 1:
        keySize = 1024;
        break;
    case 2:
        keySize = 512;
        break;
    default:
        ASSERT_NOT_REACHED();
        return String();
    }

    return adoptCF(wkSignedPublicKeyAndChallengeString(keySize, challengeString.createCFString().get(), keygenKeychainItemName(url.host()).createCFString().get())).get();
}

}

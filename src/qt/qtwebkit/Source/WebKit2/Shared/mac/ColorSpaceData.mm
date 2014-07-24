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

#include "config.h"
#include "ColorSpaceData.h"

#include "ArgumentCodersCF.h"
#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"

namespace WebKit {

enum EncodedDataType {
    Null,
    Name,
    Data,
};

void ColorSpaceData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    if (cgColorSpace) {
        // Try to encode the name.
        if (RetainPtr<CFStringRef> name = adoptCF(CGColorSpaceCopyName(cgColorSpace.get()))) {
            encoder.encodeEnum(Name);
            CoreIPC::encode(encoder, name.get());
            return;
        }

        // Failing that, just encode the ICC data.
        if (RetainPtr<CFDataRef> profileData = adoptCF(CGColorSpaceCopyICCProfile(cgColorSpace.get()))) {
            encoder.encodeEnum(Data);
            CoreIPC::encode(encoder, profileData.get());
            return;
        }
    }

    // The color space was null or failed to be encoded.
    encoder.encodeEnum(Null);
}

bool ColorSpaceData::decode(CoreIPC::ArgumentDecoder& decoder, ColorSpaceData& colorSpaceData)
{
    EncodedDataType dataType;
    if (!decoder.decodeEnum(dataType))
        return false;

    switch (dataType) {
    case Null:
        colorSpaceData.cgColorSpace = nullptr;
        return true;
    case Name: {
        RetainPtr<CFStringRef> name;
        if (!CoreIPC::decode(decoder, name))
            return false;

        colorSpaceData.cgColorSpace = adoptCF(CGColorSpaceCreateWithName(name.get()));
        return true;
    }
    case Data: {
        RetainPtr<CFDataRef> data;
        if (!CoreIPC::decode(decoder, data))
            return false;

        colorSpaceData.cgColorSpace = adoptCF(CGColorSpaceCreateWithICCProfile(data.get()));
        return true;
    }

    default:
        return false;
    }
}

} // namespace WebKit

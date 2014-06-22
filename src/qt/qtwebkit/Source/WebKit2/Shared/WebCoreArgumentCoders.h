/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebCoreArgumentCoders_h
#define WebCoreArgumentCoders_h

#include "ArgumentCoders.h"

namespace WebCore {
    class AffineTransform;
    class AuthenticationChallenge;
    class Color;
    class Credential;
    class Cursor;
    class DatabaseDetails;
    class FloatPoint;
    class FloatRect;
    class FloatSize;
    class HTTPHeaderMap;
    class IntPoint;
    class IntRect;
    class IntSize;
    class KeyframeValueList;
    class KURL;
    class Notification;
    class ProtectionSpace;
    class ResourceError;
    class ResourceRequest;
    class ResourceResponse;
    class TextCheckingRequestData;
    class UserStyleSheet;
    class UserScript;
    struct CompositionUnderline;
    struct Cookie;
    struct DictationAlternative;
    struct DragSession;
    struct FileChooserSettings;
    struct GrammarDetail;
    struct MimeClassInfo;
    struct PluginInfo;
    struct TextCheckingResult;
    struct ViewportAttributes;
    struct WindowFeatures;
}

#if PLATFORM(MAC)
namespace WebCore {
    struct KeypressCommand;
}
#endif

namespace CoreIPC {

template<> struct ArgumentCoder<WebCore::AffineTransform> {
    static void encode(ArgumentEncoder&, const WebCore::AffineTransform&);
    static bool decode(ArgumentDecoder&, WebCore::AffineTransform&);
};

template<> struct ArgumentCoder<WebCore::FloatPoint> {
    static void encode(ArgumentEncoder&, const WebCore::FloatPoint&);
    static bool decode(ArgumentDecoder&, WebCore::FloatPoint&);
};

template<> struct ArgumentCoder<WebCore::FloatRect> {
    static void encode(ArgumentEncoder&, const WebCore::FloatRect&);
    static bool decode(ArgumentDecoder&, WebCore::FloatRect&);
};

template<> struct ArgumentCoder<WebCore::FloatSize> {
    static void encode(ArgumentEncoder&, const WebCore::FloatSize&);
    static bool decode(ArgumentDecoder&, WebCore::FloatSize&);
};

template<> struct ArgumentCoder<WebCore::IntPoint> {
    static void encode(ArgumentEncoder&, const WebCore::IntPoint&);
    static bool decode(ArgumentDecoder&, WebCore::IntPoint&);
};

template<> struct ArgumentCoder<WebCore::IntRect> {
    static void encode(ArgumentEncoder&, const WebCore::IntRect&);
    static bool decode(ArgumentDecoder&, WebCore::IntRect&);
};

template<> struct ArgumentCoder<WebCore::IntSize> {
    static void encode(ArgumentEncoder&, const WebCore::IntSize&);
    static bool decode(ArgumentDecoder&, WebCore::IntSize&);
};

template<> struct ArgumentCoder<WebCore::ViewportAttributes> {
    static void encode(ArgumentEncoder&, const WebCore::ViewportAttributes&);
    static bool decode(ArgumentDecoder&, WebCore::ViewportAttributes&);
};

template<> struct ArgumentCoder<WebCore::MimeClassInfo> {
    static void encode(ArgumentEncoder&, const WebCore::MimeClassInfo&);
    static bool decode(ArgumentDecoder&, WebCore::MimeClassInfo&);
};

template<> struct ArgumentCoder<WebCore::PluginInfo> {
    static void encode(ArgumentEncoder&, const WebCore::PluginInfo&);
    static bool decode(ArgumentDecoder&, WebCore::PluginInfo&);
};

template<> struct ArgumentCoder<WebCore::HTTPHeaderMap> {
    static void encode(ArgumentEncoder&, const WebCore::HTTPHeaderMap&);
    static bool decode(ArgumentDecoder&, WebCore::HTTPHeaderMap&);
};

template<> struct ArgumentCoder<WebCore::AuthenticationChallenge> {
    static void encode(ArgumentEncoder&, const WebCore::AuthenticationChallenge&);
    static bool decode(ArgumentDecoder&, WebCore::AuthenticationChallenge&);
};

template<> struct ArgumentCoder<WebCore::ProtectionSpace> {
    static void encode(ArgumentEncoder&, const WebCore::ProtectionSpace&);
    static bool decode(ArgumentDecoder&, WebCore::ProtectionSpace&);
};

template<> struct ArgumentCoder<WebCore::Credential> {
    static void encode(ArgumentEncoder&, const WebCore::Credential&);
    static bool decode(ArgumentDecoder&, WebCore::Credential&);
};

template<> struct ArgumentCoder<WebCore::Cursor> {
    static void encode(ArgumentEncoder&, const WebCore::Cursor&);
    static bool decode(ArgumentDecoder&, WebCore::Cursor&);
};

template<> struct ArgumentCoder<WebCore::ResourceRequest> {
#if PLATFORM(MAC)
    static const bool kShouldSerializeWebCoreData = false;
#else
    static const bool kShouldSerializeWebCoreData = true;
#endif

    static void encode(ArgumentEncoder&, const WebCore::ResourceRequest&);
    static bool decode(ArgumentDecoder&, WebCore::ResourceRequest&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::ResourceRequest&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::ResourceRequest&);
};

template<> struct ArgumentCoder<WebCore::ResourceResponse> {
    static void encode(ArgumentEncoder&, const WebCore::ResourceResponse&);
    static bool decode(ArgumentDecoder&, WebCore::ResourceResponse&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::ResourceResponse&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::ResourceResponse&);
};

template<> struct ArgumentCoder<WebCore::ResourceError> {
#if PLATFORM(MAC)
    static const bool kShouldSerializeWebCoreData = false;
#else
    static const bool kShouldSerializeWebCoreData = true;
#endif

    static void encode(ArgumentEncoder&, const WebCore::ResourceError&);
    static bool decode(ArgumentDecoder&, WebCore::ResourceError&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::ResourceError&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::ResourceError&);
};

template<> struct ArgumentCoder<WebCore::WindowFeatures> {
    static void encode(ArgumentEncoder&, const WebCore::WindowFeatures&);
    static bool decode(ArgumentDecoder&, WebCore::WindowFeatures&);
};

template<> struct ArgumentCoder<WebCore::Color> {
    static void encode(ArgumentEncoder&, const WebCore::Color&);
    static bool decode(ArgumentDecoder&, WebCore::Color&);
};

#if PLATFORM(MAC)
template<> struct ArgumentCoder<WebCore::KeypressCommand> {
    static void encode(ArgumentEncoder&, const WebCore::KeypressCommand&);
    static bool decode(ArgumentDecoder&, WebCore::KeypressCommand&);
};
#endif

template<> struct ArgumentCoder<WebCore::CompositionUnderline> {
    static void encode(ArgumentEncoder&, const WebCore::CompositionUnderline&);
    static bool decode(ArgumentDecoder&, WebCore::CompositionUnderline&);
};

template<> struct ArgumentCoder<WebCore::Cookie> {
    static void encode(ArgumentEncoder&, const WebCore::Cookie&);
    static bool decode(ArgumentDecoder&, WebCore::Cookie&);
};

template<> struct ArgumentCoder<WebCore::DatabaseDetails> {
    static void encode(ArgumentEncoder&, const WebCore::DatabaseDetails&);
    static bool decode(ArgumentDecoder&, WebCore::DatabaseDetails&);
};

template<> struct ArgumentCoder<WebCore::DictationAlternative> {
    static void encode(ArgumentEncoder&, const WebCore::DictationAlternative&);
    static bool decode(ArgumentDecoder&, WebCore::DictationAlternative&);
};

template<> struct ArgumentCoder<WebCore::FileChooserSettings> {
    static void encode(ArgumentEncoder&, const WebCore::FileChooserSettings&);
    static bool decode(ArgumentDecoder&, WebCore::FileChooserSettings&);
};

template<> struct ArgumentCoder<WebCore::GrammarDetail> {
    static void encode(ArgumentEncoder&, const WebCore::GrammarDetail&);
    static bool decode(ArgumentDecoder&, WebCore::GrammarDetail&);
};

template<> struct ArgumentCoder<WebCore::TextCheckingRequestData> {
    static void encode(ArgumentEncoder&, const WebCore::TextCheckingRequestData&);
    static bool decode(ArgumentDecoder&, WebCore::TextCheckingRequestData&);
};

template<> struct ArgumentCoder<WebCore::TextCheckingResult> {
    static void encode(ArgumentEncoder&, const WebCore::TextCheckingResult&);
    static bool decode(ArgumentDecoder&, WebCore::TextCheckingResult&);
};
    
template<> struct ArgumentCoder<WebCore::DragSession> {
    static void encode(ArgumentEncoder&, const WebCore::DragSession&);
    static bool decode(ArgumentDecoder&, WebCore::DragSession&);
};

template<> struct ArgumentCoder<WebCore::KURL> {
    static void encode(ArgumentEncoder&, const WebCore::KURL&);
    static bool decode(ArgumentDecoder&, WebCore::KURL&);
};

template<> struct ArgumentCoder<WebCore::UserStyleSheet> {
    static void encode(ArgumentEncoder&, const WebCore::UserStyleSheet&);
    static bool decode(ArgumentDecoder&, WebCore::UserStyleSheet&);
};

template<> struct ArgumentCoder<WebCore::UserScript> {
    static void encode(ArgumentEncoder&, const WebCore::UserScript&);
    static bool decode(ArgumentDecoder&, WebCore::UserScript&);
};

} // namespace CoreIPC

#endif // WebCoreArgumentCoders_h

/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WebCoreArgumentCoders.h"

#include "ShareableBitmap.h"
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/Cookie.h>
#include <WebCore/Credential.h>
#include <WebCore/Cursor.h>
#include <WebCore/DatabaseDetails.h>
#include <WebCore/DictationAlternative.h>
#include <WebCore/DragSession.h>
#include <WebCore/Editor.h>
#include <WebCore/FileChooser.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/GraphicsLayer.h>
#include <WebCore/Image.h>
#include <WebCore/KURL.h>
#include <WebCore/PluginData.h>
#include <WebCore/ProtectionSpace.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/TextCheckerClient.h>
#include <WebCore/UserScript.h>
#include <WebCore/UserStyleSheet.h>
#include <WebCore/ViewportArguments.h>
#include <WebCore/WindowFeatures.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

using namespace WebCore;
using namespace WebKit;

namespace CoreIPC {

void ArgumentCoder<AffineTransform>::encode(ArgumentEncoder& encoder, const AffineTransform& affineTransform)
{
    SimpleArgumentCoder<AffineTransform>::encode(encoder, affineTransform);
}

bool ArgumentCoder<AffineTransform>::decode(ArgumentDecoder& decoder, AffineTransform& affineTransform)
{
    return SimpleArgumentCoder<AffineTransform>::decode(decoder, affineTransform);
}


void ArgumentCoder<FloatPoint>::encode(ArgumentEncoder& encoder, const FloatPoint& floatPoint)
{
    SimpleArgumentCoder<FloatPoint>::encode(encoder, floatPoint);
}

bool ArgumentCoder<FloatPoint>::decode(ArgumentDecoder& decoder, FloatPoint& floatPoint)
{
    return SimpleArgumentCoder<FloatPoint>::decode(decoder, floatPoint);
}


void ArgumentCoder<FloatRect>::encode(ArgumentEncoder& encoder, const FloatRect& floatRect)
{
    SimpleArgumentCoder<FloatRect>::encode(encoder, floatRect);
}

bool ArgumentCoder<FloatRect>::decode(ArgumentDecoder& decoder, FloatRect& floatRect)
{
    return SimpleArgumentCoder<FloatRect>::decode(decoder, floatRect);
}


void ArgumentCoder<FloatSize>::encode(ArgumentEncoder& encoder, const FloatSize& floatSize)
{
    SimpleArgumentCoder<FloatSize>::encode(encoder, floatSize);
}

bool ArgumentCoder<FloatSize>::decode(ArgumentDecoder& decoder, FloatSize& floatSize)
{
    return SimpleArgumentCoder<FloatSize>::decode(decoder, floatSize);
}


void ArgumentCoder<IntPoint>::encode(ArgumentEncoder& encoder, const IntPoint& intPoint)
{
    SimpleArgumentCoder<IntPoint>::encode(encoder, intPoint);
}

bool ArgumentCoder<IntPoint>::decode(ArgumentDecoder& decoder, IntPoint& intPoint)
{
    return SimpleArgumentCoder<IntPoint>::decode(decoder, intPoint);
}


void ArgumentCoder<IntRect>::encode(ArgumentEncoder& encoder, const IntRect& intRect)
{
    SimpleArgumentCoder<IntRect>::encode(encoder, intRect);
}

bool ArgumentCoder<IntRect>::decode(ArgumentDecoder& decoder, IntRect& intRect)
{
    return SimpleArgumentCoder<IntRect>::decode(decoder, intRect);
}


void ArgumentCoder<IntSize>::encode(ArgumentEncoder& encoder, const IntSize& intSize)
{
    SimpleArgumentCoder<IntSize>::encode(encoder, intSize);
}

bool ArgumentCoder<IntSize>::decode(ArgumentDecoder& decoder, IntSize& intSize)
{
    return SimpleArgumentCoder<IntSize>::decode(decoder, intSize);
}


void ArgumentCoder<ViewportAttributes>::encode(ArgumentEncoder& encoder, const ViewportAttributes& viewportAttributes)
{
    SimpleArgumentCoder<ViewportAttributes>::encode(encoder, viewportAttributes);
}

bool ArgumentCoder<ViewportAttributes>::decode(ArgumentDecoder& decoder, ViewportAttributes& viewportAttributes)
{
    return SimpleArgumentCoder<ViewportAttributes>::decode(decoder, viewportAttributes);
}


void ArgumentCoder<MimeClassInfo>::encode(ArgumentEncoder& encoder, const MimeClassInfo& mimeClassInfo)
{
    encoder << mimeClassInfo.type << mimeClassInfo.desc << mimeClassInfo.extensions;
}

bool ArgumentCoder<MimeClassInfo>::decode(ArgumentDecoder& decoder, MimeClassInfo& mimeClassInfo)
{
    if (!decoder.decode(mimeClassInfo.type))
        return false;
    if (!decoder.decode(mimeClassInfo.desc))
        return false;
    if (!decoder.decode(mimeClassInfo.extensions))
        return false;

    return true;
}


void ArgumentCoder<PluginInfo>::encode(ArgumentEncoder& encoder, const PluginInfo& pluginInfo)
{
    encoder << pluginInfo.name << pluginInfo.file << pluginInfo.desc << pluginInfo.mimes << pluginInfo.isApplicationPlugin;
}
    
bool ArgumentCoder<PluginInfo>::decode(ArgumentDecoder& decoder, PluginInfo& pluginInfo)
{
    if (!decoder.decode(pluginInfo.name))
        return false;
    if (!decoder.decode(pluginInfo.file))
        return false;
    if (!decoder.decode(pluginInfo.desc))
        return false;
    if (!decoder.decode(pluginInfo.mimes))
        return false;
    if (!decoder.decode(pluginInfo.isApplicationPlugin))
        return false;

    return true;
}


void ArgumentCoder<HTTPHeaderMap>::encode(ArgumentEncoder& encoder, const HTTPHeaderMap& headerMap)
{
    encoder << static_cast<const HashMap<AtomicString, String, CaseFoldingHash>&>(headerMap);
}

bool ArgumentCoder<HTTPHeaderMap>::decode(ArgumentDecoder& decoder, HTTPHeaderMap& headerMap)
{
    return decoder.decode(static_cast<HashMap<AtomicString, String, CaseFoldingHash>&>(headerMap));
}


void ArgumentCoder<AuthenticationChallenge>::encode(ArgumentEncoder& encoder, const AuthenticationChallenge& challenge)
{
    encoder << challenge.protectionSpace() << challenge.proposedCredential() << challenge.previousFailureCount() << challenge.failureResponse() << challenge.error();
}

bool ArgumentCoder<AuthenticationChallenge>::decode(ArgumentDecoder& decoder, AuthenticationChallenge& challenge)
{    
    ProtectionSpace protectionSpace;
    if (!decoder.decode(protectionSpace))
        return false;

    Credential proposedCredential;
    if (!decoder.decode(proposedCredential))
        return false;

    unsigned previousFailureCount;    
    if (!decoder.decode(previousFailureCount))
        return false;

    ResourceResponse failureResponse;
    if (!decoder.decode(failureResponse))
        return false;

    ResourceError error;
    if (!decoder.decode(error))
        return false;
    
    challenge = AuthenticationChallenge(protectionSpace, proposedCredential, previousFailureCount, failureResponse, error);
    return true;
}


void ArgumentCoder<ProtectionSpace>::encode(ArgumentEncoder& encoder, const ProtectionSpace& space)
{
    encoder << space.host() << space.port() << space.realm();
    encoder.encodeEnum(space.authenticationScheme());
    encoder.encodeEnum(space.serverType());
}

bool ArgumentCoder<ProtectionSpace>::decode(ArgumentDecoder& decoder, ProtectionSpace& space)
{
    String host;
    if (!decoder.decode(host))
        return false;

    int port;
    if (!decoder.decode(port))
        return false;

    String realm;
    if (!decoder.decode(realm))
        return false;
    
    ProtectionSpaceAuthenticationScheme authenticationScheme;
    if (!decoder.decodeEnum(authenticationScheme))
        return false;

    ProtectionSpaceServerType serverType;
    if (!decoder.decodeEnum(serverType))
        return false;

    space = ProtectionSpace(host, port, serverType, realm, authenticationScheme);
    return true;
}

void ArgumentCoder<Credential>::encode(ArgumentEncoder& encoder, const Credential& credential)
{
    encoder << credential.user() << credential.password();
    encoder.encodeEnum(credential.persistence());
}

bool ArgumentCoder<Credential>::decode(ArgumentDecoder& decoder, Credential& credential)
{
    String user;
    if (!decoder.decode(user))
        return false;

    String password;
    if (!decoder.decode(password))
        return false;

    CredentialPersistence persistence;
    if (!decoder.decodeEnum(persistence))
        return false;
    
    credential = Credential(user, password, persistence);
    return true;
}

static void encodeImage(ArgumentEncoder& encoder, Image* image)
{
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(image->size(), ShareableBitmap::SupportsAlpha);
    bitmap->createGraphicsContext()->drawImage(image, ColorSpaceDeviceRGB, IntPoint());

    ShareableBitmap::Handle handle;
    bitmap->createHandle(handle);

    encoder << handle;
}

static bool decodeImage(ArgumentDecoder& decoder, RefPtr<Image>& image)
{
    ShareableBitmap::Handle handle;
    if (!decoder.decode(handle))
        return false;
    
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::create(handle);
    if (!bitmap)
        return false;
    image = bitmap->createImage();
    if (!image)
        return false;
    return true;
}

void ArgumentCoder<Cursor>::encode(ArgumentEncoder& encoder, const Cursor& cursor)
{
    encoder.encodeEnum(cursor.type());
        
    if (cursor.type() != Cursor::Custom)
        return;

    if (cursor.image()->isNull()) {
        encoder << false; // There is no valid image being encoded.
        return;
    }

    encoder << true;
    encodeImage(encoder, cursor.image());
    encoder << cursor.hotSpot();
}

bool ArgumentCoder<Cursor>::decode(ArgumentDecoder& decoder, Cursor& cursor)
{
    Cursor::Type type;
    if (!decoder.decodeEnum(type))
        return false;

    if (type > Cursor::Custom)
        return false;

    if (type != Cursor::Custom) {
        const Cursor& cursorReference = Cursor::fromType(type);
        // Calling platformCursor here will eagerly create the platform cursor for the cursor singletons inside WebCore.
        // This will avoid having to re-create the platform cursors over and over.
        (void)cursorReference.platformCursor();

        cursor = cursorReference;
        return true;
    }

    bool isValidImagePresent;
    if (!decoder.decode(isValidImagePresent))
        return false;

    if (!isValidImagePresent) {
        cursor = Cursor(Image::nullImage(), IntPoint());
        return true;
    }

    RefPtr<Image> image;
    if (!decodeImage(decoder, image))
        return false;

    IntPoint hotSpot;
    if (!decoder.decode(hotSpot))
        return false;

    if (!image->rect().contains(hotSpot))
        return false;

    cursor = Cursor(image.get(), hotSpot);
    return true;
}

void ArgumentCoder<ResourceRequest>::encode(ArgumentEncoder& encoder, const ResourceRequest& resourceRequest)
{
    if (kShouldSerializeWebCoreData) {
        encoder << resourceRequest.url().string();
        encoder << resourceRequest.httpMethod();
        encoder << resourceRequest.httpHeaderFields();

        // FIXME: Do not encode HTTP message body.
        // 1. It can be large and thus costly to send across.
        // 2. It is misleading to provide a body with some requests, while others use body streams, which cannot be serialized at all.
        FormData* httpBody = resourceRequest.httpBody();
        encoder << static_cast<bool>(httpBody);
        if (httpBody)
            encoder << httpBody->flattenToString();

        encoder << resourceRequest.firstPartyForCookies().string();
    }

#if ENABLE(CACHE_PARTITIONING)
    encoder << resourceRequest.cachePartition();
#endif

    encodePlatformData(encoder, resourceRequest);
}

bool ArgumentCoder<ResourceRequest>::decode(ArgumentDecoder& decoder, ResourceRequest& resourceRequest)
{
    if (kShouldSerializeWebCoreData) {
        ResourceRequest request;

        String url;
        if (!decoder.decode(url))
            return false;
        request.setURL(KURL(KURL(), url));

        String httpMethod;
        if (!decoder.decode(httpMethod))
            return false;
        request.setHTTPMethod(httpMethod);

        HTTPHeaderMap headers;
        if (!decoder.decode(headers))
            return false;
        request.addHTTPHeaderFields(headers);

        bool hasHTTPBody;
        if (!decoder.decode(hasHTTPBody))
            return false;
        if (hasHTTPBody) {
            String httpBody;
            if (!decoder.decode(httpBody))
                return false;
            request.setHTTPBody(FormData::create(httpBody.utf8()));
        }

        String firstPartyForCookies;
        if (!decoder.decode(firstPartyForCookies))
            return false;
        request.setFirstPartyForCookies(KURL(KURL(), firstPartyForCookies));

        resourceRequest = request;
    }

#if ENABLE(CACHE_PARTITIONING)
    String cachePartition;
    if (!decoder.decode(cachePartition))
        return false;
    resourceRequest.setCachePartition(cachePartition);
#endif

    return decodePlatformData(decoder, resourceRequest);
}

void ArgumentCoder<ResourceResponse>::encode(ArgumentEncoder& encoder, const ResourceResponse& resourceResponse)
{
#if PLATFORM(MAC)
    bool shouldSerializeWebCoreData = !resourceResponse.platformResponseIsUpToDate();
    encoder << shouldSerializeWebCoreData;
#else
    bool shouldSerializeWebCoreData = true;
#endif

    encodePlatformData(encoder, resourceResponse);

    if (shouldSerializeWebCoreData) {
        bool responseIsNull = resourceResponse.isNull();
        encoder << responseIsNull;
        if (responseIsNull)
            return;

        encoder << resourceResponse.url().string();
        encoder << static_cast<int32_t>(resourceResponse.httpStatusCode());
        encoder << resourceResponse.httpHeaderFields();

        encoder << resourceResponse.mimeType();
        encoder << resourceResponse.textEncodingName();
        encoder << static_cast<int64_t>(resourceResponse.expectedContentLength());
        encoder << resourceResponse.httpStatusText();
        encoder << resourceResponse.suggestedFilename();
    }
}

bool ArgumentCoder<ResourceResponse>::decode(ArgumentDecoder& decoder, ResourceResponse& resourceResponse)
{
#if PLATFORM(MAC)
    bool hasSerializedWebCoreData;
    if (!decoder.decode(hasSerializedWebCoreData))
        return false;
#else
    bool hasSerializedWebCoreData = true;
#endif

    ResourceResponse response;

    if (!decodePlatformData(decoder, response))
        return false;

    if (hasSerializedWebCoreData) {
        bool responseIsNull;
        if (!decoder.decode(responseIsNull))
            return false;
        if (responseIsNull) {
            resourceResponse = ResourceResponse();
            return true;
        }

        String url;
        if (!decoder.decode(url))
            return false;
        response.setURL(KURL(KURL(), url));

        int32_t httpStatusCode;
        if (!decoder.decode(httpStatusCode))
            return false;
        response.setHTTPStatusCode(httpStatusCode);

        HTTPHeaderMap headers;
        if (!decoder.decode(headers))
            return false;
        for (HTTPHeaderMap::const_iterator it = headers.begin(), end = headers.end(); it != end; ++it)
            response.setHTTPHeaderField(it->key, it->value);

        String mimeType;
        if (!decoder.decode(mimeType))
            return false;
        response.setMimeType(mimeType);

        String textEncodingName;
        if (!decoder.decode(textEncodingName))
            return false;
        response.setTextEncodingName(textEncodingName);

        int64_t contentLength;
        if (!decoder.decode(contentLength))
            return false;
        response.setExpectedContentLength(contentLength);

        String httpStatusText;
        if (!decoder.decode(httpStatusText))
            return false;
        response.setHTTPStatusText(httpStatusText);

        String suggestedFilename;
        if (!decoder.decode(suggestedFilename))
            return false;
        response.setSuggestedFilename(suggestedFilename);
    }

    resourceResponse = response;

    return true;
}

void ArgumentCoder<ResourceError>::encode(ArgumentEncoder& encoder, const ResourceError& resourceError)
{
    if (kShouldSerializeWebCoreData) {
        bool errorIsNull = resourceError.isNull();
        encoder << errorIsNull;
        if (errorIsNull)
            return;

        encoder << resourceError.domain();
        encoder << resourceError.errorCode();
        encoder << resourceError.failingURL();
        encoder << resourceError.localizedDescription();
        encoder << resourceError.isCancellation();
        encoder << resourceError.isTimeout();
    }

    encodePlatformData(encoder, resourceError);
}

bool ArgumentCoder<ResourceError>::decode(ArgumentDecoder& decoder, ResourceError& resourceError)
{
    if (kShouldSerializeWebCoreData) {
        bool errorIsNull;
        if (!decoder.decode(errorIsNull))
            return false;
        if (errorIsNull) {
            resourceError = ResourceError();
            return true;
        }

        String domain;
        if (!decoder.decode(domain))
            return false;

        int errorCode;
        if (!decoder.decode(errorCode))
            return false;

        String failingURL;
        if (!decoder.decode(failingURL))
            return false;

        String localizedDescription;
        if (!decoder.decode(localizedDescription))
            return false;

        bool isCancellation;
        if (!decoder.decode(isCancellation))
            return false;

        bool isTimeout;
        if (!decoder.decode(isTimeout))
            return false;

        resourceError = ResourceError(domain, errorCode, failingURL, localizedDescription);
        resourceError.setIsCancellation(isCancellation);
        resourceError.setIsTimeout(isTimeout);
    }

    return decodePlatformData(decoder, resourceError);
}

void ArgumentCoder<WindowFeatures>::encode(ArgumentEncoder& encoder, const WindowFeatures& windowFeatures)
{
    encoder << windowFeatures.x;
    encoder << windowFeatures.y;
    encoder << windowFeatures.width;
    encoder << windowFeatures.height;
    encoder << windowFeatures.xSet;
    encoder << windowFeatures.ySet;
    encoder << windowFeatures.widthSet;
    encoder << windowFeatures.heightSet;
    encoder << windowFeatures.menuBarVisible;
    encoder << windowFeatures.statusBarVisible;
    encoder << windowFeatures.toolBarVisible;
    encoder << windowFeatures.locationBarVisible;
    encoder << windowFeatures.scrollbarsVisible;
    encoder << windowFeatures.resizable;
    encoder << windowFeatures.fullscreen;
    encoder << windowFeatures.dialog;
}

bool ArgumentCoder<WindowFeatures>::decode(ArgumentDecoder& decoder, WindowFeatures& windowFeatures)
{
    if (!decoder.decode(windowFeatures.x))
        return false;
    if (!decoder.decode(windowFeatures.y))
        return false;
    if (!decoder.decode(windowFeatures.width))
        return false;
    if (!decoder.decode(windowFeatures.height))
        return false;
    if (!decoder.decode(windowFeatures.xSet))
        return false;
    if (!decoder.decode(windowFeatures.ySet))
        return false;
    if (!decoder.decode(windowFeatures.widthSet))
        return false;
    if (!decoder.decode(windowFeatures.heightSet))
        return false;
    if (!decoder.decode(windowFeatures.menuBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.statusBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.toolBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.locationBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.scrollbarsVisible))
        return false;
    if (!decoder.decode(windowFeatures.resizable))
        return false;
    if (!decoder.decode(windowFeatures.fullscreen))
        return false;
    if (!decoder.decode(windowFeatures.dialog))
        return false;
    return true;
}


void ArgumentCoder<Color>::encode(ArgumentEncoder& encoder, const Color& color)
{
    if (!color.isValid()) {
        encoder << false;
        return;
    }

    encoder << true;
    encoder << color.rgb();
}

bool ArgumentCoder<Color>::decode(ArgumentDecoder& decoder, Color& color)
{
    bool isValid;
    if (!decoder.decode(isValid))
        return false;

    if (!isValid) {
        color = Color();
        return true;
    }

    RGBA32 rgba;
    if (!decoder.decode(rgba))
        return false;

    color = Color(rgba);
    return true;
}


void ArgumentCoder<CompositionUnderline>::encode(ArgumentEncoder& encoder, const CompositionUnderline& underline)
{
    encoder << underline.startOffset;
    encoder << underline.endOffset;
    encoder << underline.thick;
    encoder << underline.color;
}

bool ArgumentCoder<CompositionUnderline>::decode(ArgumentDecoder& decoder, CompositionUnderline& underline)
{
    if (!decoder.decode(underline.startOffset))
        return false;
    if (!decoder.decode(underline.endOffset))
        return false;
    if (!decoder.decode(underline.thick))
        return false;
    if (!decoder.decode(underline.color))
        return false;

    return true;
}


void ArgumentCoder<Cookie>::encode(ArgumentEncoder& encoder, const Cookie& cookie)
{
    encoder << cookie.name;
    encoder << cookie.value;
    encoder << cookie.domain;
    encoder << cookie.path;
    encoder << cookie.expires;
    encoder << cookie.httpOnly;
    encoder << cookie.secure;
    encoder << cookie.session;
}

bool ArgumentCoder<Cookie>::decode(ArgumentDecoder& decoder, Cookie& cookie)
{
    if (!decoder.decode(cookie.name))
        return false;
    if (!decoder.decode(cookie.value))
        return false;
    if (!decoder.decode(cookie.domain))
        return false;
    if (!decoder.decode(cookie.path))
        return false;
    if (!decoder.decode(cookie.expires))
        return false;
    if (!decoder.decode(cookie.httpOnly))
        return false;
    if (!decoder.decode(cookie.secure))
        return false;
    if (!decoder.decode(cookie.session))
        return false;

    return true;
}


#if ENABLE(SQL_DATABASE)
void ArgumentCoder<DatabaseDetails>::encode(ArgumentEncoder& encoder, const DatabaseDetails& details)
{
    encoder << details.name();
    encoder << details.displayName();
    encoder << details.expectedUsage();
    encoder << details.currentUsage();
}
    
bool ArgumentCoder<DatabaseDetails>::decode(ArgumentDecoder& decoder, DatabaseDetails& details)
{
    String name;
    if (!decoder.decode(name))
        return false;

    String displayName;
    if (!decoder.decode(displayName))
        return false;

    uint64_t expectedUsage;
    if (!decoder.decode(expectedUsage))
        return false;

    uint64_t currentUsage;
    if (!decoder.decode(currentUsage))
        return false;
    
    details = DatabaseDetails(name, displayName, expectedUsage, currentUsage);
    return true;
}
#endif

void ArgumentCoder<DictationAlternative>::encode(ArgumentEncoder& encoder, const DictationAlternative& dictationAlternative)
{
    encoder << dictationAlternative.rangeStart;
    encoder << dictationAlternative.rangeLength;
    encoder << dictationAlternative.dictationContext;
}

bool ArgumentCoder<DictationAlternative>::decode(ArgumentDecoder& decoder, DictationAlternative& dictationAlternative)
{
    if (!decoder.decode(dictationAlternative.rangeStart))
        return false;
    if (!decoder.decode(dictationAlternative.rangeLength))
        return false;
    if (!decoder.decode(dictationAlternative.dictationContext))
        return false;
    return true;
}


void ArgumentCoder<FileChooserSettings>::encode(ArgumentEncoder& encoder, const FileChooserSettings& settings)
{
    encoder << settings.allowsMultipleFiles;
#if ENABLE(DIRECTORY_UPLOAD)
    encoder << settings.allowsDirectoryUpload;
#endif
    encoder << settings.acceptMIMETypes;
    encoder << settings.selectedFiles;
#if ENABLE(MEDIA_CAPTURE)
    encoder << settings.capture;
#endif
}

bool ArgumentCoder<FileChooserSettings>::decode(ArgumentDecoder& decoder, FileChooserSettings& settings)
{
    if (!decoder.decode(settings.allowsMultipleFiles))
        return false;
#if ENABLE(DIRECTORY_UPLOAD)
    if (!decoder.decode(settings.allowsDirectoryUpload))
        return false;
#endif
    if (!decoder.decode(settings.acceptMIMETypes))
        return false;
    if (!decoder.decode(settings.selectedFiles))
        return false;
#if ENABLE(MEDIA_CAPTURE)
    if (!decoder.decode(settings.capture))
        return false;
#endif

    return true;
}


void ArgumentCoder<GrammarDetail>::encode(ArgumentEncoder& encoder, const GrammarDetail& detail)
{
    encoder << detail.location;
    encoder << detail.length;
    encoder << detail.guesses;
    encoder << detail.userDescription;
}

bool ArgumentCoder<GrammarDetail>::decode(ArgumentDecoder& decoder, GrammarDetail& detail)
{
    if (!decoder.decode(detail.location))
        return false;
    if (!decoder.decode(detail.length))
        return false;
    if (!decoder.decode(detail.guesses))
        return false;
    if (!decoder.decode(detail.userDescription))
        return false;

    return true;
}

void ArgumentCoder<TextCheckingRequestData>::encode(ArgumentEncoder& encoder, const TextCheckingRequestData& request)
{
    encoder << request.sequence();
    encoder << request.text();
    encoder << request.mask();
    encoder.encodeEnum(request.processType());
}

bool ArgumentCoder<TextCheckingRequestData>::decode(ArgumentDecoder& decoder, TextCheckingRequestData& request)
{
    int sequence;
    if (!decoder.decode(sequence))
        return false;

    String text;
    if (!decoder.decode(text))
        return false;

    TextCheckingTypeMask mask;
    if (!decoder.decode(mask))
        return false;

    TextCheckingProcessType processType;
    if (!decoder.decodeEnum(processType))
        return false;

    request = TextCheckingRequestData(sequence, text, mask, processType);
    return true;
}

void ArgumentCoder<TextCheckingResult>::encode(ArgumentEncoder& encoder, const TextCheckingResult& result)
{
    encoder.encodeEnum(result.type);
    encoder << result.location;
    encoder << result.length;
    encoder << result.details;
    encoder << result.replacement;
}

bool ArgumentCoder<TextCheckingResult>::decode(ArgumentDecoder& decoder, TextCheckingResult& result)
{
    if (!decoder.decodeEnum(result.type))
        return false;
    if (!decoder.decode(result.location))
        return false;
    if (!decoder.decode(result.length))
        return false;
    if (!decoder.decode(result.details))
        return false;
    if (!decoder.decode(result.replacement))
        return false;
    return true;
}

void ArgumentCoder<DragSession>::encode(ArgumentEncoder& encoder, const DragSession& result)
{
    encoder.encodeEnum(result.operation);
    encoder << result.mouseIsOverFileInput;
    encoder << result.numberOfItemsToBeAccepted;
}

bool ArgumentCoder<DragSession>::decode(ArgumentDecoder& decoder, DragSession& result)
{
    if (!decoder.decodeEnum(result.operation))
        return false;
    if (!decoder.decode(result.mouseIsOverFileInput))
        return false;
    if (!decoder.decode(result.numberOfItemsToBeAccepted))
        return false;
    return true;
}

void ArgumentCoder<KURL>::encode(ArgumentEncoder& encoder, const KURL& result)
{
    encoder << result.string();
}
    
bool ArgumentCoder<KURL>::decode(ArgumentDecoder& decoder, KURL& result)
{
    String urlAsString;
    if (!decoder.decode(urlAsString))
        return false;
    result = KURL(WebCore::ParsedURLString, urlAsString);
    return true;
}

void ArgumentCoder<WebCore::UserStyleSheet>::encode(ArgumentEncoder& encoder, const WebCore::UserStyleSheet& userStyleSheet)
{
    encoder << userStyleSheet.source();
    encoder << userStyleSheet.url();
    encoder << userStyleSheet.whitelist();
    encoder << userStyleSheet.blacklist();
    encoder.encodeEnum(userStyleSheet.injectedFrames());
    encoder.encodeEnum(userStyleSheet.level());
}

bool ArgumentCoder<WebCore::UserStyleSheet>::decode(ArgumentDecoder& decoder, WebCore::UserStyleSheet& userStyleSheet)
{
    String source;
    if (!decoder.decode(source))
        return false;

    KURL url;
    if (!decoder.decode(url))
        return false;

    Vector<String> whitelist;
    if (!decoder.decode(whitelist))
        return false;

    Vector<String> blacklist;
    if (!decoder.decode(blacklist))
        return false;

    WebCore::UserContentInjectedFrames injectedFrames;
    if (!decoder.decodeEnum(injectedFrames))
        return false;

    WebCore::UserStyleLevel level;
    if (!decoder.decodeEnum(level))
        return false;

    userStyleSheet = WebCore::UserStyleSheet(source, url, whitelist, blacklist, injectedFrames, level);
    return true;
}

void ArgumentCoder<WebCore::UserScript>::encode(ArgumentEncoder& encoder, const WebCore::UserScript& userScript)
{
    encoder << userScript.source();
    encoder << userScript.url();
    encoder << userScript.whitelist();
    encoder << userScript.blacklist();
    encoder.encodeEnum(userScript.injectionTime());
    encoder.encodeEnum(userScript.injectedFrames());
}

bool ArgumentCoder<WebCore::UserScript>::decode(ArgumentDecoder& decoder, WebCore::UserScript& userScript)
{
    String source;
    if (!decoder.decode(source))
        return false;

    KURL url;
    if (!decoder.decode(url))
        return false;

    Vector<String> whitelist;
    if (!decoder.decode(whitelist))
        return false;

    Vector<String> blacklist;
    if (!decoder.decode(blacklist))
        return false;

    WebCore::UserScriptInjectionTime injectionTime;
    if (!decoder.decodeEnum(injectionTime))
        return false;

    WebCore::UserContentInjectedFrames injectedFrames;
    if (!decoder.decodeEnum(injectedFrames))
        return false;

    userScript = WebCore::UserScript(source, url, whitelist, blacklist, injectionTime, injectedFrames);
    return true;
}

} // namespace CoreIPC

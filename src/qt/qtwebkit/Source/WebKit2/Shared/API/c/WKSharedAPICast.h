/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WKSharedAPICast_h
#define WKSharedAPICast_h

#include "ImageOptions.h"
#include "SameDocumentNavigationType.h"
#include "WKBase.h"
#include "WKContextMenuItemTypes.h"
#include "WKEvent.h"
#include "WKFindOptions.h"
#include "WKGeometry.h"
#include "WKImage.h"
#include "WKPageLoadTypes.h"
#include "WKPageLoadTypesPrivate.h"
#include "WKPageVisibilityTypes.h"
#include "WebError.h"
#include "WebEvent.h"
#include "WebFindOptions.h"
#include "WebNumber.h"
#include "WebSecurityOrigin.h"
#include "WebString.h"
#include "WebURL.h"
#include "WebURLRequest.h"
#include "WebURLResponse.h"
#include <WebCore/ContextMenuItem.h>
#include <WebCore/FloatRect.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/IntRect.h>
#include <WebCore/LayoutMilestones.h>
#include <WebCore/PageVisibilityState.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/UserContentTypes.h>
#include <WebCore/UserScriptTypes.h>
#include <wtf/TypeTraits.h>

namespace WebKit {

class ImmutableArray;
class ImmutableDictionary;
class MutableArray;
class MutableDictionary;
class ObjCObjectGraph;
class WebArchive;
class WebArchiveResource;
class WebCertificateInfo;
class WebConnection;
class WebContextMenuItem;
class WebData;
class WebGraphicsContext;
class WebImage;
class WebPoint;
class WebRect;
class WebSecurityOrigin;
class WebSerializedScriptValue;
class WebSize;
class WebURLRequest;
class WebURLResponse;
class WebUserContentURLPattern;

template<typename APIType> struct APITypeInfo { };
template<typename ImplType> struct ImplTypeInfo { };

#define WK_ADD_API_MAPPING(TheAPIType, TheImplType) \
    template<> struct APITypeInfo<TheAPIType> { typedef TheImplType* ImplType; }; \
    template<> struct ImplTypeInfo<TheImplType*> { typedef TheAPIType APIType; };

WK_ADD_API_MAPPING(WKArrayRef, ImmutableArray)
WK_ADD_API_MAPPING(WKBooleanRef, WebBoolean)
WK_ADD_API_MAPPING(WKCertificateInfoRef, WebCertificateInfo)
WK_ADD_API_MAPPING(WKConnectionRef, WebConnection)
WK_ADD_API_MAPPING(WKContextMenuItemRef, WebContextMenuItem)
WK_ADD_API_MAPPING(WKDataRef, WebData)
WK_ADD_API_MAPPING(WKDictionaryRef, ImmutableDictionary)
WK_ADD_API_MAPPING(WKDoubleRef, WebDouble)
WK_ADD_API_MAPPING(WKErrorRef, WebError)
WK_ADD_API_MAPPING(WKGraphicsContextRef, WebGraphicsContext)
WK_ADD_API_MAPPING(WKImageRef, WebImage)
WK_ADD_API_MAPPING(WKMutableArrayRef, MutableArray)
WK_ADD_API_MAPPING(WKMutableDictionaryRef, MutableDictionary)
WK_ADD_API_MAPPING(WKPointRef, WebPoint)
WK_ADD_API_MAPPING(WKRectRef, WebRect)
WK_ADD_API_MAPPING(WKSecurityOriginRef, WebSecurityOrigin)
WK_ADD_API_MAPPING(WKSerializedScriptValueRef, WebSerializedScriptValue)
WK_ADD_API_MAPPING(WKSizeRef, WebSize)
WK_ADD_API_MAPPING(WKStringRef, WebString)
WK_ADD_API_MAPPING(WKTypeRef, APIObject)
WK_ADD_API_MAPPING(WKUInt64Ref, WebUInt64)
WK_ADD_API_MAPPING(WKURLRef, WebURL)
WK_ADD_API_MAPPING(WKURLRequestRef, WebURLRequest)
WK_ADD_API_MAPPING(WKURLResponseRef, WebURLResponse)
WK_ADD_API_MAPPING(WKUserContentURLPatternRef, WebUserContentURLPattern)

#if PLATFORM(MAC)
WK_ADD_API_MAPPING(WKWebArchiveRef, WebArchive)
WK_ADD_API_MAPPING(WKWebArchiveResourceRef, WebArchiveResource)
WK_ADD_API_MAPPING(WKObjCTypeWrapperRef, ObjCObjectGraph)
#endif

template<typename ImplType, typename APIType = typename ImplTypeInfo<ImplType*>::APIType>
class ProxyingRefPtr {
public:
    ProxyingRefPtr(PassRefPtr<ImplType> impl)
        : m_impl(impl)
    {
    }

    operator APIType() { return toAPI(m_impl.get()); }

private:
    RefPtr<ImplType> m_impl;
};

/* Opaque typing convenience methods */

template<typename T>
inline typename APITypeInfo<T>::ImplType toImpl(T t)
{
    // An example of the conversions that take place:
    // const struct OpaqueWKArray* -> const struct OpaqueWKArray -> struct OpaqueWKArray -> struct OpaqueWKArray* -> ImmutableArray*
    
    typedef typename WTF::RemovePointer<T>::Type PotentiallyConstValueType;
    typedef typename WTF::RemoveConst<PotentiallyConstValueType>::Type NonConstValueType;

    return reinterpret_cast<typename APITypeInfo<T>::ImplType>(const_cast<NonConstValueType*>(t));
}

template<typename T>
inline typename ImplTypeInfo<T>::APIType toAPI(T t)
{
    return reinterpret_cast<typename ImplTypeInfo<T>::APIType>(t);
}

/* Special cases. */

inline ProxyingRefPtr<WebString> toAPI(StringImpl* string)
{
    return ProxyingRefPtr<WebString>(WebString::create(string));
}

inline WKStringRef toCopiedAPI(const String& string)
{
    RefPtr<WebString> webString = WebString::create(string);
    return toAPI(webString.release().leakRef());
}

inline ProxyingRefPtr<WebURL> toURLRef(StringImpl* string)
{
    if (!string)
        return ProxyingRefPtr<WebURL>(0);
    return ProxyingRefPtr<WebURL>(WebURL::create(String(string)));
}

inline WKURLRef toCopiedURLAPI(const String& string)
{
    if (!string)
        return 0;
    RefPtr<WebURL> webURL = WebURL::create(string);
    return toAPI(webURL.release().leakRef());
}

inline String toWTFString(WKStringRef stringRef)
{
    if (!stringRef)
        return String();
    return toImpl(stringRef)->string();
}

inline String toWTFString(WKURLRef urlRef)
{
    if (!urlRef)
        return String();
    return toImpl(urlRef)->string();
}

inline ProxyingRefPtr<WebError> toAPI(const WebCore::ResourceError& error)
{
    return ProxyingRefPtr<WebError>(WebError::create(error));
}

inline ProxyingRefPtr<WebURLRequest> toAPI(const WebCore::ResourceRequest& request)
{
    return ProxyingRefPtr<WebURLRequest>(WebURLRequest::create(request));
}

inline ProxyingRefPtr<WebURLResponse> toAPI(const WebCore::ResourceResponse& response)
{
    return ProxyingRefPtr<WebURLResponse>(WebURLResponse::create(response));
}

inline WKSecurityOriginRef toCopiedAPI(WebCore::SecurityOrigin* origin)
{
    if (!origin)
        return 0;
    return toAPI(WebSecurityOrigin::create(origin).leakRef());
}

/* Geometry conversions */

inline WebCore::FloatRect toFloatRect(const WKRect& wkRect)
{
    return WebCore::FloatRect(static_cast<float>(wkRect.origin.x), static_cast<float>(wkRect.origin.y),
                              static_cast<float>(wkRect.size.width), static_cast<float>(wkRect.size.height));
}

inline WebCore::IntSize toIntSize(const WKSize& wkSize)
{
    return WebCore::IntSize(static_cast<int>(wkSize.width), static_cast<int>(wkSize.height));
}

inline WebCore::IntPoint toIntPoint(const WKPoint& wkPoint)
{
    return WebCore::IntPoint(static_cast<int>(wkPoint.x), static_cast<int>(wkPoint.y));
}

inline WebCore::IntRect toIntRect(const WKRect& wkRect)
{
    return WebCore::IntRect(static_cast<int>(wkRect.origin.x), static_cast<int>(wkRect.origin.y),
                            static_cast<int>(wkRect.size.width), static_cast<int>(wkRect.size.height));
}

inline WKRect toAPI(const WebCore::FloatRect& rect)
{
    WKRect wkRect;
    wkRect.origin.x = rect.x();
    wkRect.origin.y = rect.y();
    wkRect.size.width = rect.width();
    wkRect.size.height = rect.height();
    return wkRect;
}

inline WKRect toAPI(const WebCore::IntRect& rect)
{
    WKRect wkRect;
    wkRect.origin.x = rect.x();
    wkRect.origin.y = rect.y();
    wkRect.size.width = rect.width();
    wkRect.size.height = rect.height();
    return wkRect;
}

inline WKSize toAPI(const WebCore::IntSize& size)
{
    WKSize wkSize;
    wkSize.width = size.width();
    wkSize.height = size.height();
    return wkSize;
}

inline WKPoint toAPI(const WebCore::IntPoint& point)
{
    WKPoint wkPoint;
    wkPoint.x = point.x();
    wkPoint.y = point.y();
    return wkPoint;
}

/* Enum conversions */

inline WKTypeID toAPI(APIObject::Type type)
{
    return static_cast<WKTypeID>(type);
}

inline WKEventModifiers toAPI(WebEvent::Modifiers modifiers)
{
    WKEventModifiers wkModifiers = 0;
    if (modifiers & WebEvent::ShiftKey)
        wkModifiers |= kWKEventModifiersShiftKey;
    if (modifiers & WebEvent::ControlKey)
        wkModifiers |= kWKEventModifiersControlKey;
    if (modifiers & WebEvent::AltKey)
        wkModifiers |= kWKEventModifiersAltKey;
    if (modifiers & WebEvent::MetaKey)
        wkModifiers |= kWKEventModifiersMetaKey;
    return wkModifiers;
}

inline WKEventMouseButton toAPI(WebMouseEvent::Button mouseButton)
{
    WKEventMouseButton wkMouseButton = kWKEventMouseButtonNoButton;

    switch (mouseButton) {
    case WebMouseEvent::NoButton:
        wkMouseButton = kWKEventMouseButtonNoButton;
        break;
    case WebMouseEvent::LeftButton:
        wkMouseButton = kWKEventMouseButtonLeftButton;
        break;
    case WebMouseEvent::MiddleButton:
        wkMouseButton = kWKEventMouseButtonMiddleButton;
        break;
    case WebMouseEvent::RightButton:
        wkMouseButton = kWKEventMouseButtonRightButton;
        break;
    }

    return wkMouseButton;
}

inline WKContextMenuItemTag toAPI(WebCore::ContextMenuAction action)
{
    switch (action) {
    case WebCore::ContextMenuItemTagNoAction:
        return kWKContextMenuItemTagNoAction;
    case WebCore::ContextMenuItemTagOpenLinkInNewWindow:
        return kWKContextMenuItemTagOpenLinkInNewWindow;
    case WebCore::ContextMenuItemTagDownloadLinkToDisk:
        return kWKContextMenuItemTagDownloadLinkToDisk;
    case WebCore::ContextMenuItemTagCopyLinkToClipboard:
        return kWKContextMenuItemTagCopyLinkToClipboard;
    case WebCore::ContextMenuItemTagOpenImageInNewWindow:
        return kWKContextMenuItemTagOpenImageInNewWindow;
    case WebCore::ContextMenuItemTagDownloadImageToDisk:
        return kWKContextMenuItemTagDownloadImageToDisk;
    case WebCore::ContextMenuItemTagCopyImageToClipboard:
        return kWKContextMenuItemTagCopyImageToClipboard;
#if PLATFORM(EFL) || PLATFORM(GTK) || PLATFORM(QT)
    case WebCore::ContextMenuItemTagCopyImageUrlToClipboard:
        return kWKContextMenuItemTagCopyImageUrlToClipboard;
#endif
    case WebCore::ContextMenuItemTagOpenFrameInNewWindow:
        return kWKContextMenuItemTagOpenFrameInNewWindow;
    case WebCore::ContextMenuItemTagCopy:
        return kWKContextMenuItemTagCopy;
    case WebCore::ContextMenuItemTagGoBack:
        return kWKContextMenuItemTagGoBack;
    case WebCore::ContextMenuItemTagGoForward:
        return kWKContextMenuItemTagGoForward;
    case WebCore::ContextMenuItemTagStop:
        return kWKContextMenuItemTagStop;
    case WebCore::ContextMenuItemTagReload:
        return kWKContextMenuItemTagReload;
    case WebCore::ContextMenuItemTagCut:
        return kWKContextMenuItemTagCut;
    case WebCore::ContextMenuItemTagPaste:
        return kWKContextMenuItemTagPaste;
#if PLATFORM(EFL) || PLATFORM(GTK) || PLATFORM(QT)
    case WebCore::ContextMenuItemTagSelectAll:
        return kWKContextMenuItemTagSelectAll;
#endif
    case WebCore::ContextMenuItemTagSpellingGuess:
        return kWKContextMenuItemTagSpellingGuess;
    case WebCore::ContextMenuItemTagNoGuessesFound:
        return kWKContextMenuItemTagNoGuessesFound;
    case WebCore::ContextMenuItemTagIgnoreSpelling:
        return kWKContextMenuItemTagIgnoreSpelling;
    case WebCore::ContextMenuItemTagLearnSpelling:
        return kWKContextMenuItemTagLearnSpelling;
    case WebCore::ContextMenuItemTagOther:
        return kWKContextMenuItemTagOther;
    case WebCore::ContextMenuItemTagSearchInSpotlight:
        return kWKContextMenuItemTagSearchInSpotlight;
    case WebCore::ContextMenuItemTagSearchWeb:
        return kWKContextMenuItemTagSearchWeb;
    case WebCore::ContextMenuItemTagLookUpInDictionary:
        return kWKContextMenuItemTagLookUpInDictionary;
    case WebCore::ContextMenuItemTagOpenWithDefaultApplication:
        return kWKContextMenuItemTagOpenWithDefaultApplication;
    case WebCore::ContextMenuItemPDFActualSize:
        return kWKContextMenuItemTagPDFActualSize;
    case WebCore::ContextMenuItemPDFZoomIn:
        return kWKContextMenuItemTagPDFZoomIn;
    case WebCore::ContextMenuItemPDFZoomOut:
        return kWKContextMenuItemTagPDFZoomOut;
    case WebCore::ContextMenuItemPDFAutoSize:
        return kWKContextMenuItemTagPDFAutoSize;
    case WebCore::ContextMenuItemPDFSinglePage:
        return kWKContextMenuItemTagPDFSinglePage;
    case WebCore::ContextMenuItemPDFFacingPages:
        return kWKContextMenuItemTagPDFFacingPages;
    case WebCore::ContextMenuItemPDFContinuous:
        return kWKContextMenuItemTagPDFContinuous;
    case WebCore::ContextMenuItemPDFNextPage:
        return kWKContextMenuItemTagPDFNextPage;
    case WebCore::ContextMenuItemPDFPreviousPage:
        return kWKContextMenuItemTagPDFPreviousPage;
    case WebCore::ContextMenuItemTagOpenLink:
        return kWKContextMenuItemTagOpenLink;
    case WebCore::ContextMenuItemTagIgnoreGrammar:
        return kWKContextMenuItemTagIgnoreGrammar;
    case WebCore::ContextMenuItemTagSpellingMenu:
        return kWKContextMenuItemTagSpellingMenu;
    case WebCore::ContextMenuItemTagShowSpellingPanel:
        return kWKContextMenuItemTagShowSpellingPanel;
    case WebCore::ContextMenuItemTagCheckSpelling:
        return kWKContextMenuItemTagCheckSpelling;
    case WebCore::ContextMenuItemTagCheckSpellingWhileTyping:
        return kWKContextMenuItemTagCheckSpellingWhileTyping;
    case WebCore::ContextMenuItemTagCheckGrammarWithSpelling:
        return kWKContextMenuItemTagCheckGrammarWithSpelling;
    case WebCore::ContextMenuItemTagFontMenu:
        return kWKContextMenuItemTagFontMenu;
    case WebCore::ContextMenuItemTagShowFonts:
        return kWKContextMenuItemTagShowFonts;
    case WebCore::ContextMenuItemTagBold:
        return kWKContextMenuItemTagBold;
    case WebCore::ContextMenuItemTagItalic:
        return kWKContextMenuItemTagItalic;
    case WebCore::ContextMenuItemTagUnderline:
        return kWKContextMenuItemTagUnderline;
    case WebCore::ContextMenuItemTagOutline:
        return kWKContextMenuItemTagOutline;
    case WebCore::ContextMenuItemTagStyles:
        return kWKContextMenuItemTagStyles;
    case WebCore::ContextMenuItemTagShowColors:
        return kWKContextMenuItemTagShowColors;
    case WebCore::ContextMenuItemTagSpeechMenu:
        return kWKContextMenuItemTagSpeechMenu;
    case WebCore::ContextMenuItemTagStartSpeaking:
        return kWKContextMenuItemTagStartSpeaking;
    case WebCore::ContextMenuItemTagStopSpeaking:
        return kWKContextMenuItemTagStopSpeaking;
    case WebCore::ContextMenuItemTagWritingDirectionMenu:
        return kWKContextMenuItemTagWritingDirectionMenu;
    case WebCore::ContextMenuItemTagDefaultDirection:
        return kWKContextMenuItemTagDefaultDirection;
    case WebCore::ContextMenuItemTagLeftToRight:
        return kWKContextMenuItemTagLeftToRight;
    case WebCore::ContextMenuItemTagRightToLeft:
        return kWKContextMenuItemTagRightToLeft;
    case WebCore::ContextMenuItemTagPDFSinglePageScrolling:
        return kWKContextMenuItemTagPDFSinglePageScrolling;
    case WebCore::ContextMenuItemTagPDFFacingPagesScrolling:
        return kWKContextMenuItemTagPDFFacingPagesScrolling;
    case WebCore::ContextMenuItemTagDictationAlternative:
        return kWKContextMenuItemTagDictationAlternative;
#if ENABLE(INSPECTOR)
    case WebCore::ContextMenuItemTagInspectElement:
        return kWKContextMenuItemTagInspectElement;
#endif
    case WebCore::ContextMenuItemTagTextDirectionMenu:
        return kWKContextMenuItemTagTextDirectionMenu;
    case WebCore::ContextMenuItemTagTextDirectionDefault:
        return kWKContextMenuItemTagTextDirectionDefault;
    case WebCore::ContextMenuItemTagTextDirectionLeftToRight:
        return kWKContextMenuItemTagTextDirectionLeftToRight;
    case WebCore::ContextMenuItemTagTextDirectionRightToLeft:
        return kWKContextMenuItemTagTextDirectionRightToLeft;
    case WebCore::ContextMenuItemTagOpenMediaInNewWindow:
        return kWKContextMenuItemTagOpenMediaInNewWindow;
    case WebCore::ContextMenuItemTagDownloadMediaToDisk:
        return kWKContextMenuItemTagDownloadMediaToDisk;
    case WebCore::ContextMenuItemTagCopyMediaLinkToClipboard:
        return kWKContextMenuItemTagCopyMediaLinkToClipboard;
    case WebCore::ContextMenuItemTagToggleMediaControls:
        return kWKContextMenuItemTagToggleMediaControls;
    case WebCore::ContextMenuItemTagToggleMediaLoop:
        return kWKContextMenuItemTagToggleMediaLoop;
    case WebCore::ContextMenuItemTagToggleVideoFullscreen:
        return kWKContextMenuItemTagToggleVideoFullscreen;
    case WebCore::ContextMenuItemTagEnterVideoFullscreen:
        return kWKContextMenuItemTagEnterVideoFullscreen;
    case WebCore::ContextMenuItemTagMediaPlayPause:
        return kWKContextMenuItemTagMediaPlayPause;
    case WebCore::ContextMenuItemTagMediaMute:
        return kWKContextMenuItemTagMediaMute;
#if PLATFORM(MAC)
    case WebCore::ContextMenuItemTagCorrectSpellingAutomatically:
        return kWKContextMenuItemTagCorrectSpellingAutomatically;
    case WebCore::ContextMenuItemTagSubstitutionsMenu:
        return kWKContextMenuItemTagSubstitutionsMenu;
    case WebCore::ContextMenuItemTagShowSubstitutions:
        return kWKContextMenuItemTagShowSubstitutions;
    case WebCore::ContextMenuItemTagSmartCopyPaste:
        return kWKContextMenuItemTagSmartCopyPaste;
    case WebCore::ContextMenuItemTagSmartQuotes:
        return kWKContextMenuItemTagSmartQuotes;
    case WebCore::ContextMenuItemTagSmartDashes:
        return kWKContextMenuItemTagSmartDashes;
    case WebCore::ContextMenuItemTagSmartLinks:
        return kWKContextMenuItemTagSmartLinks;
    case WebCore::ContextMenuItemTagTextReplacement:
        return kWKContextMenuItemTagTextReplacement;
    case WebCore::ContextMenuItemTagTransformationsMenu:
        return kWKContextMenuItemTagTransformationsMenu;
    case WebCore::ContextMenuItemTagMakeUpperCase:
        return kWKContextMenuItemTagMakeUpperCase;
    case WebCore::ContextMenuItemTagMakeLowerCase:
        return kWKContextMenuItemTagMakeLowerCase;
    case WebCore::ContextMenuItemTagCapitalize:
        return kWKContextMenuItemTagCapitalize;
    case WebCore::ContextMenuItemTagChangeBack:
        return kWKContextMenuItemTagChangeBack;
#endif
    case WebCore::ContextMenuItemTagOpenLinkInThisWindow:
        return kWKContextMenuItemTagOpenLinkInThisWindow;
    default:
        if (action < WebCore::ContextMenuItemBaseApplicationTag)
            LOG_ERROR("ContextMenuAction %i is an unknown tag but is below the allowable custom tag value of %i", action, WebCore::  ContextMenuItemBaseApplicationTag);
        return static_cast<WKContextMenuItemTag>(action);
    }
}

inline WebCore::ContextMenuAction toImpl(WKContextMenuItemTag tag)
{
    switch (tag) {
    case kWKContextMenuItemTagNoAction:
        return WebCore::ContextMenuItemTagNoAction;
    case kWKContextMenuItemTagOpenLinkInNewWindow:
        return WebCore::ContextMenuItemTagOpenLinkInNewWindow;
    case kWKContextMenuItemTagDownloadLinkToDisk:
        return WebCore::ContextMenuItemTagDownloadLinkToDisk;
    case kWKContextMenuItemTagCopyLinkToClipboard:
        return WebCore::ContextMenuItemTagCopyLinkToClipboard;
    case kWKContextMenuItemTagOpenImageInNewWindow:
        return WebCore::ContextMenuItemTagOpenImageInNewWindow;
    case kWKContextMenuItemTagDownloadImageToDisk:
        return WebCore::ContextMenuItemTagDownloadImageToDisk;
    case kWKContextMenuItemTagCopyImageToClipboard:
        return WebCore::ContextMenuItemTagCopyImageToClipboard;
    case kWKContextMenuItemTagOpenFrameInNewWindow:
#if PLATFORM(EFL) || PLATFORM(GTK) || PLATFORM(QT)
    case kWKContextMenuItemTagCopyImageUrlToClipboard:
        return WebCore::ContextMenuItemTagCopyImageUrlToClipboard;
#endif
        return WebCore::ContextMenuItemTagOpenFrameInNewWindow;
    case kWKContextMenuItemTagCopy:
        return WebCore::ContextMenuItemTagCopy;
    case kWKContextMenuItemTagGoBack:
        return WebCore::ContextMenuItemTagGoBack;
    case kWKContextMenuItemTagGoForward:
        return WebCore::ContextMenuItemTagGoForward;
    case kWKContextMenuItemTagStop:
        return WebCore::ContextMenuItemTagStop;
    case kWKContextMenuItemTagReload:
        return WebCore::ContextMenuItemTagReload;
    case kWKContextMenuItemTagCut:
        return WebCore::ContextMenuItemTagCut;
    case kWKContextMenuItemTagPaste:
        return WebCore::ContextMenuItemTagPaste;
#if PLATFORM(EFL) || PLATFORM(GTK) || PLATFORM(QT)
    case kWKContextMenuItemTagSelectAll:
        return WebCore::ContextMenuItemTagSelectAll;
#endif
    case kWKContextMenuItemTagSpellingGuess:
        return WebCore::ContextMenuItemTagSpellingGuess;
    case kWKContextMenuItemTagNoGuessesFound:
        return WebCore::ContextMenuItemTagNoGuessesFound;
    case kWKContextMenuItemTagIgnoreSpelling:
        return WebCore::ContextMenuItemTagIgnoreSpelling;
    case kWKContextMenuItemTagLearnSpelling:
        return WebCore::ContextMenuItemTagLearnSpelling;
    case kWKContextMenuItemTagOther:
        return WebCore::ContextMenuItemTagOther;
    case kWKContextMenuItemTagSearchInSpotlight:
        return WebCore::ContextMenuItemTagSearchInSpotlight;
    case kWKContextMenuItemTagSearchWeb:
        return WebCore::ContextMenuItemTagSearchWeb;
    case kWKContextMenuItemTagLookUpInDictionary:
        return WebCore::ContextMenuItemTagLookUpInDictionary;
    case kWKContextMenuItemTagOpenWithDefaultApplication:
        return WebCore::ContextMenuItemTagOpenWithDefaultApplication;
    case kWKContextMenuItemTagPDFActualSize:
        return WebCore::ContextMenuItemPDFActualSize;
    case kWKContextMenuItemTagPDFZoomIn:
        return WebCore::ContextMenuItemPDFZoomIn;
    case kWKContextMenuItemTagPDFZoomOut:
        return WebCore::ContextMenuItemPDFZoomOut;
    case kWKContextMenuItemTagPDFAutoSize:
        return WebCore::ContextMenuItemPDFAutoSize;
    case kWKContextMenuItemTagPDFSinglePage:
        return WebCore::ContextMenuItemPDFSinglePage;
    case kWKContextMenuItemTagPDFFacingPages:
        return WebCore::ContextMenuItemPDFFacingPages;
    case kWKContextMenuItemTagPDFContinuous:
        return WebCore::ContextMenuItemPDFContinuous;
    case kWKContextMenuItemTagPDFNextPage:
        return WebCore::ContextMenuItemPDFNextPage;
    case kWKContextMenuItemTagPDFPreviousPage:
        return WebCore::ContextMenuItemPDFPreviousPage;
    case kWKContextMenuItemTagOpenLink:
        return WebCore::ContextMenuItemTagOpenLink;
    case kWKContextMenuItemTagIgnoreGrammar:
        return WebCore::ContextMenuItemTagIgnoreGrammar;
    case kWKContextMenuItemTagSpellingMenu:
        return WebCore::ContextMenuItemTagSpellingMenu;
    case kWKContextMenuItemTagShowSpellingPanel:
        return WebCore::ContextMenuItemTagShowSpellingPanel;
    case kWKContextMenuItemTagCheckSpelling:
        return WebCore::ContextMenuItemTagCheckSpelling;
    case kWKContextMenuItemTagCheckSpellingWhileTyping:
        return WebCore::ContextMenuItemTagCheckSpellingWhileTyping;
    case kWKContextMenuItemTagCheckGrammarWithSpelling:
        return WebCore::ContextMenuItemTagCheckGrammarWithSpelling;
    case kWKContextMenuItemTagFontMenu:
        return WebCore::ContextMenuItemTagFontMenu;
    case kWKContextMenuItemTagShowFonts:
        return WebCore::ContextMenuItemTagShowFonts;
    case kWKContextMenuItemTagBold:
        return WebCore::ContextMenuItemTagBold;
    case kWKContextMenuItemTagItalic:
        return WebCore::ContextMenuItemTagItalic;
    case kWKContextMenuItemTagUnderline:
        return WebCore::ContextMenuItemTagUnderline;
    case kWKContextMenuItemTagOutline:
        return WebCore::ContextMenuItemTagOutline;
    case kWKContextMenuItemTagStyles:
        return WebCore::ContextMenuItemTagStyles;
    case kWKContextMenuItemTagShowColors:
        return WebCore::ContextMenuItemTagShowColors;
    case kWKContextMenuItemTagSpeechMenu:
        return WebCore::ContextMenuItemTagSpeechMenu;
    case kWKContextMenuItemTagStartSpeaking:
        return WebCore::ContextMenuItemTagStartSpeaking;
    case kWKContextMenuItemTagStopSpeaking:
        return WebCore::ContextMenuItemTagStopSpeaking;
    case kWKContextMenuItemTagWritingDirectionMenu:
        return WebCore::ContextMenuItemTagWritingDirectionMenu;
    case kWKContextMenuItemTagDefaultDirection:
        return WebCore::ContextMenuItemTagDefaultDirection;
    case kWKContextMenuItemTagLeftToRight:
        return WebCore::ContextMenuItemTagLeftToRight;
    case kWKContextMenuItemTagRightToLeft:
        return WebCore::ContextMenuItemTagRightToLeft;
    case kWKContextMenuItemTagPDFSinglePageScrolling:
        return WebCore::ContextMenuItemTagPDFSinglePageScrolling;
    case kWKContextMenuItemTagPDFFacingPagesScrolling:
        return WebCore::ContextMenuItemTagPDFFacingPagesScrolling;
    case kWKContextMenuItemTagDictationAlternative:
        return WebCore::ContextMenuItemTagDictationAlternative;
#if ENABLE(INSPECTOR)
    case kWKContextMenuItemTagInspectElement:
        return WebCore::ContextMenuItemTagInspectElement;
#endif
    case kWKContextMenuItemTagTextDirectionMenu:
        return WebCore::ContextMenuItemTagTextDirectionMenu;
    case kWKContextMenuItemTagTextDirectionDefault:
        return WebCore::ContextMenuItemTagTextDirectionDefault;
    case kWKContextMenuItemTagTextDirectionLeftToRight:
        return WebCore::ContextMenuItemTagTextDirectionLeftToRight;
    case kWKContextMenuItemTagTextDirectionRightToLeft:
        return WebCore::ContextMenuItemTagTextDirectionRightToLeft;
    case kWKContextMenuItemTagOpenMediaInNewWindow:
        return WebCore::ContextMenuItemTagOpenMediaInNewWindow;
    case kWKContextMenuItemTagDownloadMediaToDisk:
        return WebCore::ContextMenuItemTagDownloadMediaToDisk;
    case kWKContextMenuItemTagCopyMediaLinkToClipboard:
        return WebCore::ContextMenuItemTagCopyMediaLinkToClipboard;
    case kWKContextMenuItemTagToggleMediaControls:
        return WebCore::ContextMenuItemTagToggleMediaControls;
    case kWKContextMenuItemTagToggleMediaLoop:
        return WebCore::ContextMenuItemTagToggleMediaLoop;
    case kWKContextMenuItemTagToggleVideoFullscreen:
        return WebCore::ContextMenuItemTagToggleVideoFullscreen;
    case kWKContextMenuItemTagEnterVideoFullscreen:
        return WebCore::ContextMenuItemTagEnterVideoFullscreen;
    case kWKContextMenuItemTagMediaPlayPause:
        return WebCore::ContextMenuItemTagMediaPlayPause;
    case kWKContextMenuItemTagMediaMute:
        return WebCore::ContextMenuItemTagMediaMute;
#if PLATFORM(MAC)
    case kWKContextMenuItemTagCorrectSpellingAutomatically:
        return WebCore::ContextMenuItemTagCorrectSpellingAutomatically;
    case kWKContextMenuItemTagSubstitutionsMenu:
        return WebCore::ContextMenuItemTagSubstitutionsMenu;
    case kWKContextMenuItemTagShowSubstitutions:
        return WebCore::ContextMenuItemTagShowSubstitutions;
    case kWKContextMenuItemTagSmartCopyPaste:
        return WebCore::ContextMenuItemTagSmartCopyPaste;
    case kWKContextMenuItemTagSmartQuotes:
        return WebCore::ContextMenuItemTagSmartQuotes;
    case kWKContextMenuItemTagSmartDashes:
        return WebCore::ContextMenuItemTagSmartDashes;
    case kWKContextMenuItemTagSmartLinks:
        return WebCore::ContextMenuItemTagSmartLinks;
    case kWKContextMenuItemTagTextReplacement:
        return WebCore::ContextMenuItemTagTextReplacement;
    case kWKContextMenuItemTagTransformationsMenu:
        return WebCore::ContextMenuItemTagTransformationsMenu;
    case kWKContextMenuItemTagMakeUpperCase:
        return WebCore::ContextMenuItemTagMakeUpperCase;
    case kWKContextMenuItemTagMakeLowerCase:
        return WebCore::ContextMenuItemTagMakeLowerCase;
    case kWKContextMenuItemTagCapitalize:
        return WebCore::ContextMenuItemTagCapitalize;
    case kWKContextMenuItemTagChangeBack:
        return WebCore::ContextMenuItemTagChangeBack;
#endif
    case kWKContextMenuItemTagOpenLinkInThisWindow:
        return WebCore::ContextMenuItemTagOpenLinkInThisWindow;
    default:
        if (tag < kWKContextMenuItemBaseApplicationTag)
            LOG_ERROR("WKContextMenuItemTag %i is an unknown tag but is below the allowable custom tag value of %i", tag, kWKContextMenuItemBaseApplicationTag);
        return static_cast<WebCore::ContextMenuAction>(tag);
    }
}

inline WKContextMenuItemType toAPI(WebCore::ContextMenuItemType type)
{
    switch(type) {
    case WebCore::ActionType:
        return kWKContextMenuItemTypeAction;
    case WebCore::CheckableActionType:
        return kWKContextMenuItemTypeCheckableAction;
    case WebCore::SeparatorType:
        return kWKContextMenuItemTypeSeparator;
    case WebCore::SubmenuType:
        return kWKContextMenuItemTypeSubmenu;
    default:
        ASSERT_NOT_REACHED();
        return kWKContextMenuItemTypeAction;
    }
}

inline FindOptions toFindOptions(WKFindOptions wkFindOptions)
{
    unsigned findOptions = 0;

    if (wkFindOptions & kWKFindOptionsCaseInsensitive)
        findOptions |= FindOptionsCaseInsensitive;
    if (wkFindOptions & kWKFindOptionsAtWordStarts)
        findOptions |= FindOptionsAtWordStarts;
    if (wkFindOptions & kWKFindOptionsTreatMedialCapitalAsWordStart)
        findOptions |= FindOptionsTreatMedialCapitalAsWordStart;
    if (wkFindOptions & kWKFindOptionsBackwards)
        findOptions |= FindOptionsBackwards;
    if (wkFindOptions & kWKFindOptionsWrapAround)
        findOptions |= FindOptionsWrapAround;
    if (wkFindOptions & kWKFindOptionsShowOverlay)
        findOptions |= FindOptionsShowOverlay;
    if (wkFindOptions & kWKFindOptionsShowFindIndicator)
        findOptions |= FindOptionsShowFindIndicator;
    if (wkFindOptions & kWKFindOptionsShowHighlight)
        findOptions |= FindOptionsShowHighlight;

    return static_cast<FindOptions>(findOptions);
}

inline WKFrameNavigationType toAPI(WebCore::NavigationType type)
{
    WKFrameNavigationType wkType = kWKFrameNavigationTypeOther;

    switch (type) {
    case WebCore::NavigationTypeLinkClicked:
        wkType = kWKFrameNavigationTypeLinkClicked;
        break;
    case WebCore::NavigationTypeFormSubmitted:
        wkType = kWKFrameNavigationTypeFormSubmitted;
        break;
    case WebCore::NavigationTypeBackForward:
        wkType = kWKFrameNavigationTypeBackForward;
        break;
    case WebCore::NavigationTypeReload:
        wkType = kWKFrameNavigationTypeReload;
        break;
    case WebCore::NavigationTypeFormResubmitted:
        wkType = kWKFrameNavigationTypeFormResubmitted;
        break;
    case WebCore::NavigationTypeOther:
        wkType = kWKFrameNavigationTypeOther;
        break;
    }
    
    return wkType;
}

inline WKSameDocumentNavigationType toAPI(SameDocumentNavigationType type)
{
    WKFrameNavigationType wkType = kWKSameDocumentNavigationAnchorNavigation;

    switch (type) {
    case SameDocumentNavigationAnchorNavigation:
        wkType = kWKSameDocumentNavigationAnchorNavigation;
        break;
    case SameDocumentNavigationSessionStatePush:
        wkType = kWKSameDocumentNavigationSessionStatePush;
        break;
    case SameDocumentNavigationSessionStateReplace:
        wkType = kWKSameDocumentNavigationSessionStateReplace;
        break;
    case SameDocumentNavigationSessionStatePop:
        wkType = kWKSameDocumentNavigationSessionStatePop;
        break;
    }
    
    return wkType;
}

inline WKLayoutMilestones toWKLayoutMilestones(WebCore::LayoutMilestones milestones)
{
    unsigned wkMilestones = 0;

    if (milestones & WebCore::DidFirstLayout)
        wkMilestones |= kWKDidFirstLayout;
    if (milestones & WebCore::DidFirstVisuallyNonEmptyLayout)
        wkMilestones |= kWKDidFirstVisuallyNonEmptyLayout;
    if (milestones & WebCore::DidHitRelevantRepaintedObjectsAreaThreshold)
        wkMilestones |= kWKDidHitRelevantRepaintedObjectsAreaThreshold;
    if (milestones & WebCore::DidFirstFlushForHeaderLayer)
        wkMilestones |= kWKDidFirstFlushForHeaderLayer;
    if (milestones & WebCore::DidFirstLayoutAfterSuppressedIncrementalRendering)
        wkMilestones |= kWKDidFirstLayoutAfterSuppressedIncrementalRendering;
    if (milestones & WebCore::DidFirstPaintAfterSuppressedIncrementalRendering)
        wkMilestones |= kWKDidFirstPaintAfterSuppressedIncrementalRendering;
    
    return wkMilestones;
}

inline WebCore::LayoutMilestones toLayoutMilestones(WKLayoutMilestones wkMilestones)
{
    WebCore::LayoutMilestones milestones = 0;

    if (wkMilestones & kWKDidFirstLayout)
        milestones |= WebCore::DidFirstLayout;
    if (wkMilestones & kWKDidFirstVisuallyNonEmptyLayout)
        milestones |= WebCore::DidFirstVisuallyNonEmptyLayout;
    if (wkMilestones & kWKDidHitRelevantRepaintedObjectsAreaThreshold)
        milestones |= WebCore::DidHitRelevantRepaintedObjectsAreaThreshold;
    if (wkMilestones & kWKDidFirstFlushForHeaderLayer)
        milestones |= WebCore::DidFirstFlushForHeaderLayer;
    if (wkMilestones & kWKDidFirstLayoutAfterSuppressedIncrementalRendering)
        milestones |= WebCore::DidFirstLayoutAfterSuppressedIncrementalRendering;
    if (wkMilestones & kWKDidFirstPaintAfterSuppressedIncrementalRendering)
        milestones |= WebCore::DidFirstPaintAfterSuppressedIncrementalRendering;
    
    return milestones;
}

inline WebCore::PageVisibilityState toPageVisibilityState(WKPageVisibilityState wkPageVisibilityState)
{
    switch (wkPageVisibilityState) {
    case kWKPageVisibilityStateVisible:
        return WebCore::PageVisibilityStateVisible;
    case kWKPageVisibilityStateHidden:
        return WebCore::PageVisibilityStateHidden;
    case kWKPageVisibilityStatePrerender:
        return WebCore::PageVisibilityStatePrerender;
    case kWKPageVisibilityStateUnloaded:
        return WebCore::PageVisibilityStateUnloaded;
    }

    ASSERT_NOT_REACHED();
    return WebCore::PageVisibilityStateVisible;
}

inline ImageOptions toImageOptions(WKImageOptions wkImageOptions)
{
    unsigned imageOptions = 0;

    if (wkImageOptions & kWKImageOptionsShareable)
        imageOptions |= ImageOptionsShareable;

    return static_cast<ImageOptions>(imageOptions);
}

inline SnapshotOptions snapshotOptionsFromImageOptions(WKImageOptions wkImageOptions)
{
    unsigned snapshotOptions = 0;

    if (wkImageOptions & kWKImageOptionsShareable)
        snapshotOptions |= SnapshotOptionsShareable;

    return snapshotOptions;
}

inline SnapshotOptions toSnapshotOptions(WKSnapshotOptions wkSnapshotOptions)
{
    unsigned snapshotOptions = 0;

    if (wkSnapshotOptions & kWKSnapshotOptionsShareable)
        snapshotOptions |= SnapshotOptionsShareable;
    if (wkSnapshotOptions & kWKSnapshotOptionsExcludeSelectionHighlighting)
        snapshotOptions |= SnapshotOptionsExcludeSelectionHighlighting;
    if (wkSnapshotOptions & kWKSnapshotOptionsInViewCoordinates)
        snapshotOptions |= SnapshotOptionsInViewCoordinates;
    if (wkSnapshotOptions & kWKSnapshotOptionsPaintSelectionRectangle)
        snapshotOptions |= SnapshotOptionsPaintSelectionRectangle;

    return snapshotOptions;
}

inline WebCore::UserScriptInjectionTime toUserScriptInjectionTime(WKUserScriptInjectionTime wkInjectedTime)
{
    switch (wkInjectedTime) {
    case kWKInjectAtDocumentStart:
        return WebCore::InjectAtDocumentStart;
    case kWKInjectAtDocumentEnd:
        return WebCore::InjectAtDocumentEnd;
    }

    ASSERT_NOT_REACHED();
    return WebCore::InjectAtDocumentStart;
}

inline WebCore::UserContentInjectedFrames toUserContentInjectedFrames(WKUserContentInjectedFrames wkInjectedFrames)
{
    switch (wkInjectedFrames) {
    case kWKInjectInAllFrames:
        return WebCore::InjectInAllFrames;
    case kWKInjectInTopFrameOnly:
        return WebCore::InjectInTopFrameOnly;
    }

    ASSERT_NOT_REACHED();
    return WebCore::InjectInAllFrames;
}

} // namespace WebKit

#endif // WKSharedAPICast_h

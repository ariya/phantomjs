/*
 * Copyright (C) 2009, 2010 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ResourceRequest_h
#define ResourceRequest_h

#include "ResourceRequestBase.h"

namespace BlackBerry {
namespace Platform {
class NetworkRequest;
}
}

namespace WebCore {

class ResourceRequest : public ResourceRequestBase {
public:
    // The type of this ResourceRequest, based on how the resource will be used.
    enum TargetType {
        TargetIsMainFrame,
        TargetIsSubframe,
        TargetIsSubresource, // Resource is a generic subresource. (Generally a specific type should be specified)
        TargetIsStyleSheet,
        TargetIsScript,
        TargetIsFontResource,
        TargetIsImage,
        TargetIsObject,
        TargetIsMedia,
        TargetIsWorker,
        TargetIsSharedWorker,
        TargetIsPrefetch,
        TargetIsFavicon,
        TargetIsXHR,
        TargetIsTextTrack,
        TargetIsUnspecified,
    };
    ResourceRequest(const String& url)
        : ResourceRequestBase(KURL(ParsedURLString, url), UseProtocolCachePolicy)
        , m_isXMLHTTPRequest(false)
        , m_mustHandleInternally(false)
        , m_forceDownload(false)
        , m_targetType(TargetIsUnspecified)
    {
    }

    ResourceRequest(const KURL& url)
        : ResourceRequestBase(url, UseProtocolCachePolicy)
        , m_isXMLHTTPRequest(false)
        , m_mustHandleInternally(false)
        , m_forceDownload(false)
        , m_targetType(TargetIsUnspecified)
    {
    }

    ResourceRequest(const KURL& url, const String& referrer, ResourceRequestCachePolicy policy = UseProtocolCachePolicy)
        : ResourceRequestBase(url, policy)
        , m_isXMLHTTPRequest(false)
        , m_mustHandleInternally(false)
        , m_forceDownload(false)
        , m_targetType(TargetIsUnspecified)
    {
        setHTTPReferrer(referrer);
    }

    ResourceRequest()
        : ResourceRequestBase(KURL(), UseProtocolCachePolicy)
        , m_isXMLHTTPRequest(false)
        , m_mustHandleInternally(false)
        , m_forceDownload(false)
        , m_targetType(TargetIsUnspecified)
    {
    }

    void setToken(const String& token) { m_token = token; }
    String token() const { return m_token; }

    // FIXME: For RIM Bug #452. The BlackBerry application wants the anchor text for a clicked hyperlink so as to
    // make an informed decision as to whether to allow the navigation. We should move this functionality into a
    // UI/Policy delegate.
    void setAnchorText(const String& anchorText) { m_anchorText = anchorText; }
    String anchorText() const { return m_anchorText; }

    void setOverrideContentType(const String& contentType) { m_overrideContentType = contentType; }
    String overrideContentType() const { return m_overrideContentType; }

    void setIsXMLHTTPRequest(bool isXMLHTTPRequest) { m_isXMLHTTPRequest = isXMLHTTPRequest; }
    bool isXMLHTTPRequest() const { return m_isXMLHTTPRequest; }

    // Marks requests which must be handled by webkit even if LinksHandledExternally is set.
    void setMustHandleInternally(bool mustHandleInternally) { m_mustHandleInternally = mustHandleInternally; }
    bool mustHandleInternally() const { return m_mustHandleInternally; }

    void initializePlatformRequest(BlackBerry::Platform::NetworkRequest&, bool cookiesEnabled, bool isInitial = false, bool rereadCookies = false) const;
    void setForceDownload(bool forceDownload) { m_forceDownload = forceDownload; }
    bool forceDownload() const { return m_forceDownload; }
    void setSuggestedSaveName(const String& name) { m_suggestedSaveName = name; }
    String suggestedSaveName() const { return m_suggestedSaveName; }

    // What this request is for.
    TargetType targetType() const { return m_targetType; }
    void setTargetType(TargetType type) { m_targetType = type; }

    static TargetType targetTypeFromMimeType(const String& mimeType);

    void clearHTTPContentLength();
    void clearHTTPContentType();

private:
    friend class ResourceRequestBase;

    String m_token;
    String m_anchorText;
    String m_overrideContentType;
    String m_suggestedSaveName;
    bool m_isXMLHTTPRequest;
    bool m_mustHandleInternally;
    bool m_forceDownload;
    TargetType m_targetType;

    void doUpdatePlatformRequest() { }
    void doUpdateResourceRequest() { }
    void doUpdatePlatformHTTPBody() { }
    void doUpdateResourceHTTPBody() { }

    PassOwnPtr<CrossThreadResourceRequestData> doPlatformCopyData(PassOwnPtr<CrossThreadResourceRequestData>) const;
    void doPlatformAdopt(PassOwnPtr<CrossThreadResourceRequestData>);
};

struct CrossThreadResourceRequestData : public CrossThreadResourceRequestDataBase {
    String m_token;
    String m_anchorText;
    String m_overrideContentType;
    String m_suggestedSaveName;
    bool m_isXMLHTTPRequest;
    bool m_mustHandleInternally;
    bool m_forceDownload;
    ResourceRequest::TargetType m_targetType;
};

} // namespace WebCore

#endif // ResourceRequest_h

/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "ResourceRequest.h"

#include "BlobRegistryImpl.h"
#include "CookieManager.h"
#include <network/NetworkRequest.h>
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>

using BlackBerry::Platform::NetworkRequest;

namespace WebCore {

unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    return 10000;
}

static inline NetworkRequest::CachePolicy platformCachePolicyForRequest(const ResourceRequest& request)
{
    switch (request.cachePolicy()) {
    case WebCore::UseProtocolCachePolicy:
        return NetworkRequest::UseProtocolCachePolicy;
    case WebCore::ReloadIgnoringCacheData:
        return NetworkRequest::ReloadIgnoringCacheData;
    case WebCore::ReturnCacheDataElseLoad:
        return NetworkRequest::ReturnCacheDataElseLoad;
    case WebCore::ReturnCacheDataDontLoad:
        return NetworkRequest::ReturnCacheDataDontLoad;
    default:
        return NetworkRequest::UseProtocolCachePolicy;
    }
}

static inline NetworkRequest::TargetType platformTargetTypeForRequest(const ResourceRequest& request)
{
    if (request.isXMLHTTPRequest())
        return NetworkRequest::TargetIsXMLHTTPRequest;
    if (request.forceDownload())
        return NetworkRequest::TargetIsDownload;

    switch (request.targetType()) {
    case ResourceRequest::TargetIsMainFrame:
        return NetworkRequest::TargetIsMainFrame;
    case ResourceRequest::TargetIsSubframe:
        return NetworkRequest::TargetIsSubframe;
    case ResourceRequest::TargetIsSubresource:
        return NetworkRequest::TargetIsSubresource;
    case ResourceRequest::TargetIsStyleSheet:
        return NetworkRequest::TargetIsStyleSheet;
    case ResourceRequest::TargetIsScript:
        return NetworkRequest::TargetIsScript;
    case ResourceRequest::TargetIsFontResource:
        return NetworkRequest::TargetIsFontResource;
    case ResourceRequest::TargetIsImage:
        return NetworkRequest::TargetIsImage;
    case ResourceRequest::TargetIsObject:
        return NetworkRequest::TargetIsObject;
    case ResourceRequest::TargetIsMedia:
        return NetworkRequest::TargetIsMedia;
    case ResourceRequest::TargetIsWorker:
        return NetworkRequest::TargetIsWorker;
    case ResourceRequest::TargetIsSharedWorker:
        return NetworkRequest::TargetIsSharedWorker;

        // FIXME: this need to be updated to the right value, but
        // we need to coordinate with AIR api change.
    case ResourceRequest::TargetIsFavicon:
        return NetworkRequest::TargetIsImage;
    case ResourceRequest::TargetIsPrefetch:
        return NetworkRequest::TargetIsSubresource;
    case ResourceRequest::TargetIsXHR:
        return NetworkRequest::TargetIsSubresource;
    case ResourceRequest::TargetIsTextTrack:
        return NetworkRequest::TargetIsSubresource;
    case ResourceRequest::TargetIsUnspecified:
        return NetworkRequest::TargetIsSubresource;

    default:
        ASSERT_NOT_REACHED();
        return NetworkRequest::TargetIsUnknown;
    }
}

typedef HashMap<String, ResourceRequest::TargetType> MimeTypeResourceRequestTypeMap;

static const MimeTypeResourceRequestTypeMap& mimeTypeRequestTypeMap()
{
    static MimeTypeResourceRequestTypeMap* map = 0;
    if (!map) {
        map = new MimeTypeResourceRequestTypeMap;

        if (map) {
            // The list here should match extensionMap[] in MIMETypeRegistryBlackBerry.cpp
            map->add(String("text/css"), ResourceRequest::TargetIsStyleSheet);
            map->add(String("application/x-javascript"), ResourceRequest::TargetIsScript);
            map->add(String("image/bmp"), ResourceRequest::TargetIsImage);
            map->add(String("image/gif"), ResourceRequest::TargetIsImage);
            map->add(String("image/x-icon"), ResourceRequest::TargetIsImage);
            map->add(String("image/jpeg"), ResourceRequest::TargetIsImage);
            map->add(String("image/png"), ResourceRequest::TargetIsImage);
            map->add(String("image/x-portable-bitmap"), ResourceRequest::TargetIsImage);
            map->add(String("image/x-portable-graymap"), ResourceRequest::TargetIsImage);
            map->add(String("image/x-portable-pixmap"), ResourceRequest::TargetIsImage);
            map->add(String("image/svg+xml"), ResourceRequest::TargetIsImage);
            map->add(String("image/tiff"), ResourceRequest::TargetIsImage);
            map->add(String("image/x-xbitmap"), ResourceRequest::TargetIsImage);
            map->add(String("image/x-xpm"), ResourceRequest::TargetIsImage);
        }
    }

    return *map;
}

ResourceRequest::TargetType ResourceRequest::targetTypeFromMimeType(const String& mimeType)
{
    const MimeTypeResourceRequestTypeMap& map = mimeTypeRequestTypeMap();

    MimeTypeResourceRequestTypeMap::const_iterator iter = map.find(mimeType);
    if (iter == map.end())
        return ResourceRequest::TargetIsUnspecified;

    return iter->value;
}

void ResourceRequest::initializePlatformRequest(NetworkRequest& platformRequest, bool cookiesEnabled, bool isInitial, bool rereadCookies) const
{
    // If this is the initial load, skip the request body and headers.
    if (isInitial)
        platformRequest.setRequestInitial(timeoutInterval());
    else {
        platformRequest.setRequestUrl(url().string(),
            httpMethod(),
                platformCachePolicyForRequest(*this),
                platformTargetTypeForRequest(*this),
                timeoutInterval());

        platformRequest.setConditional(isConditional());
        platformRequest.setSuggestedSaveName(suggestedSaveName());

        if (httpBody() && !httpBody()->isEmpty()) {
            RefPtr<FormData> formData = httpBody();
#if ENABLE(BLOB)
            formData = formData->resolveBlobReferences();
#endif
            const Vector<FormDataElement>& elements = formData->elements();
            // Use setData for simple forms because it is slightly more efficient.
            if (elements.size() == 1 && elements[0].m_type == FormDataElement::data)
                platformRequest.setData(elements[0].m_data.data(), elements[0].m_data.size());
            else {
                for (unsigned i = 0; i < elements.size(); ++i) {
                    const FormDataElement& element = elements[i];
                    if (element.m_type == FormDataElement::data)
                        platformRequest.addMultipartData(element.m_data.data(), element.m_data.size());
                    else if (element.m_type == FormDataElement::encodedFile)
                        platformRequest.addMultipartFilename(element.m_filename.characters(), element.m_filename.length());
                    else
                        ASSERT_NOT_REACHED(); // Blobs should be resolved at this point.
                }
            }
        }

        // When ResourceRequest is reused by CacheResourceLoader, page refreshing or redirection, its cookies may be dirtied. We won't use these cookies any more.
        bool cookieHeaderMayBeDirty = rereadCookies || cachePolicy() == WebCore::ReloadIgnoringCacheData || cachePolicy() == WebCore::ReturnCacheDataElseLoad;

        for (HTTPHeaderMap::const_iterator it = httpHeaderFields().begin(); it != httpHeaderFields().end(); ++it) {
            String key = it->key;
            String value = it->value;
            if (!key.isEmpty()) {
                if (equalIgnoringCase(key, "Cookie")) {
                    // We won't use the old cookies of resourceRequest for new location because these cookies may be changed by redirection.
                    if (cookieHeaderMayBeDirty)
                        continue;
                    // We need to check the encoding and encode the cookie's value using latin1 or utf8 to support unicode data.
                    if (!value.containsOnlyLatin1()) {
                        platformRequest.addHeader("Cookie", value.utf8().data());
                        continue;
                    }
                }
                platformRequest.addHeader(key, value);
            }
        }

        // If request's cookies may be dirty, they must be set again.
        // If there aren't cookies in the header list, we need trying to add cookies.
        if (cookiesEnabled && (cookieHeaderMayBeDirty || !httpHeaderFields().contains("Cookie")) && !url().isNull()) {
            // Prepare a cookie header if there are cookies related to this url.
            String cookiePairs = cookieManager().getCookie(url(), WithHttpOnlyCookies);
            if (!cookiePairs.isEmpty())
                platformRequest.addHeader("Cookie", cookiePairs.containsOnlyLatin1() ? cookiePairs.latin1().data() : cookiePairs.utf8().data());
        }

        if (!httpHeaderFields().contains("Accept-Language"))
            platformRequest.addAcceptLanguageHeader();
    }
}

PassOwnPtr<CrossThreadResourceRequestData> ResourceRequest::doPlatformCopyData(PassOwnPtr<CrossThreadResourceRequestData> data) const
{
    data->m_token = m_token;
    data->m_anchorText = m_anchorText;
    data->m_overrideContentType = m_overrideContentType;
    data->m_suggestedSaveName = m_suggestedSaveName;
    data->m_isXMLHTTPRequest = m_isXMLHTTPRequest;
    data->m_mustHandleInternally = m_mustHandleInternally;
    data->m_forceDownload = m_forceDownload;
    data->m_targetType = m_targetType;
    return data;
}

void ResourceRequest::doPlatformAdopt(PassOwnPtr<CrossThreadResourceRequestData> data)
{
    m_token = data->m_token;
    m_anchorText = data->m_anchorText;
    m_overrideContentType = data->m_overrideContentType;
    m_suggestedSaveName = data->m_suggestedSaveName;
    m_isXMLHTTPRequest = data->m_isXMLHTTPRequest;
    m_mustHandleInternally = data->m_mustHandleInternally;
    m_forceDownload = data->m_forceDownload;
    m_targetType = data->m_targetType;
}

void ResourceRequest::clearHTTPContentLength()
{
    updateResourceRequest();

    m_httpHeaderFields.remove("Content-Length");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequest::clearHTTPContentType()
{
    updateResourceRequest();

    m_httpHeaderFields.remove("Content-Type");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

} // namespace WebCore

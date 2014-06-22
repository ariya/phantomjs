/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
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
#include "NetworkJob.h"

#include "AuthenticationChallengeManager.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "CookieManager.h"
#include "CredentialBackingStore.h"
#include "CredentialStorage.h"
#include "Frame.h"
#include "FrameLoaderClientBlackBerry.h"
#include "HTTPParsers.h"
#include "KURL.h"
#include "MIMESniffing.h"
#include "MIMETypeRegistry.h"
#include "NetworkManager.h"
#include "Page.h"
#include "RSSFilterStream.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"

#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformSettings.h>
#include <LocalizeResource.h>
#include <network/MultipartStream.h>
#include <network/NetworkStreamFactory.h>

using BlackBerry::Platform::NetworkRequest;

namespace WebCore {

static const int s_redirectMaximum = 10;

inline static bool isInfo(int statusCode)
{
    return 100 <= statusCode && statusCode < 200;
}

inline static bool isRedirect(int statusCode)
{
    return 300 <= statusCode && statusCode < 400 && statusCode != 304;
}

inline static bool isUnauthorized(int statusCode)
{
    return statusCode == 401;
}

static const char* const appendableHeaders[] = {"access-control-allow-origin", "allow",
    "set-cookie", "set-cookie2", "vary", "via", "warning"};

static bool isAppendableHeader(const String& key)
{
    // Non-standard header fields are conventionally marked by prefixing the field name with X-.
    if (key.startsWith("x-"))
        return true;

    for (size_t i = 0; i < sizeof(appendableHeaders) /sizeof(char*); i++)
        if (key == appendableHeaders[i])
            return true;

    return false;
}

NetworkJob::NetworkJob()
    : FrameDestructionObserver(0)
    , m_playerId(0)
    , m_deleteJobTimer(this, &NetworkJob::fireDeleteJobTimer)
    , m_streamFactory(0)
    , m_isFile(false)
    , m_isFTP(false)
    , m_isFTPDir(true)
#ifndef NDEBUG
    , m_isRunning(true) // Always started immediately after creation.
#endif
    , m_cancelled(false)
    , m_statusReceived(false)
    , m_dataReceived(false)
    , m_responseSent(false)
    , m_callingClient(false)
    , m_needsRetryAsFTPDirectory(false)
    , m_isOverrideContentType(false)
    , m_newJobWithCredentialsStarted(false)
    , m_isHeadMethod(false)
    , m_extendedStatusCode(0)
    , m_redirectCount(0)
    , m_deferredData(*this)
    , m_deferLoadingCount(0)
    , m_isAuthenticationChallenging(false)
{
}

NetworkJob::~NetworkJob()
{
    if (m_isAuthenticationChallenging)
        AuthenticationChallengeManager::instance()->cancelAuthenticationChallenge(this);
}

void NetworkJob::initialize(int playerId,
    const String& pageGroupName,
    const KURL& url,
    const BlackBerry::Platform::NetworkRequest& request,
    PassRefPtr<ResourceHandle> handle,
    BlackBerry::Platform::NetworkStreamFactory* streamFactory,
    Frame* frame,
    int deferLoadingCount,
    int redirectCount)
{
    BLACKBERRY_ASSERT(handle);
    BLACKBERRY_ASSERT(frame);

    m_playerId = playerId;
    m_pageGroupName = pageGroupName;

    m_response.setURL(url);
    m_isFile = url.protocolIs("file") || url.protocolIs("local");
    m_isFTP = url.protocolIs("ftp");

    m_handle = handle;

    m_streamFactory = streamFactory;

    if (frame && frame->loader()->pageDismissalEventBeingDispatched() != FrameLoader::NoDismissal) {
        // In the case the frame will be detached soon, we still need to ping the server, but it is
        // no longer safe to reference the Frame object.
        // See http://trac.webkit.org/changeset/65910 and https://bugs.webkit.org/show_bug.cgi?id=30457.
        // m_frame would be set to zero.
        observeFrame(0);
    } else
        observeFrame(frame);

    m_redirectCount = redirectCount;
    m_deferLoadingCount = deferLoadingCount;

    m_isHeadMethod = m_handle->firstRequest().httpMethod().upper() == "HEAD";

    // We don't need to explicitly call notifyHeaderReceived, as the Content-Type
    // will ultimately get parsed when sendResponseIfNeeded gets called.
    if (!request.getOverrideContentType().empty()) {
        m_contentType = String(request.getOverrideContentType());
        m_isOverrideContentType = true;
    }

    if (!request.getSuggestedSaveName().empty())
        m_contentDisposition = "filename=" + String(request.getSuggestedSaveName());

    BlackBerry::Platform::FilterStream* wrappedStream = m_streamFactory->createNetworkStream(request, m_playerId);
    ASSERT(wrappedStream);

    BlackBerry::Platform::NetworkRequest::TargetType targetType = request.getTargetType();
    if ((targetType == BlackBerry::Platform::NetworkRequest::TargetIsMainFrame
        || targetType == BlackBerry::Platform::NetworkRequest::TargetIsSubframe)
        && !m_isOverrideContentType) {
        RSSFilterStream* filter = new RSSFilterStream();
        filter->setWrappedStream(wrappedStream);
        wrappedStream = filter;
    }

    setWrappedStream(wrappedStream);
}

int NetworkJob::cancelJob()
{
    m_cancelled = true;

    return streamCancel();
}

void NetworkJob::updateDeferLoadingCount(int delta)
{
    m_deferLoadingCount += delta;
    ASSERT(m_deferLoadingCount >= 0);

    if (!isDeferringLoading()) {
        // There might already be a timer set to call this, but it's safe to schedule it again.
        m_deferredData.scheduleProcessDeferredData();
    }
}

void NetworkJob::notifyStatusReceived(int status, const BlackBerry::Platform::String& message)
{
    if (shouldDeferLoading())
        m_deferredData.deferOpen(status, message);
    else
        handleNotifyStatusReceived(status, message);
}

void NetworkJob::handleNotifyStatusReceived(int status, const String& message)
{
    // Check for messages out of order or after cancel.
    if (m_responseSent || m_cancelled)
        return;

    if (isInfo(status))
        return; // ignore

    m_statusReceived = true;

    // Convert non-HTTP status codes to generic HTTP codes.
    m_extendedStatusCode = status;
    if (!status)
        m_response.setHTTPStatusCode(200);
    else if (status < 0)
        m_response.setHTTPStatusCode(404);
    else
        m_response.setHTTPStatusCode(status);

    m_response.setHTTPStatusText(message);

    if (isUnauthorized(m_extendedStatusCode))
        purgeCredentials();
}

void NetworkJob::notifyHeadersReceived(const BlackBerry::Platform::NetworkRequest::HeaderList& headers)
{
    bool cookiesEnabled = m_frame && m_frame->loader() && m_frame->loader()->client()
        && static_cast<FrameLoaderClientBlackBerry*>(m_frame->loader()->client())->cookiesEnabled();

    BlackBerry::Platform::NetworkRequest::HeaderList::const_iterator endIt = headers.end();
    for (BlackBerry::Platform::NetworkRequest::HeaderList::const_iterator it = headers.begin(); it != endIt; ++it) {

        // Handle Set-Cookie headers immediately, even if loading is being deferred, since any request
        // created while loading is deferred should include all cookies received. (This especially
        // affects Set-Cookie headers sent with a 401 response - often this causes an auth dialog to be
        // opened, which defers loading, but the followup request using the credentials from the dialog
        // needs to include the cookie.)
        //
        // This is safe because handleSetCookieHeader only updates the cookiejar, it doesn't call back
        // into the loader.
        String keyString(it->first);
        if (cookiesEnabled && equalIgnoringCase(keyString, "set-cookie"))
            handleSetCookieHeader(it->second);

        if (shouldDeferLoading())
            m_deferredData.deferHeaderReceived(it->first, it->second);
        else {
            String valueString;
            if (equalIgnoringCase(keyString, "Location")) {
                // Location, like all headers, is supposed to be Latin-1. But some sites (wikipedia) send it in UTF-8.
                // All byte strings that are valid UTF-8 are also valid Latin-1 (although outside ASCII, the meaning will
                // differ), but the reverse isn't true. So try UTF-8 first and fall back to Latin-1 if it's invalid.
                // (High Latin-1 should be url-encoded anyway.)
                //
                // FIXME: maybe we should do this with other headers?
                // Skip it for now - we don't want to rewrite random bytes unless we're sure. (Definitely don't want to
                // rewrite cookies, for instance.) Needs more investigation.
                valueString = it->second;
                if (valueString.isNull())
                    valueString = it->second;
            } else
                valueString = it->second;

            handleNotifyHeaderReceived(keyString, valueString);
        }
    }
}

void NetworkJob::notifyMultipartHeaderReceived(const char* key, const char* value)
{
    if (shouldDeferLoading())
        m_deferredData.deferMultipartHeaderReceived(key, value);
    else
        handleNotifyMultipartHeaderReceived(key, value);
}

void NetworkJob::notifyAuthReceived(NetworkRequest::AuthType authType, NetworkRequest::AuthProtocol authProtocol, NetworkRequest::AuthScheme authScheme, const char* realm, AuthResult result)
{
    ProtectionSpaceServerType serverType;
    switch (authType) {
    case NetworkRequest::AuthTypeHost:
        switch (authProtocol) {
        case NetworkRequest::AuthProtocolHTTP:
            serverType = ProtectionSpaceServerHTTP;
            break;
        case NetworkRequest::AuthProtocolHTTPS:
            serverType = ProtectionSpaceServerHTTPS;
            break;
        case NetworkRequest::AuthProtocolFTP:
            serverType = ProtectionSpaceServerFTP;
            break;
        case NetworkRequest::AuthProtocolFTPS:
            serverType = ProtectionSpaceServerFTPS;
            break;
        default:
            ASSERT_NOT_REACHED();
            return;
        }
        break;
    case NetworkRequest::AuthTypeProxy:
        switch (authProtocol) {
        case NetworkRequest::AuthProtocolHTTP:
            serverType = ProtectionSpaceProxyHTTP;
            break;
        case NetworkRequest::AuthProtocolHTTPS:
            serverType = ProtectionSpaceProxyHTTPS;
            break;
        case NetworkRequest::AuthProtocolFTP:
        case NetworkRequest::AuthProtocolFTPS:
            serverType = ProtectionSpaceProxyFTP;
            break;
        default:
            ASSERT_NOT_REACHED();
            return;
        }
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    ProtectionSpaceAuthenticationScheme scheme;
    switch (authScheme) {
    case NetworkRequest::AuthSchemeDefault:
        scheme = ProtectionSpaceAuthenticationSchemeDefault;
        break;
    case NetworkRequest::AuthSchemeHTTPBasic:
        scheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
        break;
    case NetworkRequest::AuthSchemeHTTPDigest:
        scheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
        break;
    case NetworkRequest::AuthSchemeNegotiate:
        scheme = ProtectionSpaceAuthenticationSchemeNegotiate;
        break;
    case NetworkRequest::AuthSchemeNTLM:
        scheme = ProtectionSpaceAuthenticationSchemeNTLM;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    // On success, update stored credentials if necessary
    // On failure, purge credentials and send new request
    // On retry, update stored credentials if necessary and send new request
    if (result == AuthResultFailure)
        purgeCredentials();
    else {
        // Update the credentials that will be stored to match the scheme that was actually used
        AuthenticationChallenge& challenge = authType == NetworkRequest::AuthTypeProxy ? m_handle->getInternal()->m_proxyWebChallenge : m_handle->getInternal()->m_hostWebChallenge;
        if (challenge.hasCredentials()) {
            const ProtectionSpace& oldSpace = challenge.protectionSpace();
            if (oldSpace.authenticationScheme() != scheme && oldSpace.serverType() == serverType) {
                ProtectionSpace newSpace(oldSpace.host(), oldSpace.port(), oldSpace.serverType(), oldSpace.realm(), scheme);
                updateCurrentWebChallenge(AuthenticationChallenge(newSpace, challenge.proposedCredential(), challenge.previousFailureCount(), challenge.failureResponse(), challenge.error()));
            }
        }
        storeCredentials();
    }
    if (result != AuthResultSuccess) {
        switch (sendRequestWithCredentials(serverType, scheme, realm, result != AuthResultRetry)) {
        case SendRequestSucceeded:
            m_newJobWithCredentialsStarted = true;
            break;
        case SendRequestCancelled:
            streamFailedToGetCredentials(authType, authProtocol, authScheme);
            // fall through
        case SendRequestWaiting:
            m_newJobWithCredentialsStarted = false;
            break;
        }
    }
}

void NetworkJob::notifyStringHeaderReceived(const String& key, const String& value)
{
    if (shouldDeferLoading())
        m_deferredData.deferHeaderReceived(key, value);
    else
        handleNotifyHeaderReceived(key, value);
}

void NetworkJob::handleNotifyHeaderReceived(const String& key, const String& value)
{
    // Check for messages out of order or after cancel.
    if (!m_statusReceived || m_responseSent || m_cancelled)
        return;

    String lowerKey = key.lower();
    if (lowerKey == "content-type")
        m_contentType = value.lower();
    else if (lowerKey == "content-disposition")
        m_contentDisposition = value;
    else if (equalIgnoringCase(key, BlackBerry::Platform::NetworkRequest::HEADER_BLACKBERRY_FTP))
        handleFTPHeader(value);

    if (m_response.httpHeaderFields().contains(key.utf8().data()) && isAppendableHeader(lowerKey)) {
        // If there are several headers with same key, we should combine the following ones with the first.
        m_response.setHTTPHeaderField(key, m_response.httpHeaderField(key) + ", " + value);
    } else
        m_response.setHTTPHeaderField(key, value);
}

void NetworkJob::handleNotifyMultipartHeaderReceived(const String& key, const String& value)
{
    if (!m_multipartResponse) {
        // Create a new response based on the original set of headers + the
        // replacement headers. We only replace the same few headers that gecko
        // does. See netwerk/streamconv/converters/nsMultiMixedConv.cpp.
        m_multipartResponse = adoptPtr(new ResourceResponse);
        m_multipartResponse->setURL(m_response.url());

        // The list of BlackBerry::Platform::replaceHeaders that we do not copy from the original
        // response when generating a response.
        const WebCore::HTTPHeaderMap& map = m_response.httpHeaderFields();

        for (WebCore::HTTPHeaderMap::const_iterator it = map.begin(); it != map.end(); ++it) {
            bool needsCopyfromOriginalResponse = true;
            int replaceHeadersIndex = 0;
            while (BlackBerry::Platform::MultipartStream::replaceHeaders[replaceHeadersIndex]) {
                if (it->key.lower() == BlackBerry::Platform::MultipartStream::replaceHeaders[replaceHeadersIndex]) {
                    needsCopyfromOriginalResponse = false;
                    break;
                }
                replaceHeadersIndex++;
            }
            if (needsCopyfromOriginalResponse)
                m_multipartResponse->setHTTPHeaderField(it->key, it->value);
        }

        m_multipartResponse->setIsMultipartPayload(true);
    }

    if (key.lower() == "content-type") {
        String contentType = value.lower();
        m_multipartResponse->setMimeType(extractMIMETypeFromMediaType(contentType));
        m_multipartResponse->setTextEncodingName(extractCharsetFromMediaType(contentType));
    }
    m_multipartResponse->setHTTPHeaderField(key, value);
}

void NetworkJob::handleSetCookieHeader(const String& value)
{
    KURL url = m_response.url();
    CookieManager& manager = cookieManager();
    if ((manager.cookiePolicy() == CookieStorageAcceptPolicyOnlyFromMainDocumentDomain)
        && (m_handle->firstRequest().firstPartyForCookies() != url)
        && manager.getCookie(url, WithHttpOnlyCookies).isEmpty())
        return;
    manager.setCookies(url, value);
}

void NetworkJob::notifyDataReceivedPlain(const char* buf, size_t len)
{
    if (shouldDeferLoading())
        m_deferredData.deferDataReceived(buf, len);
    else
        handleNotifyDataReceived(buf, len);
}

void NetworkJob::handleNotifyDataReceived(const char* buf, size_t len)
{
    // Check for messages out of order or after cancel.
    if ((!m_isFile && !m_statusReceived) || m_cancelled)
        return;

    if (!buf || !len)
        return;

    // The loadFile API sets the override content type,
    // this will always be used as the content type and should not be overridden.
    if (!m_dataReceived && !m_isOverrideContentType) {
        bool shouldSniff = true;

        // Don't bother sniffing the content type of a file that
        // is on a file system if it has a MIME mappable file extension.
        // The file extension is likely to be correct.
        if (m_isFile) {
            String urlFilename = m_response.url().lastPathComponent();
            size_t pos = urlFilename.reverseFind('.');
            if (pos != notFound) {
                String extension = urlFilename.substring(pos + 1);
                String mimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);
                if (!mimeType.isEmpty())
                    shouldSniff = false;
            }
        }

        if (shouldSniff) {
            MIMESniffer sniffer = MIMESniffer(m_contentType.latin1().data(), MIMETypeRegistry::isSupportedImageResourceMIMEType(m_contentType));
            if (const char* type = sniffer.sniff(buf, std::min(len, sniffer.dataSize())))
                m_sniffedMimeType = String(type);
        }
    }

    m_dataReceived = true;

    // Protect against reentrancy.
    updateDeferLoadingCount(1);

    if (shouldSendClientData()) {
        sendResponseIfNeeded();
        sendMultipartResponseIfNeeded();
        if (isClientAvailable()) {
            RecursionGuard guard(m_callingClient);
            m_handle->client()->didReceiveData(m_handle.get(), buf, len, len);
        }
    }

    updateDeferLoadingCount(-1);
}

void NetworkJob::notifyDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    if (shouldDeferLoading())
        m_deferredData.deferDataSent(bytesSent, totalBytesToBeSent);
    else
        handleNotifyDataSent(bytesSent, totalBytesToBeSent);
}

void NetworkJob::handleNotifyDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    if (m_cancelled)
        return;

    // Protect against reentrancy.
    updateDeferLoadingCount(1);

    if (isClientAvailable()) {
        RecursionGuard guard(m_callingClient);
        m_handle->client()->didSendData(m_handle.get(), bytesSent, totalBytesToBeSent);
    }

    updateDeferLoadingCount(-1);
}

void NetworkJob::notifyClose(int status)
{
    if (shouldDeferLoading())
        m_deferredData.deferClose(status);
    else
        handleNotifyClose(status);
}

void NetworkJob::handleNotifyClose(int status)
{
#ifndef NDEBUG
    m_isRunning = false;
#endif

    if (!m_cancelled) {
        if (!m_statusReceived) {
            // Connection failed before sending notifyStatusReceived: use generic NetworkError.
            notifyStatusReceived(BlackBerry::Platform::FilterStream::StatusNetworkError, BlackBerry::Platform::String::emptyString());
        }

        if (shouldReleaseClientResource()) {
            if (isRedirect(m_extendedStatusCode) && (m_redirectCount >= s_redirectMaximum))
                m_extendedStatusCode = BlackBerry::Platform::FilterStream::StatusTooManyRedirects;

            sendResponseIfNeeded();

            if (isClientAvailable()) {
                if (isError(status))
                    m_extendedStatusCode = status;
                RecursionGuard guard(m_callingClient);
                if (shouldNotifyClientFailed()) {
                    String domain = m_extendedStatusCode < 0 ? ResourceError::platformErrorDomain : ResourceError::httpErrorDomain;
                    ResourceError error(domain, m_extendedStatusCode, m_response.url().string(), m_response.httpStatusText());
                    m_handle->client()->didFail(m_handle.get(), error);
                } else
                    m_handle->client()->didFinishLoading(m_handle.get(), 0);
            }
        }
    }

    // Whoever called notifyClose still have a reference to the job, so
    // schedule the deletion with a timer.
    m_deleteJobTimer.startOneShot(0);

    // Detach from the ResourceHandle in any case.
    m_handle = 0;
    m_multipartResponse = nullptr;
}

bool NetworkJob::shouldReleaseClientResource()
{
    if ((m_needsRetryAsFTPDirectory && retryAsFTPDirectory()) || (isRedirect(m_extendedStatusCode) && handleRedirect()) || m_newJobWithCredentialsStarted || m_isAuthenticationChallenging)
        return false;
    return true;
}

bool NetworkJob::shouldNotifyClientFailed() const
{
    ResourceRequest request = m_handle->firstRequest();
    if (request.forceDownload())
        return false;
    if (m_extendedStatusCode < 0)
        return true;
    if (isError(m_extendedStatusCode) && !m_dataReceived && !m_isHeadMethod && request.targetType() != ResourceRequest::TargetIsXHR)
        return true;

    return false;
}

bool NetworkJob::retryAsFTPDirectory()
{
    m_needsRetryAsFTPDirectory = false;

    ASSERT(m_handle);
    ResourceRequest newRequest = m_handle->firstRequest();
    KURL url = newRequest.url();
    url.setPath(url.path() + "/");
    newRequest.setURL(url);
    newRequest.setMustHandleInternally(true);

    // Update the UI.
    handleNotifyHeaderReceived("Location", url.string());

    return startNewJobWithRequest(newRequest);
}

bool NetworkJob::startNewJobWithRequest(ResourceRequest& newRequest, bool increaseRedirectCount, bool rereadCookies)
{
    // m_frame can be null if this is a PingLoader job (See NetworkJob::initialize).
    // In this case we don't start new request.
    if (!m_frame)
        return false;

    if (isClientAvailable()) {
        RecursionGuard guard(m_callingClient);
        m_handle->client()->willSendRequest(m_handle.get(), newRequest, m_response);

        // m_cancelled can become true if the url fails the policy check.
        // newRequest can be cleared when the redirect is rejected.
        if (m_cancelled || newRequest.isEmpty())
            return false;
    }

    // Pass the ownership of the ResourceHandle to the new NetworkJob.
    RefPtr<ResourceHandle> handle = m_handle;
    cancelJob();

    int status = NetworkManager::instance()->startJob(m_playerId,
        m_pageGroupName,
        handle,
        newRequest,
        m_streamFactory,
        m_frame,
        m_deferLoadingCount,
        increaseRedirectCount ? m_redirectCount + 1 : m_redirectCount,
        rereadCookies);
    return status == BlackBerry::Platform::FilterStream::StatusSuccess;
}

bool NetworkJob::handleRedirect()
{
    ASSERT(m_handle);
    if (!m_handle || m_redirectCount >= s_redirectMaximum)
        return false;

    String location = m_response.httpHeaderField("Location");
    if (location.isNull())
        return false;

    KURL newURL(m_response.url(), location);
    if (!newURL.isValid())
        return false;

    if (newURL.protocolIsData()) {
        m_extendedStatusCode = BlackBerry::Platform::FilterStream::StatusInvalidRedirectToData;
        return false;
    }

    ResourceRequest newRequest = m_handle->firstRequest();
    newRequest.setURL(newURL);
    newRequest.setMustHandleInternally(true);

    String method = newRequest.httpMethod().upper();
    if (method != "GET" && method != "HEAD") {
        newRequest.setHTTPMethod("GET");
        newRequest.setHTTPBody(0);
        newRequest.clearHTTPContentLength();
        newRequest.clearHTTPContentType();
    }

    // If this request is challenged, store the credentials now (if they are null this will do nothing)
    storeCredentials();

    // Do not send existing credentials with the new request.
    m_handle->getInternal()->m_currentWebChallenge.nullify();
    m_handle->getInternal()->m_proxyWebChallenge.nullify();
    m_handle->getInternal()->m_hostWebChallenge.nullify();

    return startNewJobWithRequest(newRequest, /* increaseRedirectCount */ true, /* rereadCookies */ true);
}

void NetworkJob::sendResponseIfNeeded()
{
    if (m_responseSent)
        return;

    m_responseSent = true;

    if (shouldNotifyClientFailed())
        return;

    String urlFilename;
    if (!m_response.url().protocolIsData())
        urlFilename = m_response.url().lastPathComponent();

    // Get the MIME type that was set by the content sniffer
    // if there's no custom sniffer header, try to set it from the Content-Type header
    // if this fails, guess it from extension.
    String mimeType = m_sniffedMimeType;
    if (m_isFTP && m_isFTPDir)
        mimeType = "application/x-ftp-directory";
    else if (mimeType.isNull())
        mimeType = extractMIMETypeFromMediaType(m_contentType);
    if (mimeType.isNull())
        mimeType = MIMETypeRegistry::getMIMETypeForPath(urlFilename);
    if (!m_dataReceived && mimeType == "application/octet-stream") {
        // For empty content, if can't guess its mimetype from filename, we manually
        // set the mimetype to "text/plain" in case it goes to download.
        mimeType = "text/plain";
    }
    m_response.setMimeType(mimeType);

    // Set encoding from Content-Type header.
    m_response.setTextEncodingName(extractCharsetFromMediaType(m_contentType));

    // Set content length from header.
    String contentLength = m_response.httpHeaderField("Content-Length");
    if (!contentLength.isNull())
        m_response.setExpectedContentLength(contentLength.toInt64());

    String suggestedFilename = filenameFromHTTPContentDisposition(m_contentDisposition);
    if (suggestedFilename.isEmpty()) {
        // Check and see if an extension already exists.
        String mimeExtension = MIMETypeRegistry::getPreferredExtensionForMIMEType(mimeType);
        if (urlFilename.isEmpty()) {
            if (mimeExtension.isEmpty()) // No extension found for the mimeType.
                suggestedFilename = String(BlackBerry::Platform::LocalizeResource::getString(BlackBerry::Platform::FILENAME_UNTITLED));
            else
                suggestedFilename = String(BlackBerry::Platform::LocalizeResource::getString(BlackBerry::Platform::FILENAME_UNTITLED)) + "." + mimeExtension;
        } else {
            if (urlFilename.reverseFind('.') == notFound && !mimeExtension.isEmpty())
                suggestedFilename = urlFilename + '.' + mimeExtension;
            else
                suggestedFilename = urlFilename;
        }
    }
    m_response.setSuggestedFilename(suggestedFilename);

    if (isClientAvailable()) {
        RecursionGuard guard(m_callingClient);
        m_handle->client()->didReceiveResponse(m_handle.get(), m_response);
    }
}

void NetworkJob::sendMultipartResponseIfNeeded()
{
    if (m_multipartResponse && isClientAvailable()) {
        m_handle->client()->didReceiveResponse(m_handle.get(), *m_multipartResponse);
        m_multipartResponse = nullptr;
    }
}

bool NetworkJob::handleFTPHeader(const String& header)
{
    size_t spacePos = header.find(' ');
    if (spacePos == notFound)
        return false;
    String statusCode = header.left(spacePos);
    switch (statusCode.toInt()) {
    case 213:
        m_isFTPDir = false;
        break;
    case 530:
        purgeCredentials();
        if (m_response.url().protocolIs("ftps"))
            sendRequestWithCredentials(ProtectionSpaceServerFTPS, ProtectionSpaceAuthenticationSchemeDefault, "ftp");
        else
            sendRequestWithCredentials(ProtectionSpaceServerFTP, ProtectionSpaceAuthenticationSchemeDefault, "ftp");
        break;
    case 230:
        storeCredentials();
        break;
    case 550:
        // The user might have entered an URL which point to a directory but forgot type '/',
        // e.g., ftp://ftp.trolltech.com/qt/source where 'source' is a directory. We need to
        // added '/' and try again.
        if (m_handle && !m_handle->firstRequest().url().path().endsWith("/"))
            m_needsRetryAsFTPDirectory = true;
        break;
    }

    return true;
}

NetworkJob::SendRequestResult NetworkJob::sendRequestWithCredentials(ProtectionSpaceServerType type, ProtectionSpaceAuthenticationScheme scheme, const String& realm, bool requireCredentials)
{
    ASSERT(m_handle);
    if (!m_handle)
        return SendRequestCancelled;

    KURL newURL = m_response.url();
    if (!newURL.isValid())
        return SendRequestCancelled;

    // IMPORTANT: if a new source of credentials is added to this method, be sure to handle it in
    // purgeCredentials as well!

    String host;
    int port;
    BlackBerry::Platform::ProxyInfo proxyInfo;
    if (type == ProtectionSpaceProxyHTTP || type == ProtectionSpaceProxyHTTPS) {
        proxyInfo = BlackBerry::Platform::Settings::instance()->proxyInfo(newURL.string());
        ASSERT(!proxyInfo.address.empty());
        if (proxyInfo.address.empty()) {
            // Fall back to the response url if there's no proxy
            // FIXME: is this the best way to handle this?
            host = m_response.url().host();
            port = m_response.url().port();
        } else {
            // proxyInfo returns host:port, without a protocol. KURL can't parse this, so stick http
            // on the front.
            // (We could split into host and port by hand, but that gets hard to parse with IPv6 urls,
            // so better to reuse KURL's parsing.)
            StringBuilder proxyAddress;
            if (type == ProtectionSpaceProxyHTTP)
                proxyAddress.append("http://");
            else
                proxyAddress.append("https://");
            proxyAddress.append(proxyInfo.address);

            KURL proxyURL(KURL(), proxyAddress.toString());
            host = proxyURL.host();
            port = proxyURL.port();
        }
    } else {
        host = m_response.url().host();
        port = m_response.url().port();
    }

    ProtectionSpace protectionSpace(host, port, type, realm, scheme);

    // We've got the scheme and realm. Now we need a username and password.
    Credential credential;

    if (!requireCredentials) {
        // Don't overwrite any existing credentials with the empty credential
        updateCurrentWebChallenge(AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError()), /* allowOverwrite */ false);
    } else if (!(credential = CredentialStorage::get(protectionSpace)).isEmpty()
#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
            || !(credential = CredentialStorage::getFromPersistentStorage(protectionSpace)).isEmpty()
#endif
            ) {
        // First search the CredentialStorage and Persistent Credential Storage
        AuthenticationChallenge challenge(protectionSpace, credential, 0, m_response, ResourceError());
        challenge.setStored(true);
        updateCurrentWebChallenge(challenge);
    } else {
        ASSERT(credential.isEmpty());
        if (m_handle->firstRequest().targetType() == ResourceRequest::TargetIsFavicon) {
            // The favicon loading is triggerred after the main resource has been loaded
            // and parsed, so if we cancel the authentication challenge when loading the main
            // resource, we should also cancel loading the favicon when it starts to
            // load. If not we will receive another challenge which may confuse the user.
            return SendRequestCancelled;
        }

        // CredentialStore is empty. Ask the user via dialog.
        String username;
        String password;

        if (!proxyInfo.address.empty()) {
            username = proxyInfo.username;
            password = proxyInfo.password;
        } else {
            username = m_handle->getInternal()->m_user;
            password = m_handle->getInternal()->m_pass;
        }

        // Before asking the user for credentials, we check if the URL contains that.
        if (username.isEmpty() && password.isEmpty()) {
            if (m_handle->firstRequest().targetType() != ResourceRequest::TargetIsMainFrame && BlackBerry::Platform::Settings::instance()->isChromeProcess())
                return SendRequestCancelled;

            if (!m_frame || !m_frame->page())
                return SendRequestCancelled;

            // DO overwrite any existing credentials with the empty credential
            updateCurrentWebChallenge(AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError()));

            m_isAuthenticationChallenging = true;
            updateDeferLoadingCount(1);

            AuthenticationChallengeManager::instance()->authenticationChallenge(newURL, protectionSpace,
                Credential(), this, m_frame->page()->chrome().client()->platformPageClient());
            return SendRequestWaiting;
        }

        credential = Credential(username, password, CredentialPersistenceForSession);

        updateCurrentWebChallenge(AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError()));
    }

    notifyChallengeResult(newURL, protectionSpace, AuthenticationChallengeSuccess, credential);
    return m_newJobWithCredentialsStarted ? SendRequestSucceeded : SendRequestCancelled;
}

void NetworkJob::storeCredentials()
{
    if (!m_handle)
        return;

    storeCredentials(m_handle->getInternal()->m_hostWebChallenge);
    storeCredentials(m_handle->getInternal()->m_proxyWebChallenge);
}

void NetworkJob::storeCredentials(AuthenticationChallenge& challenge)
{
    if (challenge.isNull())
        return;

    if (challenge.isStored())
        return;

    // Obviously we can't have successfully authenticated with empty credentials. (To store empty
    // credentials, use purgeCredentials.)

    // FIXME: We should assert here, but there is one path (when the credentials are read from the
    // proxy config entirely in the platform layer) where storeCredentials is called with an empty
    // challenge. The credentials should be passed back from the platform layer for storage in this
    // case - see PR 287791.
    if (challenge.proposedCredential().user().isEmpty() || challenge.proposedCredential().password().isEmpty())
        return;

    CredentialStorage::set(challenge.proposedCredential(), challenge.protectionSpace(), m_response.url());
    challenge.setStored(true);

    if (challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTP || challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTPS) {
        StringBuilder proxyAddress;
        proxyAddress.append(challenge.protectionSpace().host());
        proxyAddress.append(":");
        proxyAddress.appendNumber(challenge.protectionSpace().port());

        BlackBerry::Platform::ProxyInfo proxyInfo;
        proxyInfo.address = proxyAddress.toString();
        proxyInfo.username = challenge.proposedCredential().user();
        proxyInfo.password = challenge.proposedCredential().password();

        BlackBerry::Platform::Settings::instance()->storeProxyCredentials(proxyInfo);
        if (m_frame && m_frame->page())
            m_frame->page()->chrome().client()->platformPageClient()->syncProxyCredential(challenge.proposedCredential());
    }
}

void NetworkJob::purgeCredentials()
{
    if (!m_handle)
        return;

    purgeCredentials(m_handle->getInternal()->m_hostWebChallenge);
    purgeCredentials(m_handle->getInternal()->m_proxyWebChallenge);

    m_handle->getInternal()->m_currentWebChallenge.nullify();
    m_handle->getInternal()->m_proxyWebChallenge.nullify();
    m_handle->getInternal()->m_hostWebChallenge.nullify();
}

void NetworkJob::purgeCredentials(AuthenticationChallenge& challenge)
{
    if (challenge.isNull())
        return;

    const String& purgeUsername = challenge.proposedCredential().user();
    const String& purgePassword = challenge.proposedCredential().password();

    // Since this credential didn't work, remove it from all sources which would return it
    // IMPORTANT: every source that is checked for a password in sendRequestWithCredentials should
    // be handled here!

    if (challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTP || challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTPS) {
        BlackBerry::Platform::ProxyInfo proxyInfo = BlackBerry::Platform::Settings::instance()->proxyInfo(m_handle->firstRequest().url().string());
        if (!proxyInfo.address.empty() && purgeUsername == proxyInfo.username.c_str() && purgePassword == proxyInfo.password.c_str()) {
            proxyInfo.username.clear();
            proxyInfo.password.clear();
            BlackBerry::Platform::Settings::instance()->storeProxyCredentials(proxyInfo);
        }
    } else if (m_handle->getInternal()->m_user == purgeUsername && m_handle->getInternal()->m_pass == purgePassword) {
        m_handle->getInternal()->m_user = "";
        m_handle->getInternal()->m_pass = "";
    }

    // Do not compare credential objects with == here, since we don't care about the persistence.

    const Credential& storedCredential = CredentialStorage::get(challenge.protectionSpace());
    if (storedCredential.user() == purgeUsername && storedCredential.password() == purgePassword) {
        CredentialStorage::remove(challenge.protectionSpace());
        challenge.setStored(false);
    }
#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
    const Credential& persistedCredential = credentialBackingStore().getLogin(challenge.protectionSpace());
    if (persistedCredential.user() == purgeUsername && persistedCredential.password() == purgePassword)
        credentialBackingStore().removeLogin(challenge.protectionSpace(), purgeUsername);
#endif
}

bool NetworkJob::shouldSendClientData() const
{
    return (!isRedirect(m_extendedStatusCode) || !m_response.httpHeaderFields().contains("Location"))
        && !m_needsRetryAsFTPDirectory;
}

void NetworkJob::fireDeleteJobTimer(Timer<NetworkJob>*)
{
    NetworkManager::instance()->deleteJob(this);
}

void NetworkJob::notifyChallengeResult(const KURL& url, const ProtectionSpace& protectionSpace, AuthenticationChallengeResult result, const Credential& credential)
{
    ASSERT(url.isValid());
    ASSERT(url == m_response.url());
    ASSERT(!protectionSpace.host().isEmpty());

    if (m_isAuthenticationChallenging) {
        m_isAuthenticationChallenging = false;
        if (result == AuthenticationChallengeSuccess)
            cancelJob();
        updateDeferLoadingCount(-1);
    }

    if (result != AuthenticationChallengeSuccess) {
        NetworkRequest::AuthType authType;
        NetworkRequest::AuthProtocol authProtocol;
        NetworkRequest::AuthScheme authScheme;
        protectionSpaceToPlatformAuth(protectionSpace, authType, authProtocol, authScheme);
        streamFailedToGetCredentials(authType, authProtocol, authScheme);
        return;
    }

    updateCurrentWebChallenge(AuthenticationChallenge(protectionSpace, credential, 0, m_response, ResourceError()), /* allowOverwrite */ false);

    ResourceRequest newRequest = m_handle->firstRequest();
    newRequest.setURL(url);
    newRequest.setMustHandleInternally(true);
    m_newJobWithCredentialsStarted = startNewJobWithRequest(newRequest, /* increaseRedirectCount */ false, /* rereadCookies */ true);
}

void NetworkJob::frameDestroyed()
{
    if (m_frame && !m_cancelled)
        cancelJob();
    FrameDestructionObserver::frameDestroyed();
}

void NetworkJob::willDetachPage()
{
    if (m_frame && !m_cancelled)
        cancelJob();
}

void NetworkJob::updateCurrentWebChallenge(const AuthenticationChallenge& challenge, bool allowOverwrite)
{
    if (allowOverwrite || !m_handle->getInternal()->m_currentWebChallenge.hasCredentials())
        m_handle->getInternal()->m_currentWebChallenge = challenge;
    if (challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTP || challenge.protectionSpace().serverType() == ProtectionSpaceProxyHTTPS) {
        if (allowOverwrite || !m_handle->getInternal()->m_proxyWebChallenge.hasCredentials())
            m_handle->getInternal()->m_proxyWebChallenge = challenge;
    } else {
        if (allowOverwrite || !m_handle->getInternal()->m_hostWebChallenge.hasCredentials())
            m_handle->getInternal()->m_hostWebChallenge = challenge;
    }
}

const BlackBerry::Platform::String NetworkJob::mimeType() const
{
    return m_response.mimeType();
}

} // namespace WebCore

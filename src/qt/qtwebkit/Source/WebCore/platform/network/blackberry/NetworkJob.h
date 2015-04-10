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

#ifndef NetworkJob_h
#define NetworkJob_h

#include "AuthenticationChallengeManager.h"
#include "DeferredData.h"
#include "FrameDestructionObserver.h"
#include "ResourceHandle.h"
#include "ResourceResponse.h"
#include "Timer.h"

#include <network/FilterStream.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace BlackBerry {
namespace Platform {
class NetworkRequest;
class NetworkStreamFactory;
}
}

namespace WebCore {

class Credential;
class Frame;
class KURL;
class ProtectionSpace;
class ResourceRequest;

class NetworkJob : public AuthenticationChallengeClient, public BlackBerry::Platform::FilterStream, public FrameDestructionObserver {
public:
    NetworkJob();
    ~NetworkJob();

    void initialize(int playerId,
        const String& pageGroupName,
        const KURL&,
        const BlackBerry::Platform::NetworkRequest&,
        PassRefPtr<ResourceHandle>,
        BlackBerry::Platform::NetworkStreamFactory*,
        Frame*,
        int deferLoadingCount,
        int redirectCount);
    PassRefPtr<ResourceHandle> handle() const { return m_handle; }
#ifndef NDEBUG
    bool isRunning() const { return m_isRunning; }
#endif
    bool isCancelled() const { return m_cancelled; }
    int cancelJob();
    bool isDeferringLoading() const { return m_deferLoadingCount > 0; }
    void updateDeferLoadingCount(int delta);
    virtual void notifyStatusReceived(int status, const BlackBerry::Platform::String& message);
    void handleNotifyStatusReceived(int status, const String& message);
    virtual void notifyHeadersReceived(const BlackBerry::Platform::NetworkRequest::HeaderList& headers);
    virtual void notifyMultipartHeaderReceived(const char* key, const char* value);
    virtual void notifyAuthReceived(BlackBerry::Platform::NetworkRequest::AuthType,
        BlackBerry::Platform::NetworkRequest::AuthProtocol,
        BlackBerry::Platform::NetworkRequest::AuthScheme,
        const char* realm,
        AuthResult);
    // notifyStringHeaderReceived exists only to resolve ambiguity between char* and String parameters
    void notifyStringHeaderReceived(const String& key, const String& value);
    void handleNotifyHeaderReceived(const String& key, const String& value);
    void handleNotifyMultipartHeaderReceived(const String& key, const String& value);
    void handleSetCookieHeader(const String& value);
    void notifyDataReceivedPlain(const char* buf, size_t len);
    void handleNotifyDataReceived(const char* buf, size_t len);
    virtual void notifyDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent);
    void handleNotifyDataSent(unsigned long long bytesSent, unsigned long long totalBytesToBeSent);
    virtual void notifyClose(int status);
    void handleNotifyClose(int status);
    virtual int status() const { return m_extendedStatusCode; }
    virtual const BlackBerry::Platform::String mimeType() const;

    virtual void notifyChallengeResult(const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&);

    // Implementation of FrameDestructionObserver pure virtuals.
    virtual void frameDestroyed();
    virtual void willDetachPage();

private:
    bool isClientAvailable() const { return !m_cancelled && m_handle && m_handle->client(); }

    virtual void notifyDataReceived(BlackBerry::Platform::NetworkBuffer* buffer)
    {
        notifyDataReceivedPlain(BlackBerry::Platform::networkBufferData(buffer), BlackBerry::Platform::networkBufferDataLength(buffer));
    }

    virtual void setWasDiskCached(bool value)
    {
       m_response.setWasCached(value);
    }

    bool shouldSendClientData() const;

    bool shouldReleaseClientResource();
    bool shouldNotifyClientFailed() const;

    bool shouldDeferLoading() const
    {
        return isDeferringLoading() || m_deferredData.hasDeferredData();
    }

    bool retryAsFTPDirectory();

    bool startNewJobWithRequest(ResourceRequest& newRequest, bool increasRedirectCount = false, bool rereadCookies = false);

    bool handleRedirect();

    // This can cause m_cancelled to be set to true, if it passes up an error to m_handle->client() which causes the job to be cancelled
    void sendResponseIfNeeded();
    void sendMultipartResponseIfNeeded();

    void fireDeleteJobTimer(Timer<NetworkJob>*);

    bool handleFTPHeader(const String& header);

    // The server needs authentication credentials. Search in the CredentialStorage
    // or prompt the user via dialog, then resend the request with the credentials.
    enum SendRequestResult {
        SendRequestSucceeded,
        SendRequestCancelled,
        SendRequestWaiting
    };
    SendRequestResult sendRequestWithCredentials(ProtectionSpaceServerType, ProtectionSpaceAuthenticationScheme, const String& realm, bool requireCredentials = true);

    void storeCredentials();
    void storeCredentials(AuthenticationChallenge&);
    void purgeCredentials();
    void purgeCredentials(AuthenticationChallenge&);

    bool isError(int statusCode) const
    {
        return statusCode < 0 || (400 <= statusCode && statusCode < 600);
    }

    void updateCurrentWebChallenge(const AuthenticationChallenge&, bool allowOverwrite = true);

private:
    int m_playerId;
    String m_pageGroupName;
    RefPtr<ResourceHandle> m_handle;
    ResourceResponse m_response;
    OwnPtr<ResourceResponse> m_multipartResponse;
    Timer<NetworkJob> m_deleteJobTimer;
    String m_contentType;
    String m_sniffedMimeType;
    String m_contentDisposition;
    BlackBerry::Platform::NetworkStreamFactory* m_streamFactory;
    bool m_isFile;
    bool m_isFTP;
    bool m_isFTPDir;
#ifndef NDEBUG
    bool m_isRunning;
#endif
    bool m_cancelled;
    bool m_statusReceived;
    bool m_dataReceived;
    bool m_responseSent;
    bool m_callingClient;
    bool m_needsRetryAsFTPDirectory;
    bool m_isOverrideContentType;
    bool m_newJobWithCredentialsStarted;
    bool m_isHeadMethod;

    // If an HTTP status code is received, m_extendedStatusCode and m_response.httpStatusCode will both be set to it.
    // If a platform error code is received, m_extendedStatusCode will be set to it and m_response.httpStatusCode will be set to 404.
    int m_extendedStatusCode;

    int m_redirectCount;

    DeferredData m_deferredData;
    int m_deferLoadingCount;

    bool m_isAuthenticationChallenging;
};

} // namespace WebCore

#endif // NetworkJob_h

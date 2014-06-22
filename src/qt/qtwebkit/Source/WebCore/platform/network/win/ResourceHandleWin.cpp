/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ResourceHandle.h"

#include "DataURL.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "SharedBuffer.h"
#include "Timer.h"

#include <wtf/MainThread.h>
#include <wtf/text/CString.h>

#include <windows.h>
#include <wininet.h>

namespace WebCore {

static inline HINTERNET createInternetHandle(const String& userAgent, bool asynchronous)
{
    String userAgentString = userAgent;
    HINTERNET internetHandle = InternetOpenW(userAgentString.charactersWithNullTermination().data(), INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, asynchronous ? INTERNET_FLAG_ASYNC : 0);

    if (asynchronous)
        InternetSetStatusCallback(internetHandle, &ResourceHandle::internetStatusCallback);

    return internetHandle;
}

static HINTERNET asynchronousInternetHandle(const String& userAgent)
{
    static HINTERNET internetHandle = createInternetHandle(userAgent, true);
    return internetHandle;
}

static String queryHTTPHeader(HINTERNET requestHandle, DWORD infoLevel)
{
    DWORD bufferSize = 0;
    HttpQueryInfoW(requestHandle, infoLevel, 0, &bufferSize, 0);

    Vector<UChar> characters(bufferSize / sizeof(UChar));

    if (!HttpQueryInfoW(requestHandle, infoLevel, characters.data(), &bufferSize, 0))
        return String();

    characters.removeLast(); // Remove NullTermination.
    return String::adopt(characters);
}


class WebCoreSynchronousLoader : public ResourceHandleClient {
    WTF_MAKE_NONCOPYABLE(WebCoreSynchronousLoader);
public:
    WebCoreSynchronousLoader(ResourceError&, ResourceResponse&, Vector<char>&, const String& userAgent);
    ~WebCoreSynchronousLoader();

    HINTERNET internetHandle() const { return m_internetHandle; }

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&);
    virtual void didReceiveData(ResourceHandle*, const char*, int, int encodedDataLength);
    virtual void didFinishLoading(ResourceHandle*, double /*finishTime*/);
    virtual void didFail(ResourceHandle*, const ResourceError&);

private:
    ResourceError& m_error;
    ResourceResponse& m_response;
    Vector<char>& m_data;
    HINTERNET m_internetHandle;
};

WebCoreSynchronousLoader::WebCoreSynchronousLoader(ResourceError& error, ResourceResponse& response, Vector<char>& data, const String& userAgent)
    : m_error(error)
    , m_response(response)
    , m_data(data)
    , m_internetHandle(createInternetHandle(userAgent, false))
{
}

WebCoreSynchronousLoader::~WebCoreSynchronousLoader()
{
    InternetCloseHandle(m_internetHandle);
}

void WebCoreSynchronousLoader::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    m_response = response;
}

void WebCoreSynchronousLoader::didReceiveData(ResourceHandle*, const char* data, int length, int)
{
    m_data.append(data, length);
}

void WebCoreSynchronousLoader::didFinishLoading(ResourceHandle*, double)
{
}

void WebCoreSynchronousLoader::didFail(ResourceHandle*, const ResourceError& error)
{
    m_error = error;
}


ResourceHandleInternal::~ResourceHandleInternal()
{
}

ResourceHandle::~ResourceHandle()
{
}

static void callOnRedirect(void* context)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(context);
    handle->onRedirect();
}

static void callOnRequestComplete(void* context)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(context);
    handle->onRequestComplete();
}

void ResourceHandle::internetStatusCallback(HINTERNET internetHandle, DWORD_PTR context, DWORD internetStatus,
                                                     LPVOID statusInformation, DWORD statusInformationLength)
{
    ResourceHandle* handle = reinterpret_cast<ResourceHandle*>(context);

    switch (internetStatus) {
    case INTERNET_STATUS_REDIRECT:
        handle->d->m_redirectUrl = String(static_cast<UChar*>(statusInformation), statusInformationLength);
        callOnMainThread(callOnRedirect, handle);
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        callOnMainThread(callOnRequestComplete, handle);
        break;

    default:
        break;
    }
}

void ResourceHandle::onRedirect()
{
    ResourceRequest newRequest = firstRequest();
    newRequest.setURL(KURL(ParsedURLString, d->m_redirectUrl));

    ResourceResponse response(firstRequest().url(), String(), 0, String(), String());

    if (ResourceHandleClient* resourceHandleClient = client())
        resourceHandleClient->willSendRequest(this, newRequest, response);
}

bool ResourceHandle::onRequestComplete()
{
    if (!d->m_internetHandle) { // 0 if canceled.
        deref(); // balances ref in start
        return false;
    }

    if (d->m_bytesRemainingToWrite) {
        DWORD bytesWritten;
        InternetWriteFile(d->m_requestHandle,
                          d->m_formData.data() + (d->m_formData.size() - d->m_bytesRemainingToWrite),
                          d->m_bytesRemainingToWrite,
                          &bytesWritten);
        d->m_bytesRemainingToWrite -= bytesWritten;
        if (d->m_bytesRemainingToWrite)
            return true;
        d->m_formData.clear();
    }

    if (!d->m_sentEndRequest) {
        HttpEndRequestW(d->m_requestHandle, 0, 0, reinterpret_cast<DWORD_PTR>(this));
        d->m_sentEndRequest = true;
        return true;
    }

    static const int bufferSize = 32768;
    char buffer[bufferSize];
    INTERNET_BUFFERSA buffers;
    buffers.dwStructSize = sizeof(INTERNET_BUFFERSA);
    buffers.lpvBuffer = buffer;
    buffers.dwBufferLength = bufferSize;

    BOOL ok = FALSE;
    while ((ok = InternetReadFileExA(d->m_requestHandle, &buffers, d->m_loadSynchronously ? 0 : IRF_NO_WAIT, reinterpret_cast<DWORD_PTR>(this))) && buffers.dwBufferLength) {
        if (!d->m_hasReceivedResponse) {
            d->m_hasReceivedResponse = true;

            ResourceResponse response;
            response.setURL(firstRequest().url());

            String httpStatusText = queryHTTPHeader(d->m_requestHandle, HTTP_QUERY_STATUS_TEXT);
            if (!httpStatusText.isNull())
                response.setHTTPStatusText(httpStatusText);

            String httpStatusCode = queryHTTPHeader(d->m_requestHandle, HTTP_QUERY_STATUS_CODE);
            if (!httpStatusCode.isNull())
                response.setHTTPStatusCode(httpStatusCode.toInt());

            String httpContentLength = queryHTTPHeader(d->m_requestHandle, HTTP_QUERY_CONTENT_LENGTH);
            if (!httpContentLength.isNull())
                response.setExpectedContentLength(httpContentLength.toInt());

            String httpContentType = queryHTTPHeader(d->m_requestHandle, HTTP_QUERY_CONTENT_TYPE);
            if (!httpContentType.isNull()) {
                response.setMimeType(extractMIMETypeFromMediaType(httpContentType));
                response.setTextEncodingName(extractCharsetFromMediaType(httpContentType));
            }

            if (ResourceHandleClient* resourceHandleClient = client())
                resourceHandleClient->didReceiveResponse(this, response);
        }

        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=19793
        // -1 means we do not provide any data about transfer size to inspector so it would use
        // Content-Length headers or content size to show transfer size.
        if (ResourceHandleClient* resourceHandleClient = client())
            resourceHandleClient->didReceiveData(this, buffer, buffers.dwBufferLength, -1);
        buffers.dwBufferLength = bufferSize;
    }

    if (!ok && GetLastError() == ERROR_IO_PENDING)
        return true;

    if (ResourceHandleClient* resourceHandleClient = client())
        resourceHandleClient->didFinishLoading(this, 0);

    InternetCloseHandle(d->m_requestHandle);
    InternetCloseHandle(d->m_connectHandle);
    deref(); // balances ref in start
    return false;
}

bool ResourceHandle::start()
{
    if (firstRequest().url().isLocalFile() || firstRequest().url().protocolIsData()) {
        ref(); // balanced by deref in fileLoadTimer
        if (d->m_loadSynchronously)
            fileLoadTimer(0);
        else
            d->m_fileLoadTimer.startOneShot(0.0);
        return true;
    }

    if (!d->m_internetHandle)
        d->m_internetHandle = asynchronousInternetHandle(d->m_context->userAgent());

    if (!d->m_internetHandle)
        return false;

    DWORD flags = INTERNET_FLAG_KEEP_CONNECTION
        | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
        | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP
        | INTERNET_FLAG_DONT_CACHE
        | INTERNET_FLAG_RELOAD;

    d->m_connectHandle = InternetConnectW(d->m_internetHandle, firstRequest().url().host().charactersWithNullTermination().data(), firstRequest().url().port(),
                                          0, 0, INTERNET_SERVICE_HTTP, flags, reinterpret_cast<DWORD_PTR>(this));

    if (!d->m_connectHandle)
        return false;

    String urlStr = firstRequest().url().path();
    String urlQuery = firstRequest().url().query();

    if (!urlQuery.isEmpty()) {
        urlStr.append('?');
        urlStr.append(urlQuery);
    }

    String httpMethod = firstRequest().httpMethod();
    String httpReferrer = firstRequest().httpReferrer();

    LPCWSTR httpAccept[] = { L"*/*", 0 };

    d->m_requestHandle = HttpOpenRequestW(d->m_connectHandle, httpMethod.charactersWithNullTermination().data(), urlStr.charactersWithNullTermination().data(),
                                          0, httpReferrer.charactersWithNullTermination().data(), httpAccept, flags, reinterpret_cast<DWORD_PTR>(this));

    if (!d->m_requestHandle) {
        InternetCloseHandle(d->m_connectHandle);
        return false;
    }

    if (firstRequest().httpBody()) {
        firstRequest().httpBody()->flatten(d->m_formData);
        d->m_bytesRemainingToWrite = d->m_formData.size();
    }

    Vector<UChar> httpHeaders;
    const HTTPHeaderMap& httpHeaderFields = firstRequest().httpHeaderFields();

    for (HTTPHeaderMap::const_iterator it = httpHeaderFields.begin(); it != httpHeaderFields.end(); ++it) {
        if (equalIgnoringCase(it->key, "Accept") || equalIgnoringCase(it->key, "Referer") || equalIgnoringCase(it->key, "User-Agent"))
            continue;

        if (!httpHeaders.isEmpty())
            httpHeaders.append('\n');

        httpHeaders.append(it->key.characters(), it->key.length());
        httpHeaders.append(':');
        httpHeaders.append(it->value.characters(), it->value.length());
    }

    INTERNET_BUFFERSW internetBuffers;
    ZeroMemory(&internetBuffers, sizeof(internetBuffers));
    internetBuffers.dwStructSize = sizeof(internetBuffers);
    internetBuffers.lpcszHeader = httpHeaders.data();
    internetBuffers.dwHeadersLength = httpHeaders.size();
    internetBuffers.dwBufferTotal = d->m_bytesRemainingToWrite;

    HttpSendRequestExW(d->m_requestHandle, &internetBuffers, 0, 0, reinterpret_cast<DWORD_PTR>(this));

    ref(); // balanced by deref in onRequestComplete

    if (d->m_loadSynchronously)
        while (onRequestComplete()) {
            // Loop until finished.
        }

    return true;
}

void ResourceHandle::fileLoadTimer(Timer<ResourceHandle>*)
{
    RefPtr<ResourceHandle> protector(this);
    deref(); // balances ref in start

    if (firstRequest().url().protocolIsData()) {
        handleDataURL(this);
        return;
    }

    String fileName = firstRequest().url().fileSystemPath();
    HANDLE fileHandle = CreateFileW(fileName.charactersWithNullTermination().data(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        client()->didFail(this, ResourceError());
        return;
    }

    ResourceResponse response;

    int dotPos = fileName.reverseFind('.');
    int slashPos = fileName.reverseFind('/');

    if (slashPos < dotPos && dotPos != -1) {
        String ext = fileName.substring(dotPos + 1);
        response.setMimeType(MIMETypeRegistry::getMIMETypeForExtension(ext));
    }

    client()->didReceiveResponse(this, response);

    bool result = false;
    DWORD bytesRead = 0;

    do {
        const int bufferSize = 8192;
        char buffer[bufferSize];
        result = ReadFile(fileHandle, &buffer, bufferSize, &bytesRead, 0);
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=19793
        // -1 means we do not provide any data about transfer size to inspector so it would use
        // Content-Length headers or content size to show transfer size.
        if (result && bytesRead)
            client()->didReceiveData(this, buffer, bytesRead, -1);
        // Check for end of file.
    } while (result && bytesRead);

    CloseHandle(fileHandle);

    client()->didFinishLoading(this, 0);
}

void ResourceHandle::cancel()
{
    if (d->m_requestHandle) {
        d->m_internetHandle = 0;
        InternetCloseHandle(d->m_requestHandle);
        InternetCloseHandle(d->m_connectHandle);
    } else
        d->m_fileLoadTimer.stop();
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials storedCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    UNUSED_PARAM(storedCredentials);

    WebCoreSynchronousLoader syncLoader(error, response, data, request.httpUserAgent());
    ResourceHandle handle(context, request, &syncLoader, true, false);

    handle.setSynchronousInternetHandle(syncLoader.internetHandle());
    handle.start();
}

void ResourceHandle::setSynchronousInternetHandle(HINTERNET internetHandle)
{
    d->m_internetHandle = internetHandle;
    d->m_loadSynchronously = true;
}

void prefetchDNS(const String&)
{
    notImplemented();
}

bool ResourceHandle::loadsBlocked()
{
    return false;
}

void ResourceHandle::platformSetDefersLoading(bool)
{
    notImplemented();
}

} // namespace WebCore

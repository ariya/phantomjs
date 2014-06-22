/*
 *  Copyright (C) 2003, 2006, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2005, 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2011 Google Inc. All rights reserved.
 *  Copyright (C) 2012 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef XMLHttpRequest_h
#define XMLHttpRequest_h

#include "ActiveDOMObject.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "FormData.h"
#include "ResourceResponse.h"
#include "ScriptWrappable.h"
#include "ThreadableLoaderClient.h"
#include "XMLHttpRequestProgressEventThrottle.h"
#include <wtf/OwnPtr.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

class Blob;
class Document;
class DOMFormData;
class ResourceRequest;
class SecurityOrigin;
class SharedBuffer;
class TextResourceDecoder;
class ThreadableLoader;

class XMLHttpRequest : public ScriptWrappable, public RefCounted<XMLHttpRequest>, public EventTarget, private ThreadableLoaderClient, public ActiveDOMObject {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<XMLHttpRequest> create(ScriptExecutionContext*);
    ~XMLHttpRequest();

    // These exact numeric values are important because JS expects them.
    enum State {
        UNSENT = 0,
        OPENED = 1,
        HEADERS_RECEIVED = 2,
        LOADING = 3,
        DONE = 4
    };
    
    enum ResponseTypeCode {
        ResponseTypeDefault,
        ResponseTypeText, 
        ResponseTypeDocument,
        ResponseTypeBlob,
        ResponseTypeArrayBuffer
    };

    virtual void contextDestroyed();
#if ENABLE(XHR_TIMEOUT)
    virtual void didTimeout();
#endif
    virtual bool canSuspend() const;
    virtual void suspend(ReasonForSuspension);
    virtual void resume();
    virtual void stop();

    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    const KURL& url() const { return m_url; }
    String statusText(ExceptionCode&) const;
    int status(ExceptionCode&) const;
    State readyState() const;
    bool withCredentials() const { return m_includeCredentials; }
    void setWithCredentials(bool, ExceptionCode&);
    void open(const String& method, const KURL&, ExceptionCode&);
    void open(const String& method, const KURL&, bool async, ExceptionCode&);
    void open(const String& method, const KURL&, bool async, const String& user, ExceptionCode&);
    void open(const String& method, const KURL&, bool async, const String& user, const String& password, ExceptionCode&);
    void send(ExceptionCode&);
    void send(Document*, ExceptionCode&);
    void send(const String&, ExceptionCode&);
    void send(Blob*, ExceptionCode&);
    void send(DOMFormData*, ExceptionCode&);
    void send(ArrayBuffer*, ExceptionCode&);
    void send(ArrayBufferView*, ExceptionCode&);
    void abort();
    void setRequestHeader(const AtomicString& name, const String& value, ExceptionCode&);
    void overrideMimeType(const String& override);
    String getAllResponseHeaders(ExceptionCode&) const;
    String getResponseHeader(const AtomicString& name, ExceptionCode&) const;
    String responseText(ExceptionCode&);
    Document* responseXML(ExceptionCode&);
    Document* optionalResponseXML() const { return m_responseDocument.get(); }
    Blob* responseBlob(ExceptionCode&);
    Blob* optionalResponseBlob() const { return m_responseBlob.get(); }
#if ENABLE(XHR_TIMEOUT)
    unsigned long timeout() const { return m_timeoutMilliseconds; }
    void setTimeout(unsigned long timeout, ExceptionCode&);
#endif

    void sendFromInspector(PassRefPtr<FormData>, ExceptionCode&);

    // Expose HTTP validation methods for other untrusted requests.
    static bool isAllowedHTTPMethod(const String&);
    static String uppercaseKnownHTTPMethod(const String&);
    static bool isAllowedHTTPHeader(const String&);

    void setResponseType(const String&, ExceptionCode&);
    String responseType();
    ResponseTypeCode responseTypeCode() const { return m_responseTypeCode; }
    
    // response attribute has custom getter.
    ArrayBuffer* responseArrayBuffer(ExceptionCode&);
    ArrayBuffer* optionalResponseArrayBuffer() const { return m_responseArrayBuffer.get(); }

    void setLastSendLineNumber(unsigned lineNumber) { m_lastSendLineNumber = lineNumber; }
    void setLastSendURL(const String& url) { m_lastSendURL = url; }

    XMLHttpRequestUpload* upload();
    XMLHttpRequestUpload* optionalUpload() const { return m_upload.get(); }

    DEFINE_ATTRIBUTE_EVENT_LISTENER(readystatechange);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(loadend);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(loadstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
#if ENABLE(XHR_TIMEOUT)
    DEFINE_ATTRIBUTE_EVENT_LISTENER(timeout);
#endif

    using RefCounted<XMLHttpRequest>::ref;
    using RefCounted<XMLHttpRequest>::deref;

private:
    XMLHttpRequest(ScriptExecutionContext*);

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    Document* document() const;
    SecurityOrigin* securityOrigin() const;

#if ENABLE(DASHBOARD_SUPPORT)
    bool usesDashboardBackwardCompatibilityMode() const;
#endif

    virtual void didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent);
    virtual void didReceiveResponse(unsigned long identifier, const ResourceResponse&);
    virtual void didReceiveData(const char* data, int dataLength);
    virtual void didFinishLoading(unsigned long identifier, double finishTime);
    virtual void didFail(const ResourceError&);
    virtual void didFailRedirectCheck();

    String responseMIMEType() const;
    bool responseIsXML() const;

    bool initSend(ExceptionCode&);
    void sendBytesData(const void*, size_t, ExceptionCode&);

    String getRequestHeader(const AtomicString& name) const;
    void setRequestHeaderInternal(const AtomicString& name, const String& value);

    void changeState(State newState);
    void callReadyStateChangeListener();
    void dropProtection();
    void internalAbort();
    void clearResponse();
    void clearResponseBuffers();
    void clearRequest();

    void createRequest(ExceptionCode&);

    void genericError();
    void networkError();
    void abortError();

    OwnPtr<XMLHttpRequestUpload> m_upload;

    KURL m_url;
    String m_method;
    HTTPHeaderMap m_requestHeaders;
    RefPtr<FormData> m_requestEntityBody;
    String m_mimeTypeOverride;
    bool m_async;
    bool m_includeCredentials;
#if ENABLE(XHR_TIMEOUT)
    unsigned long m_timeoutMilliseconds;
#endif
    RefPtr<Blob> m_responseBlob;

    RefPtr<ThreadableLoader> m_loader;
    State m_state;

    ResourceResponse m_response;
    String m_responseEncoding;

    RefPtr<TextResourceDecoder> m_decoder;

    StringBuilder m_responseBuilder;
    mutable bool m_createdDocument;
    mutable RefPtr<Document> m_responseDocument;
    
    RefPtr<SharedBuffer> m_binaryResponseBuilder;
    mutable RefPtr<ArrayBuffer> m_responseArrayBuffer;

    bool m_error;

    bool m_uploadEventsAllowed;
    bool m_uploadComplete;

    bool m_sameOriginRequest;

    // Used for onprogress tracking
    long long m_receivedLength;

    unsigned m_lastSendLineNumber;
    String m_lastSendURL;
    ExceptionCode m_exceptionCode;

    EventTargetData m_eventTargetData;

    XMLHttpRequestProgressEventThrottle m_progressEventThrottle;

    // An enum corresponding to the allowed string values for the responseType attribute.
    ResponseTypeCode m_responseTypeCode;
};

} // namespace WebCore

#endif // XMLHttpRequest_h

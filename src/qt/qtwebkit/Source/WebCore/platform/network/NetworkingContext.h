/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef NetworkingContext_h
#define NetworkingContext_h

#include "NetworkStorageSession.h"
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(MAC)
#include <wtf/SchedulePair.h>
#endif

#if PLATFORM(QT)
#include <qglobal.h>
#endif

#if PLATFORM(MAC)
OBJC_CLASS NSOperationQueue;
#endif

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QObject;
class QNetworkAccessManager;
class QUrl;
QT_END_NAMESPACE
#endif

#if USE(SOUP)
typedef struct _SoupSession SoupSession;
#endif

namespace WebCore {

class ResourceError;
class ResourceRequest;

class NetworkingContext : public RefCounted<NetworkingContext> {
public:
    virtual ~NetworkingContext() { }

    virtual bool isValid() const { return true; }

    virtual bool shouldClearReferrerOnHTTPSToHTTPRedirect() const = 0;

#if PLATFORM(MAC)
    virtual bool needsSiteSpecificQuirks() const = 0;
    virtual bool localFileContentSniffingEnabled() const = 0; // FIXME: Reconcile with ResourceHandle::forceContentSniffing().
    virtual SchedulePairHashSet* scheduledRunLoopPairs() const { return 0; }
    virtual RetainPtr<CFDataRef> sourceApplicationAuditData() const = 0;
    virtual ResourceError blockedError(const ResourceRequest&) const = 0;
#endif

#if PLATFORM(MAC) || USE(CFNETWORK) || USE(SOUP)
    virtual NetworkStorageSession& storageSession() const = 0;
#endif

#if PLATFORM(QT)
    // FIXME: Wrap QNetworkAccessManager into a NetworkStorageSession to make the code cross-platform.
    virtual QObject* originatingObject() const = 0;
    virtual QNetworkAccessManager* networkAccessManager() const = 0;
    virtual bool mimeSniffingEnabled() const = 0;
    virtual bool thirdPartyCookiePolicyPermission(const QUrl&) const = 0;
#endif

#if PLATFORM(WIN)
    virtual String userAgent() const = 0;
    virtual String referrer() const = 0;
    virtual ResourceError blockedError(const ResourceRequest&) const = 0;
#endif

#if USE(SOUP)
    virtual uint64_t initiatingPageID() const = 0;
#endif

protected:
    NetworkingContext() { }
};

}

#endif // NetworkingContext_h

/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
              (C) 2012 Digia Plc. and/or its subsidiary(-ies)

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

#include "config.h"

#include "DNSResolveQueue.h"

#include <QHostInfo>
#include <QObject>
#include <QString>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DnsPrefetchHelper : public QObject {
    Q_OBJECT
public:
    DnsPrefetchHelper() : QObject() { }

public Q_SLOTS:
    void lookup(QString hostname)
    {
        if (hostname.isEmpty()) {
            DNSResolveQueue::shared().decrementRequestCount();
            return; // this actually happens
        }

        QHostInfo::lookupHost(hostname, this, SLOT(lookedUp(QHostInfo)));
    }

    void lookedUp(const QHostInfo&)
    {
        // we do not cache the result, we throw it away.
        // we currently rely on the OS to cache the results. If it does not do that
        // then at least the ISP nameserver did it.
        // Since Qt 4.6.3, Qt also has a small DNS cache.
        DNSResolveQueue::shared().decrementRequestCount();
    }
};

// This is called on mouse over a href and on page loading.
void prefetchDNS(const String& hostname)
{
    if (hostname.isEmpty())
        return;
    DNSResolveQueue::shared().add(hostname);
}

bool DNSResolveQueue::platformProxyIsEnabledInSystemPreferences()
{
    // Qt expects the system or a proxy to cache the result, but other platforms disable WebCore DNS prefetching when proxies are enabled.
    return false;
}

// This is called by the platform-independent DNSResolveQueue.
void DNSResolveQueue::platformResolve(const String& hostname)
{
    static DnsPrefetchHelper dnsPrefetchHelper;
    dnsPrefetchHelper.lookup(QString(hostname));
}

} // namespace

#include "DNSQt.moc"

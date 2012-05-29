/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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
#include "NetworkStateNotifier.h"

#if PLATFORM(QT) && USE(QT_BEARER)

#include "NetworkStateNotifierPrivate.h"
#include "qnetworkconfigmanager.h"

namespace WebCore {

NetworkStateNotifierPrivate::NetworkStateNotifierPrivate(NetworkStateNotifier* notifier)
    : m_configurationManager(new QNetworkConfigurationManager())
    , m_online(m_configurationManager->isOnline())
    , m_networkAccessAllowed(true)
    , m_notifier(notifier)
{
    Q_ASSERT(notifier);
    connect(m_configurationManager, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));
}

void NetworkStateNotifierPrivate::onlineStateChanged(bool isOnline)
{
    if (m_online == isOnline)
        return;

    m_online = isOnline;
    if (m_networkAccessAllowed)
        m_notifier->updateState();
}

void NetworkStateNotifierPrivate::networkAccessPermissionChanged(bool isAllowed)
{
    if (isAllowed == m_networkAccessAllowed)
        return;

    m_networkAccessAllowed = isAllowed;
    if (m_online)
        m_notifier->updateState();
}

NetworkStateNotifierPrivate::~NetworkStateNotifierPrivate()
{
    delete m_configurationManager;
}

void NetworkStateNotifier::updateState()
{
    if (m_isOnLine == (p->m_online && p->m_networkAccessAllowed))
        return;

    m_isOnLine = p->m_online && p->m_networkAccessAllowed;
    if (m_networkStateChangedFunction)
        m_networkStateChangedFunction();
}

NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(true)
    , m_networkStateChangedFunction(0)
{
    p = new NetworkStateNotifierPrivate(this);
    m_isOnLine = p->m_online && p->m_networkAccessAllowed;
}

void NetworkStateNotifier::setNetworkAccessAllowed(bool isAllowed)
{
    p->networkAccessPermissionChanged(isAllowed);
}

} // namespace WebCore

#include "moc_NetworkStateNotifierPrivate.cpp"

#endif

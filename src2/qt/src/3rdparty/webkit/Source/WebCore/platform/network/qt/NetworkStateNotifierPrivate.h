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

#ifndef NetworkStateNotifierPrivate_h
#define NetworkStateNotifierPrivate_h

#include <QObject>

QT_BEGIN_NAMESPACE
class QNetworkConfigurationManager;
QT_END_NAMESPACE

namespace WebCore {

class NetworkStateNotifier;

class NetworkStateNotifierPrivate : public QObject {
    Q_OBJECT
public:
    NetworkStateNotifierPrivate(NetworkStateNotifier* notifier);
    ~NetworkStateNotifierPrivate();
public slots:
    void onlineStateChanged(bool);
    void networkAccessPermissionChanged(bool);

public:
    QNetworkConfigurationManager* m_configurationManager;
    bool m_online;
    bool m_networkAccessAllowed;
    NetworkStateNotifier* m_notifier;
};

} // namespace WebCore

#endif

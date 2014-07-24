/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBBENGINE_H
#define QBBENGINE_H

#include "../qbearerengine_impl.h"

#include <QAbstractEventDispatcher>
#include <QAbstractNativeEventFilter>

#ifndef QT_NO_BEARERMANAGEMENT

struct bps_event_t;

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QNetworkSessionPrivate;

class QBBEngine : public QBearerEngineImpl, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit QBBEngine(QObject *parent = 0);
    ~QBBEngine();

    QString getInterfaceFromId(const QString &id) Q_DECL_OVERRIDE;
    bool hasIdentifier(const QString &id) Q_DECL_OVERRIDE;

    void connectToId(const QString &id) Q_DECL_OVERRIDE;
    void disconnectFromId(const QString &id) Q_DECL_OVERRIDE;

    Q_INVOKABLE void initialize() Q_DECL_OVERRIDE;
    Q_INVOKABLE void requestUpdate() Q_DECL_OVERRIDE;

    QNetworkSession::State sessionStateForId(const QString &id) Q_DECL_OVERRIDE;

    QNetworkConfigurationManager::Capabilities capabilities() const Q_DECL_OVERRIDE;

    QNetworkSessionPrivate *createSessionBackend() Q_DECL_OVERRIDE;

    QNetworkConfigurationPrivatePointer defaultConfiguration() Q_DECL_OVERRIDE;

    bool requiresPolling() const Q_DECL_OVERRIDE;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

protected:
    void updateConfiguration(const char *interface);
    void removeConfiguration(const QString &id);

private Q_SLOTS:
    void doRequestUpdate();

private:
    QHash<QString, QString> configurationInterface;

    mutable QMutex pollingMutex;

    bool pollingRequired;
    bool initialized;
};


QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QBBENGINE_H

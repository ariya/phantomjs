/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QHOSTINFO_H
#define QHOSTINFO_H

#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qhostaddress.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

class QObject;
class QHostInfoPrivate;

class Q_NETWORK_EXPORT QHostInfo
{
public:
    enum HostInfoError {
        NoError,
        HostNotFound,
        UnknownError
    };

    QHostInfo(int lookupId = -1);
    QHostInfo(const QHostInfo &d);
    QHostInfo &operator=(const QHostInfo &d);
    ~QHostInfo();

    QString hostName() const;
    void setHostName(const QString &name);

    QList<QHostAddress> addresses() const;
    void setAddresses(const QList<QHostAddress> &addresses);

    HostInfoError error() const;
    void setError(HostInfoError error);

    QString errorString() const;
    void setErrorString(const QString &errorString);

    void setLookupId(int id);
    int lookupId() const;

    static int lookupHost(const QString &name, QObject *receiver, const char *member);
    static void abortHostLookup(int lookupId);

    static QHostInfo fromName(const QString &name);
    static QString localHostName();
    static QString localDomainName();

private:
    QScopedPointer<QHostInfoPrivate> d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QHOSTINFO_H

/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qsessionmanager.h>

#ifndef QT_NO_SESSIONMANAGER

#include <qapplication.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QSessionManagerPrivate : public QObjectPrivate
{
public:
    QSessionManagerPrivate(QSessionManager *m, const QString &id,
                           const QString &key);

    QStringList restartCommand;
    QStringList discardCommand;
    const QString sessionId;
    const QString sessionKey;
    QSessionManager::RestartHint restartHint;
};

QSessionManagerPrivate::QSessionManagerPrivate(QSessionManager*,
                                               const QString &id,
                                               const QString &key)
    : QObjectPrivate(), sessionId(id), sessionKey(key)
{
}

QSessionManager::QSessionManager(QApplication *app, QString &id, QString &key)
    : QObject(*new QSessionManagerPrivate(this, id, key), app)
{
    Q_D(QSessionManager);
    d->restartHint = RestartIfRunning;
}

QSessionManager::~QSessionManager()
{
}

QString QSessionManager::sessionId() const
{
    Q_D(const QSessionManager);
    return d->sessionId;
}

QString QSessionManager::sessionKey() const
{
    Q_D(const QSessionManager);
    return d->sessionKey;
}


bool QSessionManager::allowsInteraction()
{
    return false;
}

bool QSessionManager::allowsErrorInteraction()
{
    return false;
}

void QSessionManager::release()
{
}

void QSessionManager::cancel()
{
}

void QSessionManager::setRestartHint(QSessionManager::RestartHint hint)
{
    Q_D(QSessionManager);
    d->restartHint = hint;
}

QSessionManager::RestartHint QSessionManager::restartHint() const
{
    Q_D(const QSessionManager);
    return d->restartHint;
}

void QSessionManager::setRestartCommand(const QStringList &command)
{
    Q_D(QSessionManager);
    d->restartCommand = command;
}

QStringList QSessionManager::restartCommand() const
{
    Q_D(const QSessionManager);
    return d->restartCommand;
}

void QSessionManager::setDiscardCommand(const QStringList &command)
{
    Q_D(QSessionManager);
    d->discardCommand = command;
}

QStringList QSessionManager::discardCommand() const
{
    Q_D(const QSessionManager);
    return d->discardCommand;
}

void QSessionManager::setManagerProperty(const QString &name,
                                         const QString &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

void QSessionManager::setManagerProperty(const QString &name,
                                         const QStringList &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

bool QSessionManager::isPhase2() const
{
    return false;
}

void QSessionManager::requestPhase2()
{
}

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER

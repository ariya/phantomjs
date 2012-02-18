/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMEVENTLOOPINTEGRATION_QPA_H
#define QPLATFORMEVENTLOOPINTEGRATION_QPA_H

#include <QtCore/qglobal.h>
#include <QtCore/QScopedPointer>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPlatformEventLoopIntegrationPrivate;

class Q_GUI_EXPORT QPlatformEventLoopIntegration
{
    Q_DECLARE_PRIVATE(QPlatformEventLoopIntegration);
public:
    QPlatformEventLoopIntegration();
    virtual ~QPlatformEventLoopIntegration();

    virtual void startEventLoop() = 0;
    virtual void quitEventLoop() = 0;
    virtual void qtNeedsToProcessEvents() = 0;

    qint64 nextTimerEvent() const;
    void setNextTimerEvent(qint64 nextTimerEvent);

    static void processEvents();
protected:
    QScopedPointer<QPlatformEventLoopIntegrationPrivate> d_ptr;
private:
    Q_DISABLE_COPY(QPlatformEventLoopIntegration);
    friend class QEventDispatcherQPA;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMEVENTLOOPINTEGRATION_QPA_H

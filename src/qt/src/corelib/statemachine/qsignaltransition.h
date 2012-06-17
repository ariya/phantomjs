/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSIGNALTRANSITION_H
#define QSIGNALTRANSITION_H

#include <QtCore/qabstracttransition.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_STATEMACHINE

class QSignalTransitionPrivate;
class Q_CORE_EXPORT QSignalTransition : public QAbstractTransition
{
    Q_OBJECT
    Q_PROPERTY(QObject* senderObject READ senderObject WRITE setSenderObject)
    Q_PROPERTY(QByteArray signal READ signal WRITE setSignal)
public:
    QSignalTransition(QState *sourceState = 0);
    QSignalTransition(QObject *sender, const char *signal,
                      QState *sourceState = 0);
    ~QSignalTransition();

    QObject *senderObject() const;
    void setSenderObject(QObject *sender);

    QByteArray signal() const;
    void setSignal(const QByteArray &signal);

protected:
    bool eventTest(QEvent *event);
    void onTransition(QEvent *event);

    bool event(QEvent *e);

private:
    Q_DISABLE_COPY(QSignalTransition)
    Q_DECLARE_PRIVATE(QSignalTransition)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

QT_END_HEADER

#endif

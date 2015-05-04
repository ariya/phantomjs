/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSIGNALTRANSITION_H
#define QSIGNALTRANSITION_H

#include <QtCore/qabstracttransition.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_STATEMACHINE

class QSignalTransitionPrivate;
class Q_CORE_EXPORT QSignalTransition : public QAbstractTransition
{
    Q_OBJECT
    Q_PROPERTY(QObject* senderObject READ senderObject WRITE setSenderObject NOTIFY senderObjectChanged)
    Q_PROPERTY(QByteArray signal READ signal WRITE setSignal NOTIFY signalChanged)

public:
    QSignalTransition(QState *sourceState = 0);
    QSignalTransition(const QObject *sender, const char *signal,
                      QState *sourceState = 0);
    ~QSignalTransition();

    QObject *senderObject() const;
    void setSenderObject(const QObject *sender);

    QByteArray signal() const;
    void setSignal(const QByteArray &signal);

protected:
    bool eventTest(QEvent *event);
    void onTransition(QEvent *event);

    bool event(QEvent *e);

Q_SIGNALS:
    void senderObjectChanged(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );
    void signalChanged(
#if !defined(Q_QDOC)
      QPrivateSignal
#endif
    );

private:
    Q_DISABLE_COPY(QSignalTransition)
    Q_DECLARE_PRIVATE(QSignalTransition)
};

#endif //QT_NO_STATEMACHINE

QT_END_NAMESPACE

#endif

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

#ifndef QGESTURERECOGNIZER_H
#define QGESTURERECOGNIZER_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

#ifndef QT_NO_GESTURES

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QObject;
class QEvent;
class QGesture;
class Q_GUI_EXPORT QGestureRecognizer
{
public:
    enum ResultFlag
    {
        Ignore           = 0x0001,

        MayBeGesture     = 0x0002,
        TriggerGesture   = 0x0004,
        FinishGesture    = 0x0008,
        CancelGesture    = 0x0010,

        ResultState_Mask = 0x00ff,

        ConsumeEventHint        = 0x0100,
        // StoreEventHint          = 0x0200,
        // ReplayStoredEventsHint  = 0x0400,
        // DiscardStoredEventsHint = 0x0800,

        ResultHint_Mask = 0xff00
    };
    Q_DECLARE_FLAGS(Result, ResultFlag)

    QGestureRecognizer();
    virtual ~QGestureRecognizer();

    virtual QGesture *create(QObject *target);
    virtual Result recognize(QGesture *state, QObject *watched,
                             QEvent *event) = 0;
    virtual void reset(QGesture *state);

    static Qt::GestureType registerRecognizer(QGestureRecognizer *recognizer);
    static void unregisterRecognizer(Qt::GestureType type);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGestureRecognizer::Result)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_GESTURES

#endif // QGESTURERECOGNIZER_H

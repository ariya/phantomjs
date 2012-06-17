/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMULTITOUCH_MAC_P_H
#define QMULTITOUCH_MAC_P_H

#ifdef QT_MAC_USE_COCOA
#import <Cocoa/Cocoa.h>
#endif

#include <qevent.h>
#include <qhash.h>
#include <QtCore>

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6

QT_BEGIN_NAMESPACE

#ifdef QT_MAC_USE_COCOA

class QCocoaTouch
{
    public:
        static QList<QTouchEvent::TouchPoint> getCurrentTouchPointList(NSEvent *event, bool acceptSingleTouch);
        static void setMouseInDraggingState(bool inDraggingState);

    private:
        static QHash<qint64, QCocoaTouch*> _currentTouches;
        static QPointF _screenReferencePos;
        static QPointF _trackpadReferencePos;
        static int _idAssignmentCount;
        static int _touchCount;
        static bool _updateInternalStateOnly;

        QTouchEvent::TouchPoint _touchPoint;
        qint64 _identity;

        QCocoaTouch(NSTouch *nstouch);
        ~QCocoaTouch();

        void updateTouchData(NSTouch *nstouch, NSTouchPhase phase);
        static QCocoaTouch *findQCocoaTouch(NSTouch *nstouch);
        static Qt::TouchPointState toTouchPointState(NSTouchPhase nsState);
};

#endif // QT_MAC_USE_COCOA

QT_END_NAMESPACE

#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6

#endif // QMULTITOUCH_MAC_P_H


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

#ifndef QCURSOR_P_H
#define QCURSOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qatomic.h"
#include "QtCore/qglobal.h"
#include "QtCore/qnamespace.h"
#include "QtGui/qpixmap.h"

# if defined (Q_WS_MAC)
#  include "private/qt_mac_p.h"
# elif defined(Q_WS_X11)
#  include "private/qt_x11_p.h"
# elif defined(Q_WS_WIN)
#  include "QtCore/qt_windows.h"
# elif defined(Q_OS_SYMBIAN)
#  include "private/qt_s60_p.h"
#endif

QT_BEGIN_NAMESPACE

#if defined (Q_WS_MAC)
void *qt_mac_nsCursorForQCursor(const QCursor &c);
class QMacAnimateCursor;
#endif

class QBitmap;
class QCursorData {
public:
    QCursorData(Qt::CursorShape s = Qt::ArrowCursor);
    ~QCursorData();

    static void initialize();
    static void cleanup();

    QAtomicInt ref;
    Qt::CursorShape cshape;
    QBitmap  *bm, *bmm;
    QPixmap pixmap;
    short     hx, hy;
#if defined (Q_WS_MAC)
    int mId;
#elif defined(Q_WS_QWS) || defined(Q_WS_QPA)
    int id;
#endif
#if defined (Q_WS_WIN)
    HCURSOR hcurs;
#elif defined (Q_WS_X11)
    XColor fg, bg;
    Cursor hcurs;
    Pixmap pm, pmm;
#elif defined (Q_WS_MAC)
    enum { TYPE_None, TYPE_ImageCursor, TYPE_ThemeCursor } type;
    union {
        struct {
            uint my_cursor:1;
            void *nscursor;
        } cp;
        struct {
            QMacAnimateCursor *anim;
            ThemeCursor curs;
        } tc;
    } curs;
    void initCursorFromBitmap();
    void initCursorFromPixmap();
#elif defined Q_OS_SYMBIAN
    void loadShapeFromResource(RWsSpriteBase& target, QString resource, int hx, int hy, int interval=0);
    void constructShapeSprite(RWsSpriteBase& target);
    void constructCursorSprite(RWsSpriteBase& target);
    RWsPointerCursor pcurs;
    RWsSprite scurs;
    RPointerArray<TSpriteMember> nativeSpriteMembers;
#endif
    static bool initialized;
    void update();
    static QCursorData *setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY);
};

extern QCursorData *qt_cursorTable[Qt::LastCursor + 1]; // qcursor.cpp

QT_END_NAMESPACE

#endif // QCURSOR_P_H

/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWINDOWDEFS_H
#define QWINDOWDEFS_H

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE


// Class forward definitions

class QPaintDevice;
class QWidget;
class QWindow;
class QDialog;
class QColor;
class QPalette;
class QCursor;
class QPoint;
class QSize;
class QRect;
class QPolygon;
class QPainter;
class QRegion;
class QFont;
class QFontMetrics;
class QFontInfo;
class QPen;
class QBrush;
class QMatrix;
class QPixmap;
class QBitmap;
class QMovie;
class QImage;
class QPicture;
class QTimer;
class QTime;
class QClipboard;
class QString;
class QByteArray;
class QApplication;

template<typename T> class QList;
typedef QList<QWidget *> QWidgetList;
typedef QList<QWindow *> QWindowList;

QT_END_NAMESPACE

// Window system dependent definitions


#if defined(Q_OS_WIN)
#  include <QtGui/qwindowdefs_win.h>
#endif // Q_OS_WIN




typedef QT_PREPEND_NAMESPACE(quintptr) WId;



QT_BEGIN_NAMESPACE

template<class K, class V> class QHash;
typedef QHash<WId, QWidget *> QWidgetMapper;

template<class V> class QSet;
typedef QSet<QWidget *> QWidgetSet;

QT_END_NAMESPACE

#if defined(QT_NEEDS_QMAIN)
#define main qMain
#endif

// Global platform-independent types and functions

#endif // QWINDOWDEFS_H

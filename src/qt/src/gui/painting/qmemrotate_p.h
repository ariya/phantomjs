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

#ifndef QMEMROTATE_P_H
#define QMEMROTATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qdrawhelper_p.h"

QT_BEGIN_NAMESPACE

#define QT_ROTATION_CACHEDREAD 1
#define QT_ROTATION_CACHEDWRITE 2
#define QT_ROTATION_PACKING 3
#define QT_ROTATION_TILED 4

#ifndef QT_ROTATION_ALGORITHM
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define QT_ROTATION_ALGORITHM QT_ROTATION_TILED
#else
#define QT_ROTATION_ALGORITHM QT_ROTATION_CACHEDREAD
#endif
#endif

#ifdef Q_WS_QWS
#define Q_GUI_QWS_EXPORT Q_GUI_EXPORT
#else
#define Q_GUI_QWS_EXPORT
#endif

#define QT_DECL_MEMROTATE(srctype, desttype)                            \
    void Q_GUI_QWS_EXPORT qt_memrotate90(const srctype*, int, int, int, desttype*, int); \
    void Q_GUI_QWS_EXPORT qt_memrotate180(const srctype*, int, int, int, desttype*, int); \
    void Q_GUI_QWS_EXPORT qt_memrotate270(const srctype*, int, int, int, desttype*, int)

void Q_GUI_EXPORT qt_memrotate90(const quint32*, int, int, int, quint32*, int);
void Q_GUI_QWS_EXPORT qt_memrotate180(const quint32*, int, int, int, quint32*, int);
void Q_GUI_QWS_EXPORT qt_memrotate270(const quint32*, int, int, int, quint32*, int);

QT_DECL_MEMROTATE(quint32, quint16);
QT_DECL_MEMROTATE(quint16, quint32);
QT_DECL_MEMROTATE(quint16, quint16);
QT_DECL_MEMROTATE(quint24, quint24);
QT_DECL_MEMROTATE(quint32, quint24);
QT_DECL_MEMROTATE(quint32, quint18);
QT_DECL_MEMROTATE(quint32, quint8);
QT_DECL_MEMROTATE(quint16, quint8);
QT_DECL_MEMROTATE(qrgb444, quint8);
QT_DECL_MEMROTATE(quint8, quint8);

#ifdef QT_QWS_ROTATE_BGR
QT_DECL_MEMROTATE(quint16, qbgr565);
QT_DECL_MEMROTATE(quint32, qbgr565);
QT_DECL_MEMROTATE(qrgb555, qbgr555);
QT_DECL_MEMROTATE(quint32, qbgr555);
#endif

#ifdef QT_QWS_DEPTH_GENERIC
QT_DECL_MEMROTATE(quint32, qrgb_generic16);
QT_DECL_MEMROTATE(quint16, qrgb_generic16);
#endif

#undef QT_DECL_MEMROTATE

QT_END_NAMESPACE

#endif // QMEMROTATE_P_H

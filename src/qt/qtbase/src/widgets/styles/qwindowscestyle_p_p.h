/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QWINDOWSCESTYLE_P_P_H
#define QWINDOWSCESTYLE_P_P_H

#include "qwindowscestyle_p.h"
#include <private/qwindowsstyle_p_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QPainter;
class QPalette;
class QPoint;
class QColor;
class QBrush;
class QRect;

// Private class
class QWindowsCEStylePrivate : public QWindowsStylePrivate
{
    Q_DECLARE_PUBLIC(QWindowsCEStyle)
public:
    inline QWindowsCEStylePrivate()
    { }


static void drawWinCEButton(QPainter *p, int x, int y, int w, int h,
                            const QPalette &pal, bool sunken = false,
                            const QBrush *fill = 0);

static void drawWinCEButton(QPainter *p, const QRect &r,
                            const QPalette &pal, bool sunken = false,
                            const QBrush *fill = 0);

static void drawWinCEPanel(QPainter *p, int x, int y, int w, int h,
                           const QPalette &pal, bool sunken = false,
                           const QBrush *fill = 0);

static void drawWinCEPanel(QPainter *p, const QRect &r,
                           const QPalette &pal, bool sunken = false,
                           const QBrush *fill = 0);

static void drawWinShades(QPainter *p,
                          int x, int y, int w, int h,
                          const QColor &c1, const QColor &c2,
                          const QColor &c3, const QColor &c4,
                          const QBrush *fill);

static void drawWinCEShades(QPainter *p,
                            int x, int y, int w, int h,
                            const QColor &c1, const QColor &c2,
                            const QColor &c3, const QColor &c4,
                            const QBrush *fill);

static void drawWinCEShadesSunken(QPainter *p,
                                  int x, int y, int w, int h,
                                  const QColor &c1, const QColor &c2,
                                  const QColor &c3, const QColor &c4,
                                  const QBrush *fill);




};

QT_END_NAMESPACE

#endif //QWINDOWSCESTYLE_P_P_H

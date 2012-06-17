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

#include "qwidget.h"
#include "qt_x11_p.h"

/*
  Internal Qt functions to create X windows.  We have put them in
  separate functions to allow the programmer to reimplement them by
  custom versions.
*/

QT_BEGIN_NAMESPACE

Window qt_XCreateWindow(const QWidget *, Display *display, Window parent,
                         int x, int y, uint w, uint h,
                         int borderwidth, int depth,
                         uint windowclass, Visual *visual,
                         ulong valuemask, XSetWindowAttributes *attributes)
{
    return XCreateWindow(display, parent, x, y, w, h, borderwidth, depth,
                          windowclass, visual, valuemask, attributes);
}


Window qt_XCreateSimpleWindow(const QWidget *, Display *display, Window parent,
                               int x, int y, uint w, uint h, int borderwidth,
                               ulong border, ulong background)
{
    return XCreateSimpleWindow(display, parent, x, y, w, h, borderwidth,
                                border, background);
}


void qt_XDestroyWindow(const QWidget *, Display *display, Window window)
{
    if (window)
        XDestroyWindow(display, window);
}

QT_END_NAMESPACE

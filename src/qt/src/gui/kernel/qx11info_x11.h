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

#ifndef QX11INFO_X11_H
#define QX11INFO_X11_H

#include <QtCore/qnamespace.h>

typedef struct _XDisplay Display;

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

struct QX11InfoData;
class QX11Info;
class QPaintDevice;
class QApplicationPrivate;
class QX11InfoPrivate;
struct QX11WindowAttributes;

void qt_x11_getX11InfoForWindow(QX11Info * xinfo, const QX11WindowAttributes &a);
class Q_GUI_EXPORT QX11Info
{
public:
    QX11Info();
    ~QX11Info();
    QX11Info(const QX11Info &other);
    QX11Info &operator=(const QX11Info &other);

    static Display *display();
    static const char *appClass();
    int screen() const;
    int depth() const;
    int cells() const;
    Qt::HANDLE colormap() const;
    bool defaultColormap() const;
    void *visual() const;
    bool defaultVisual() const;

    static int appScreen();
    static int appDepth(int screen = -1);
    static int appCells(int screen = -1);
    static Qt::HANDLE appColormap(int screen = -1);
    static void *appVisual(int screen = -1);
    static Qt::HANDLE appRootWindow(int screen = -1);
    static bool appDefaultColormap(int screen = -1);
    static bool appDefaultVisual(int screen = -1);
    static int appDpiX(int screen = -1);
    static int appDpiY(int screen = -1);
    static void setAppDpiX(int screen, int dpi);
    static void setAppDpiY(int screen, int dpi);
    static unsigned long appTime();
    static unsigned long appUserTime();
    static void setAppTime(unsigned long time);
    static void setAppUserTime(unsigned long time);
    static bool isCompositingManagerRunning();

protected:
    void copyX11Data(const QPaintDevice *);
    void cloneX11Data(const QPaintDevice *);
    void setX11Data(const QX11InfoData *);
    QX11InfoData* getX11Data(bool def = false) const;

    QX11InfoData *x11data;

    friend class QX11PaintEngine;
    friend class QPixmap;
    friend class QX11PixmapData;
    friend class QWidget;
    friend class QWidgetPrivate;
    friend class QGLWidget;
    friend void qt_init(QApplicationPrivate *priv, int, Display *display, Qt::HANDLE visual,
                        Qt::HANDLE colormap);
    friend void qt_cleanup();
    friend void qt_x11_getX11InfoForWindow(QX11Info * xinfo, const QX11WindowAttributes &a);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QX11INFO_X11_H

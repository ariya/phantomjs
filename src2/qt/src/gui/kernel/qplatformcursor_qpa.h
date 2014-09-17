/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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
#ifndef QGRAPHICSSYSTEMCURSOR_H
#define QGRAPHICSSYSTEMCURSOR_H

#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtCore/QWeakPointer>
#include <QtCore/QObject>
#include <QtGui/QPlatformScreen>
#include <QtGui/QCursor>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

// Cursor graphics management
class Q_GUI_EXPORT QPlatformCursorImage {
public:
    QPlatformCursorImage(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
    { set(data, mask, width, height, hotX, hotY); }
    QImage * image() { return &cursorImage; }
    QPoint hotspot() { return hot; }
    void set(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY);
    void set(const QImage &image, int hx, int hy);
    void set(Qt::CursorShape);
private:
    static void createSystemCursor(int id);
    QImage cursorImage;
    QPoint hot;
};

class QPlatformCursor;

class QPlatformCursorPrivate {
public:
    static QList<QWeakPointer<QPlatformCursor> > getInstances() { return instances; }
    static QList<QWeakPointer<QPlatformCursor> > instances;
};

class Q_GUI_EXPORT QPlatformCursor : public QObject {
public:
    QPlatformCursor(QPlatformScreen *);

    // input methods
    virtual void pointerEvent(const QMouseEvent & event) { Q_UNUSED(event); }
    virtual void changeCursor(QCursor * widgetCursor, QWidget * widget) = 0;

protected:
    QPlatformScreen* screen;  // Where to request an update

private:
    Q_DECLARE_PRIVATE(QPlatformCursor);
    friend void qt_qpa_set_cursor(QWidget * w, bool force);
    friend class QApplicationPrivate;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGRAPHICSSYSTEMCURSOR_H

/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QOFFSCREENCOMMON_H
#define QOFFSCREENCOMMON_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformdrag.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformwindow.h>

#include <qscopedpointer.h>
#include <qimage.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

class QOffscreenScreen : public QPlatformScreen
{
public:
    QOffscreenScreen();

    QRect geometry() const { return m_geometry; }
    int depth() const { return 32; }
    QImage::Format format() const { return QImage::Format_RGB32; }
    QPlatformCursor *cursor() const { return m_cursor.data(); }

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    static QPlatformWindow *windowContainingCursor;

public:
    QRect m_geometry;
    QScopedPointer<QPlatformCursor> m_cursor;
};

#ifndef QT_NO_DRAGANDDROP
class QOffscreenDrag : public QPlatformDrag
{
public:
    QMimeData *platformDropData() { return 0; }
    Qt::DropAction drag(QDrag *) { return Qt::IgnoreAction; }
};
#endif

class QOffscreenBackingStore : public QPlatformBackingStore
{
public:
    QOffscreenBackingStore(QWindow *window);
    ~QOffscreenBackingStore();

    QPaintDevice *paintDevice();
    void flush(QWindow *window, const QRegion &region, const QPoint &offset);
    void resize(const QSize &size, const QRegion &staticContents);
    bool scroll(const QRegion &area, int dx, int dy);

    QPixmap grabWindow(WId window, const QRect &rect) const;

    static QOffscreenBackingStore *backingStoreForWinId(WId id);

private:
    void clearHash();

    QImage m_image;
    QHash<WId, QRect> m_windowAreaHash;

    static QHash<WId, QOffscreenBackingStore *> m_backingStoreForWinIdHash;
};

QT_END_NAMESPACE

#endif

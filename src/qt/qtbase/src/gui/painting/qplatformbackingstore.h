/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QPLATFORMBACKINGSTORE_H
#define QPLATFORMBACKINGSTORE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qrect.h>
#include <QtCore/qobject.h>

#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE


class QRegion;
class QRect;
class QPoint;
class QImage;
class QPlatformBackingStorePrivate;
class QPlatformWindow;
class QPlatformTextureList;
class QPlatformTextureListPrivate;
class QOpenGLContext;

#ifndef QT_NO_OPENGL
class Q_GUI_EXPORT QPlatformTextureList : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPlatformTextureList)
public:
    explicit QPlatformTextureList(QObject *parent = 0);
    ~QPlatformTextureList();

    int count() const;
    bool isEmpty() const { return count() == 0; }
    GLuint textureId(int index) const;
    QRect geometry(int index) const;
    void lock(bool on);
    bool isLocked() const;

    void appendTexture(GLuint textureId, const QRect &geometry);
    void clear();

 Q_SIGNALS:
    void locked(bool);
};
#endif

class Q_GUI_EXPORT QPlatformBackingStore
{
public:
    explicit QPlatformBackingStore(QWindow *window);
    virtual ~QPlatformBackingStore();

    QWindow *window() const;

    virtual QPaintDevice *paintDevice() = 0;

    // 'window' can be a child window, in which case 'region' is in child window coordinates and
    // offset is the (child) window's offset in relation to the window surface.
    virtual void flush(QWindow *window, const QRegion &region, const QPoint &offset) = 0;
#ifndef QT_NO_OPENGL
    virtual void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset, QPlatformTextureList *textures, QOpenGLContext *context);
    virtual QImage toImage() const;
    virtual GLuint toTexture(const QRegion &dirtyRegion, QSize *textureSize) const;
#endif

    virtual void resize(const QSize &size, const QRegion &staticContents) = 0;

    virtual bool scroll(const QRegion &area, int dx, int dy);

    virtual void beginPaint(const QRegion &);
    virtual void endPaint();

private:
    QPlatformBackingStorePrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QPLATFORMBACKINGSTORE_H

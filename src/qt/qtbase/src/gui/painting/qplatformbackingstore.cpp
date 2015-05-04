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

#include "qplatformbackingstore.h"
#include <qwindow.h>
#include <qpixmap.h>
#include <private/qwindow_p.h>

#include <qopengl.h>
#include <qopenglcontext.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#ifndef QT_NO_OPENGL
#include <QtGui/private/qopengltextureblitter_p.h>
#endif

QT_BEGIN_NAMESPACE

class QPlatformBackingStorePrivate
{
public:
    QPlatformBackingStorePrivate(QWindow *w)
        : window(w)
#ifndef QT_NO_OPENGL
        , textureId(0)
        , blitter(0)
#endif
    {
    }

    ~QPlatformBackingStorePrivate()
    {
#ifndef QT_NO_OPENGL
        if (blitter)
            blitter->destroy();
        delete blitter;
#endif
    }
    QWindow *window;
    QSize size;
#ifndef QT_NO_OPENGL
    mutable GLuint textureId;
    mutable QSize textureSize;
    mutable bool needsSwizzle;
    QOpenGLTextureBlitter *blitter;
#endif
};

#ifndef QT_NO_OPENGL

struct QBackingstoreTextureInfo
{
    QWidget *widget; // may be null
    GLuint textureId;
    QRect rect;
    QPlatformTextureList::Flags flags;
};

Q_DECLARE_TYPEINFO(QBackingstoreTextureInfo, Q_MOVABLE_TYPE);

class QPlatformTextureListPrivate : public QObjectPrivate
{
public:
    QPlatformTextureListPrivate()
        : locked(false)
    {
    }

    QList<QBackingstoreTextureInfo> textures;
    bool locked;
};

QPlatformTextureList::QPlatformTextureList(QObject *parent)
: QObject(*new QPlatformTextureListPrivate, parent)
{
}

QPlatformTextureList::~QPlatformTextureList()
{
}

int QPlatformTextureList::count() const
{
    Q_D(const QPlatformTextureList);
    return d->textures.count();
}

GLuint QPlatformTextureList::textureId(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).textureId;
}

QWidget *QPlatformTextureList::widget(int index)
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).widget;
}

QPlatformTextureList::Flags QPlatformTextureList::flags(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).flags;
}

QRect QPlatformTextureList::geometry(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).rect;
}

void QPlatformTextureList::lock(bool on)
{
    Q_D(QPlatformTextureList);
    if (on != d->locked) {
        d->locked = on;
        emit locked(on);
    }
}

bool QPlatformTextureList::isLocked() const
{
    Q_D(const QPlatformTextureList);
    return d->locked;
}

void QPlatformTextureList::appendTexture(QWidget *widget, GLuint textureId, const QRect &geometry, Flags flags)
{
    Q_D(QPlatformTextureList);
    QBackingstoreTextureInfo bi;
    bi.widget = widget;
    bi.textureId = textureId;
    bi.rect = geometry;
    bi.flags = flags;
    d->textures.append(bi);
}

void QPlatformTextureList::clear()
{
    Q_D(QPlatformTextureList);
    d->textures.clear();
}
#endif // QT_NO_OPENGL

/*!
    \class QPlatformBackingStore
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformBackingStore class provides the drawing area for top-level
    windows.
*/

/*!
    \fn void QPlatformBackingStore::flush(QWindow *window, const QRegion &region,
                                  const QPoint &offset)

    Flushes the given \a region from the specified \a window onto the
    screen.

    Note that the \a offset parameter is currently unused.
*/

#ifndef QT_NO_OPENGL

static QRect deviceRect(const QRect &rect, QWindow *window)
{
    QRect deviceRect(rect.topLeft() * window->devicePixelRatio(),
                     rect.size() * window->devicePixelRatio());
    return deviceRect;
}

static QRegion deviceRegion(const QRegion &region, QWindow *window)
{
    if (!(window->devicePixelRatio() > 1))
        return region;

    QVector<QRect> rects;
    foreach (QRect rect, region.rects())
        rects.append(deviceRect(rect, window));

    QRegion deviceRegion;
    deviceRegion.setRects(rects.constData(), rects.count());
    return deviceRegion;
}

/*!
    Flushes the given \a region from the specified \a window onto the
    screen, and composes it with the specified \a textures.

    The default implementation retrieves the contents using toTexture()
    and composes using OpenGL. May be reimplemented in subclasses if there
    is a more efficient native way to do it.

    Note that the \a offset parameter is currently unused.
 */

void QPlatformBackingStore::composeAndFlush(QWindow *window, const QRegion &region,
                                            const QPoint &offset,
                                            QPlatformTextureList *textures, QOpenGLContext *context,
                                            bool translucentBackground)
{
    Q_UNUSED(offset);

    context->makeCurrent(window);
    QOpenGLFunctions *funcs = context->functions();
    funcs->glViewport(0, 0, window->width() * window->devicePixelRatio(), window->height() * window->devicePixelRatio());
    funcs->glClearColor(0, 0, 0, translucentBackground ? 0 : 1);
    funcs->glClear(GL_COLOR_BUFFER_BIT);

    if (!d_ptr->blitter) {
        d_ptr->blitter = new QOpenGLTextureBlitter;
        d_ptr->blitter->create();
    }

    d_ptr->blitter->bind();

    QRect windowRect(QPoint(), window->size() * window->devicePixelRatio());

    // Textures for renderToTexture widgets.
    for (int i = 0; i < textures->count(); ++i) {
        if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            QRect targetRect = deviceRect(textures->geometry(i), window);
            QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, windowRect);
            d_ptr->blitter->blit(textures->textureId(i), target, QOpenGLTextureBlitter::OriginBottomLeft);
        }
    }

    funcs->glEnable(GL_BLEND);
    funcs->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Do not write out alpha. We need blending, but only for RGB. The toplevel may have
    // alpha enabled in which case blending (writing out < 1.0 alpha values) would lead to
    // semi-transparency even when it is not wanted.
    funcs->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    // Backingstore texture with the normal widgets.
    GLuint textureId = toTexture(deviceRegion(region, window), &d_ptr->textureSize, &d_ptr->needsSwizzle);
    if (textureId) {
        QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(QRect(QPoint(), d_ptr->textureSize), windowRect);
        if (d_ptr->needsSwizzle)
            d_ptr->blitter->setSwizzleRB(true);
        d_ptr->blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        if (d_ptr->needsSwizzle)
            d_ptr->blitter->setSwizzleRB(false);
    }

    // Textures for renderToTexture widgets that have WA_AlwaysStackOnTop set.
    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            QRect targetRect = deviceRect(textures->geometry(i), window);
            QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, windowRect);
            d_ptr->blitter->blit(textures->textureId(i), target, QOpenGLTextureBlitter::OriginBottomLeft);
        }
    }

    funcs->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    funcs->glDisable(GL_BLEND);
    d_ptr->blitter->release();

    context->swapBuffers(window);
}

/*!
  Implemented in subclasses to return the content of the backingstore as a QImage.

  If QPlatformIntegration::RasterGLSurface is supported, either this function or
  toTexture() must be implemented.

  \sa toTexture()
 */
QImage QPlatformBackingStore::toImage() const
{
    return QImage();
}

/*!
  May be reimplemented in subclasses to return the content of the
  backingstore as an OpenGL texture. \a dirtyRegion is the part of the
  backingstore which may have changed since the last call to this function. The
  caller of this function must ensure that there is a current context.
  The size of the texture is returned in \a textureSize.

  The ownership of the texture is not transferred. The caller must not store
  the return value between calls, but instead call this function before each use.

  The default implementation returns a cached texture if \a dirtyRegion is
  empty and the window has not been resized, otherwise it retrieves the
  content using toImage() and performs a texture upload.

  If the red and blue components have to swapped, \a needsSwizzle will be set to \c true.
  This allows creating textures from images in formats like QImage::Format_RGB32 without
  any further image conversion. Instead, the swizzling will be done in the shaders when
  performing composition. Other formats, that do not need such swizzling due to being
  already byte ordered RGBA, for example QImage::Format_RGBA8888, must result in having \a
  needsSwizzle set to false.
 */
GLuint QPlatformBackingStore::toTexture(const QRegion &dirtyRegion, QSize *textureSize, bool *needsSwizzle) const
{
    QImage image = toImage();
    QSize imageSize = image.size();
    if (imageSize.isEmpty())
        return 0;

    bool resized = d_ptr->textureSize != imageSize;
    if (dirtyRegion.isEmpty() && !resized)
        return d_ptr->textureId;

    // Fast path for RGB32 and RGBA8888, convert everything else to RGBA8888.
    if (image.format() == QImage::Format_RGB32) {
        if (needsSwizzle)
            *needsSwizzle = true;
    } else {
        if (needsSwizzle)
            *needsSwizzle = false;
        if (image.format() != QImage::Format_RGBA8888)
            image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();

    if (resized) {
        if (d_ptr->textureId)
            funcs->glDeleteTextures(1, &d_ptr->textureId);
        funcs->glGenTextures(1, &d_ptr->textureId);
        funcs->glBindTexture(GL_TEXTURE_2D, d_ptr->textureId);
#ifndef QT_OPENGL_ES_2
        if (!QOpenGLContext::currentContext()->isOpenGLES()) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        }
#endif
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageSize.width(), imageSize.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                            const_cast<uchar*>(image.constBits()));
        if (textureSize)
            *textureSize = imageSize;
    } else {
        funcs->glBindTexture(GL_TEXTURE_2D, d_ptr->textureId);
        QRect imageRect = image.rect();
        QRect rect = dirtyRegion.boundingRect() & imageRect;

#ifndef QT_OPENGL_ES_2
        if (!QOpenGLContext::currentContext()->isOpenGLES()) {
            funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, image.width());
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                   image.constScanLine(rect.y()) + rect.x() * 4);
            funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        } else
#endif
        {
            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (rect.width() >= imageRect.width() / 2) {
                rect.setX(0);
                rect.setWidth(imageRect.width());
            }

            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines

            if (rect.width() == imageRect.width()) {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                       image.constScanLine(rect.y()));
            } else {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                       image.copy(rect).constBits());
            }
        }
    }

    return d_ptr->textureId;
}
#endif // QT_NO_OPENGL

/*!
    \fn QPaintDevice* QPlatformBackingStore::paintDevice()

    Implement this function to return the appropriate paint device.
*/

/*!
    Constructs an empty surface for the given top-level \a window.
*/
QPlatformBackingStore::QPlatformBackingStore(QWindow *window)
    : d_ptr(new QPlatformBackingStorePrivate(window))
{
}

/*!
    Destroys this surface.
*/
QPlatformBackingStore::~QPlatformBackingStore()
{
    delete d_ptr;
}

/*!
    Returns a pointer to the top-level window associated with this
    surface.
*/
QWindow* QPlatformBackingStore::window() const
{
    return d_ptr->window;
}

/*!
    This function is called before painting onto the surface begins,
    with the \a region in which the painting will occur.

    \sa endPaint(), paintDevice()
*/

void QPlatformBackingStore::beginPaint(const QRegion &)
{
}

/*!
    This function is called after painting onto the surface has ended.

    \sa beginPaint(), paintDevice()
*/

void QPlatformBackingStore::endPaint()
{
}

/*!
    Scrolls the given \a area \a dx pixels to the right and \a dy
    downward; both \a dx and \a dy may be negative.

    Returns \c true if the area was scrolled successfully; false otherwise.
*/
bool QPlatformBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    Q_UNUSED(area);
    Q_UNUSED(dx);
    Q_UNUSED(dy);

    return false;
}

QT_END_NAMESPACE

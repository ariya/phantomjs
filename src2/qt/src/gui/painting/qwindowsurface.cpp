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

#include <private/qwindowsurface_p.h>
#include <qwidget.h>
#include <private/qwidget_p.h>
#include <private/qbackingstore_p.h>
#include <private/qapplication_p.h>

QT_BEGIN_NAMESPACE

class QWindowSurfacePrivate
{
public:
    QWindowSurfacePrivate(QWidget *w)
        : window(w)
    {
    }

    QWidget *window;
#if !defined(Q_WS_QPA)
    QRect geometry;
#else
    QSize size;
#endif //Q_WS_QPA
    QRegion staticContents;
    QList<QImage*> bufferImages;
};

/*!
    \class QWindowSurface
    \since 4.3
    \internal
    \preliminary
    \ingroup qws qpa

    \brief The QWindowSurface class provides the drawing area for top-level
    windows.
*/


/*!
    \fn void QWindowSurface::beginPaint(const QRegion &region)

    This function is called before painting onto the surface begins,
    with the \a region in which the painting will occur.

    \sa endPaint(), paintDevice()
*/

/*!
    \fn void QWindowSurface::endPaint(const QRegion &region)

    This function is called after painting onto the surface has ended,
    with the \a region in which the painting was performed.

    \sa beginPaint(), paintDevice()
*/

/*!
    \fn void QWindowSurface::flush(QWidget *widget, const QRegion &region,
                                  const QPoint &offset)

    Flushes the given \a region from the specified \a widget onto the
    screen.

    Note that the \a offset parameter is currently unused.
*/

/*!
    \fn QPaintDevice* QWindowSurface::paintDevice()

    Implement this function to return the appropriate paint device.
*/

/*!
    Constructs an empty surface for the given top-level \a window.
*/
QWindowSurface::QWindowSurface(QWidget *window, bool setDefaultSurface)
    : d_ptr(new QWindowSurfacePrivate(window))
{
    if (!QApplicationPrivate::runtime_graphics_system) {
        if(setDefaultSurface && window)
            window->setWindowSurface(this);
    }
}

/*!
    Destroys this surface.
*/
QWindowSurface::~QWindowSurface()
{
    if (d_ptr->window)
        d_ptr->window->d_func()->extra->topextra->windowSurface = 0;
    delete d_ptr;
}

/*!
    Returns a pointer to the top-level window associated with this
    surface.
*/
QWidget* QWindowSurface::window() const
{
    return d_ptr->window;
}

void QWindowSurface::beginPaint(const QRegion &)
{
}

void QWindowSurface::endPaint(const QRegion &)
{
//     QApplication::syncX();
    qDeleteAll(d_ptr->bufferImages);
    d_ptr->bufferImages.clear();
}

#if !defined(Q_WS_QPA)
/*!
    Sets the currently allocated area to be the given \a rect.

    This function is called whenever area covered by the top-level
    window changes.

    \sa geometry()
*/
void QWindowSurface::setGeometry(const QRect &rect)
{
    d_ptr->geometry = rect;
}

/*!
    Returns the currently allocated area on the screen.
*/
QRect QWindowSurface::geometry() const
{
    return d_ptr->geometry;
}
#else

/*!
      Sets the size of the windowsurface to be \a size.

      \sa size()
*/
void QWindowSurface::resize(const QSize &size)
{
    d_ptr->size = size;
}

/*!
    Returns the current size of the windowsurface.
*/
QSize QWindowSurface::size() const
{
    return d_ptr->size;
}
#endif //Q_WS_QPA

/*!
    Scrolls the given \a area \a dx pixels to the right and \a dy
    downward; both \a dx and \a dy may be negative.

    Returns true if the area was scrolled successfully; false otherwise.
*/
bool QWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    Q_UNUSED(area);
    Q_UNUSED(dx);
    Q_UNUSED(dy);

    return false;
}

/*!
    Returns a QImage pointer which represents the actual buffer the \a widget
    is drawn into or 0 if this is unavailable.

    You must call beginPaint() before you call this function and the returned
    pointer is only valid until endPaint() is called.
*/
QImage* QWindowSurface::buffer(const QWidget *widget)
{
    if (widget->window() != window())
        return 0;

    QPaintDevice *pdev = paintDevice();
    if (!pdev || pdev->devType() != QInternal::Image)
        return 0;

    const QPoint off = offset(widget);
    QImage *img = static_cast<QImage*>(pdev);

    QRect rect(off, widget->size());
    rect &= QRect(QPoint(), img->size());

    if (rect.isEmpty())
        return 0;

    img = new QImage(img->scanLine(rect.y()) + rect.x() * img->depth() / 8,
                     rect.width(), rect.height(),
                     img->bytesPerLine(), img->format());
    d_ptr->bufferImages.append(img);

    return img;
}

/*!
  Returns a QPixmap generated from the part of the backing store
  corresponding to \a widget. Returns a null QPixmap if an error
  occurs. The contents of the pixmap are only defined for the regions
  of \a widget that have received paint events since the last resize
  of the backing store.

  If \a rectangle is a null rectangle (the default), the entire widget
  is grabbed. Otherwise, the grabbed area is limited to \a rectangle.

  The default implementation uses QWindowSurface::buffer().

  \sa QPixmap::grabWidget()
*/
QPixmap QWindowSurface::grabWidget(const QWidget *widget, const QRect &rectangle) const
{
    QPixmap result;

    if (widget->window() != window())
        return result;

    const QImage *img = const_cast<QWindowSurface *>(this)->buffer(widget->window());

    if (!img || img->isNull())
        return result;

    QRect rect = rectangle.isEmpty() ? widget->rect() : (widget->rect() & rectangle);

    rect.translate(offset(widget) - offset(widget->window()));
    rect &= QRect(QPoint(), img->size());

    if (rect.isEmpty())
        return result;

    QImage subimg(img->scanLine(rect.y()) + rect.x() * img->depth() / 8,
                  rect.width(), rect.height(),
                  img->bytesPerLine(), img->format());
    subimg.detach(); //### expensive -- maybe we should have a real SubImage that shares reference count

    result = QPixmap::fromImage(subimg);
    return result;
}

/*!
  Returns the offset of \a widget in the coordinates of this
  window surface.
 */
QPoint QWindowSurface::offset(const QWidget *widget) const
{
    QWidget *window = d_ptr->window;
    QPoint offset = widget->mapTo(window, QPoint());
#ifdef Q_WS_QWS
    offset += window->geometry().topLeft() - window->frameGeometry().topLeft();
#endif
    return offset;
}

/*!
  \fn QRect QWindowSurface::rect(const QWidget *widget) const

  Returns the rectangle for \a widget in the coordinates of this
  window surface.
*/

void QWindowSurface::setStaticContents(const QRegion &region)
{
    d_ptr->staticContents = region;
}

QRegion QWindowSurface::staticContents() const
{
    return d_ptr->staticContents;
}

bool QWindowSurface::hasStaticContents() const
{
    return hasFeature(QWindowSurface::StaticContents) && !d_ptr->staticContents.isEmpty();
}

QWindowSurface::WindowSurfaceFeatures QWindowSurface::features() const
{
    return PartialUpdates | PreservedContents;
}

#ifdef Q_WS_QPA
#define Q_EXPORT_SCROLLRECT Q_GUI_EXPORT
#else
#define Q_EXPORT_SCROLLRECT
#endif

void Q_EXPORT_SCROLLRECT qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset)
{
    // make sure we don't detach
    uchar *mem = const_cast<uchar*>(const_cast<const QImage &>(img).bits());

    int lineskip = img.bytesPerLine();
    int depth = img.depth() >> 3;

    const QRect imageRect(0, 0, img.width(), img.height());
    const QRect r = rect & imageRect & imageRect.translated(-offset);
    const QPoint p = rect.topLeft() + offset;

    if (r.isEmpty())
        return;

    const uchar *src;
    uchar *dest;

    if (r.top() < p.y()) {
        src = mem + r.bottom() * lineskip + r.left() * depth;
        dest = mem + (p.y() + r.height() - 1) * lineskip + p.x() * depth;
        lineskip = -lineskip;
    } else {
        src = mem + r.top() * lineskip + r.left() * depth;
        dest = mem + p.y() * lineskip + p.x() * depth;
    }

    const int w = r.width();
    int h = r.height();
    const int bytes = w * depth;

    // overlapping segments?
    if (offset.y() == 0 && qAbs(offset.x()) < w) {
        do {
            ::memmove(dest, src, bytes);
            dest += lineskip;
            src += lineskip;
        } while (--h);
    } else {
        do {
            ::memcpy(dest, src, bytes);
            dest += lineskip;
            src += lineskip;
        } while (--h);
    }
}

QT_END_NAMESPACE

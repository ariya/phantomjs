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

#include "qwindowsurface_qws_p.h"
#include <qwidget.h>
#include <qscreen_qws.h>
#include <qwsmanager_qws.h>
#include <qapplication.h>
#include <qwsdisplay_qws.h>
#include <qrgb.h>
#include <qpaintengine.h>
#include <qdesktopwidget.h>
#include <private/qapplication_p.h>
#include <private/qwsdisplay_qws_p.h>
#include <private/qwidget_p.h>
#include <private/qwsmanager_p.h>
#include <private/qwslock_p.h>
#include <private/qbackingstore_p.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

#ifdef Q_BACKINGSTORE_SUBSURFACES

typedef QMap<int, QWSWindowSurface*> SurfaceMap;
Q_GLOBAL_STATIC(SurfaceMap, winIdToSurfaceMap);

QWSWindowSurface* qt_findWindowSurface(int winId)
{
    return winIdToSurfaceMap()->value(winId);
}

static void qt_insertWindowSurface(int winId, QWSWindowSurface *surface)
{
    if (!surface)
        winIdToSurfaceMap()->remove(winId);
    else
        winIdToSurfaceMap()->insert(winId, surface);
}

#endif // Q_BACKINGSTORE_SUBSURFACES

inline bool isWidgetOpaque(const QWidget *w)
{
    return w->d_func()->isOpaque && !w->testAttribute(Qt::WA_TranslucentBackground);
}

static inline QScreen *getScreen(const QWidget *w)
{
    const QList<QScreen*> subScreens = qt_screen->subScreens();
    if (subScreens.isEmpty())
        return qt_screen;

    const int screen = QApplication::desktop()->screenNumber(w);

    return qt_screen->subScreens().at(screen < 0 ? 0 : screen);
}

static int bytesPerPixel(QImage::Format format)
{
    switch (format) {
    case QImage::Format_Invalid:
        return 0;
#ifndef QT_NO_DEBUG
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        qFatal("QWSWindowSurface: Invalid backingstore format: %i",
               int(format));
#endif
    case QImage::Format_Indexed8:
        return 1;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        return 4;
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
        return 2;
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
        return 3;
    default:
#ifndef QT_NO_DEBUG
        qFatal("QWSWindowSurface: Invalid backingstore format: %i",
               int(format));
#endif
        return 0;
    }
}

static inline int nextMulOf4(int n)
{
    return ((n + 3) & 0xfffffffc);
}

static inline void setImageMetrics(QImage &img, QWidget *window) {
    QScreen *myScreen = getScreen(window);
    if (myScreen) {
        int dpmx = myScreen->width()*1000 / myScreen->physicalWidth();
        int dpmy = myScreen->height()*1000 / myScreen->physicalHeight();
        img.setDotsPerMeterX(dpmx);
        img.setDotsPerMeterY(dpmy);
    }
}

void QWSWindowSurface::invalidateBuffer()
{

    QWidget *win = window();
    if (win) {
        win->d_func()->invalidateBuffer(win->rect());
#ifndef QT_NO_QWS_MANAGER
        QTLWExtra *topextra = win->d_func()->extra->topextra;
        QWSManager *manager = topextra->qwsManager;
        if (manager)
            manager->d_func()->dirtyRegion(QDecoration::All,
                                           QDecoration::Normal);
#endif
    }
}

QWSWindowSurfacePrivate::QWSWindowSurfacePrivate()
    : flags(0),
#ifdef QT_QWS_CLIENTBLIT
    directId(-1),
#endif
    winId(0)
{
}

void QWSWindowSurfacePrivate::setWinId(int id)
{
    winId = id;
}

int QWSWindowSurface::winId() const
{
    // XXX: the widget winId may change during the lifetime of the widget!!!

    const QWidget *win = window();
    if (win && win->isWindow())
        return win->internalWinId();

#ifdef Q_BACKINGSTORE_SUBSURFACES
    if (!d_ptr->winId) {
        QWSWindowSurface *that = const_cast<QWSWindowSurface*>(this);
        QWSDisplay *display = QWSDisplay::instance();
        const int id = display->takeId();
        qt_insertWindowSurface(id, that);
        that->d_ptr->winId = id;

        if (win)
            display->nameRegion(id, win->objectName(), win->windowTitle());
        else
            display->nameRegion(id, QString(), QString());

        display->setAltitude(id, 1, true); // XXX
    }
#endif

    return d_ptr->winId;
}

void QWSWindowSurface::setWinId(int id)
{
    d_ptr->winId = id;
}

/*!
    \class QWSWindowSurface
    \since 4.2
    \ingroup qws
    \preliminary
    \internal

    \brief The QWSWindowSurface class provides the drawing area for top-level
    windows in Qt for Embedded Linux.

    Note that this class is only available in Qt for Embedded Linux.

    In \l{Qt for Embedded Linux}, the default behavior is for each client to
    render its widgets into memory while the server is responsible for
    putting the contents of the memory onto the
    screen. QWSWindowSurface is used by the window system to implement
    the associated memory allocation.

    When a screen update is required, the server runs through all the
    top-level windows that intersect with the region that is about to
    be updated, and ensures that the associated clients have updated
    their memory buffer. Then the server uses the screen driver to
    copy the content of the memory to the screen. To locate the
    relevant parts of memory, the driver is provided with the list of
    top-level windows that intersect with the given region. Associated
    with each of the top-level windows there is a window surface
    representing the drawing area of the window.

    When deriving from the QWSWindowSurface class, e.g., when adding
    an \l {Adding an Accelerated Graphics Driver to Qt for Embedded Linux}
    {accelerated graphics driver}, there are several pure virtual
    functions that must be implemented. In addition, QWSWindowSurface
    provides several virtual functions that can be reimplemented to
    customize the drawing process.

    \tableofcontents

    \section1 Pure Virtual Functions

    There are in fact two window surface instances for each top-level
    window; one used by the application when drawing a window, and
    another used by the server application to perform window
    compositioning. Implement the attach() to create the server-side
    representation of the surface. The data() function must be
    implemented to provide the required data.

    Implement the key() function to uniquely identify the surface
    class, and the isValid() function to determine is a surface
    corresponds to a given widget.

    The geometry() function must be implemented to let the window
    system determine the area required by the window surface
    (QWSWindowSurface also provides a corresponding virtual
    setGeometry() function that is called whenever the area necessary
    for the top-level window to be drawn, changes). The image()
    function is called by the window system during window
    compositioning, and must be implemented to return an image of the
    top-level window.

    Finally, the paintDevice() function must be implemented to return
    the appropriate paint device, and the scroll() function must be
    implemented to scroll the given region of the surface the given
    number of pixels.

    \section1 Virtual Functions

    When painting onto the surface, the window system will always call
    the beginPaint() function before any painting operations are
    performed. Likewise the endPaint() function is automatically
    called when the painting is done. Reimplement the painterOffset()
    function to alter the offset that is applied when drawing.

    The window system uses the flush() function to put a given region
    of the widget onto the screen, and the release() function to
    deallocate the screen region corresponding to this window surface.

    \section1 Other Members

    QWSWindowSurface provides the window() function returning a
    pointer to the top-level window the surface is representing. The
    currently visible region of the associated widget can be retrieved
    and set using the clipRegion() and setClipRegion() functions,
    respectively.

    When the window system performs the window compositioning, it uses
    the SurfaceFlag enum describing the surface content. The currently
    set surface flags can be retrieved and altered using the
    surfaceFlags() and setSurfaceFlags() functions. In addition,
    QWSWindowSurface provides the isBuffered(), isOpaque() and
    isRegionReserved() convenience functions.

    \sa {Qt for Embedded Linux Architecture#Drawing on Screen}{Qt for
    Embedded Linux Architecture}
*/

/*!
    \enum QWSWindowSurface::SurfaceFlag

    This enum is used to describe the window surface's contents.  It
    is used by the screen driver to handle region allocation and
    composition.

    \value RegionReserved The surface contains a reserved area. Once
    allocated, a reserved area can not not be changed by the window
    system, i.e., no other widgets can be drawn on top of this.

    \value Buffered
    The surface is in a memory area which is not part of a framebuffer.
    (A top-level window with QWidget::windowOpacity() other than 1.0 must use
    a buffered surface in order to making blending with the background work.)

    \value Opaque
    The surface contains only opaque pixels.

    \sa surfaceFlags(), setSurfaceFlags()
*/

/*!
    \fn bool QWSWindowSurface::isValid() const
    \since 4.3

    Implement this function to return true if the surface is a valid
    surface for the given top-level \a window; otherwise return
    false.

    \sa window(), key()
*/

/*!
    \fn QString QWSWindowSurface::key() const

    Implement this function to return a string that uniquely
    identifies the class of this surface.

    \sa window(), isValid()
*/

/*!
    \fn QByteArray QWSWindowSurface::permanentState() const
    \since 4.3

    Implement this function to return the data required for creating a
    server-side representation of the surface.

    \sa attach()
*/

/*!
    \fn void QWSWindowSurface::setPermanentState(const QByteArray &data)
    \since 4.3

    Implement this function to attach a server-side surface instance
    to the corresponding client side instance using the given \a
    data. Return true if successful; otherwise return false.

    \sa data()
*/

/*!
    \fn const QImage QWSWindowSurface::image() const

    Implement this function to return an image of the top-level window.

    \sa geometry()
*/

/*!
    \fn bool QWSWindowSurface::isRegionReserved() const

    Returns true if the QWSWindowSurface::RegionReserved is set; otherwise
    returns false.

    \sa surfaceFlags()
*/

/*!
    \fn bool QWSWindowSurface::isBuffered() const

    Returns true if the QWSWindowSurface::Buffered is set; otherwise returns false.

    \sa surfaceFlags()
*/

/*!
    \fn bool QWSWindowSurface::isOpaque() const

    Returns true if the QWSWindowSurface::Opaque is set; otherwise
    returns false.

    \sa surfaceFlags()
*/


/*!
    Constructs an empty surface.
*/
QWSWindowSurface::QWSWindowSurface()
    : QWindowSurface(0), d_ptr(new QWSWindowSurfacePrivate)
{
}

/*!
    Constructs an empty surface for the given top-level \a widget.
*/
QWSWindowSurface::QWSWindowSurface(QWidget *widget)
    : QWindowSurface(widget), d_ptr(new QWSWindowSurfacePrivate)
{
}

QWSWindowSurface::~QWSWindowSurface()
{
#ifdef Q_BACKINGSTORE_SUBSURFACES
    if (d_ptr->winId)
        winIdToSurfaceMap()->remove(d_ptr->winId);
#endif

    delete d_ptr;
}

/*!
    Returns the offset to be used when painting.

    \sa paintDevice()
*/
QPoint QWSWindowSurface::painterOffset() const
{
    const QWidget *w = window();
    if (!w)
        return QPoint();
    return w->geometry().topLeft() - w->frameGeometry().topLeft();
}

void QWSWindowSurface::beginPaint(const QRegion &)
{
    lock();
}

void QWSWindowSurface::endPaint(const QRegion &)
{
    unlock();
}

// XXX: documentation!!!
QByteArray QWSWindowSurface::transientState() const
{
    return QByteArray();
}

QByteArray QWSWindowSurface::permanentState() const
{
    return QByteArray();
}

void QWSWindowSurface::setTransientState(const QByteArray &state)
{
    Q_UNUSED(state);
}

void QWSWindowSurface::setPermanentState(const QByteArray &state)
{
    Q_UNUSED(state);
}

bool QWSWindowSurface::lock(int timeout)
{
    Q_UNUSED(timeout);
    return true;
}

void QWSWindowSurface::unlock()
{
}

#ifdef QT_QWS_CLIENTBLIT
/*! \internal */
const QRegion QWSWindowSurface::directRegion() const
{
    return d_ptr->direct;
}

/*! \internal */
int QWSWindowSurface::directRegionId() const
{
    return d_ptr->directId;
}

/*! \internal */
void QWSWindowSurface::setDirectRegion(const QRegion &r, int id)
{
    d_ptr->direct = r;
    d_ptr->directId = id;
}
#endif

/*!
    Returns the region currently visible on the screen.

    \sa setClipRegion()
*/
const QRegion QWSWindowSurface::clipRegion() const
{
    return d_ptr->clip;
}

/*!
    Sets the region currently visible on the screen to be the given \a
    clip region.

    \sa clipRegion()
*/
void QWSWindowSurface::setClipRegion(const QRegion &clip)
{
    if (clip == d_ptr->clip)
        return;

    QRegion expose = (clip - d_ptr->clip);
    d_ptr->clip = clip;

    if (expose.isEmpty() || clip.isEmpty())
        return; // No repaint or flush required.

    QWidget *win = window();
    if (!win)
        return;

    if (isBuffered()) {
        // No repaint required. Flush exposed area via the backing store.
        win->d_func()->syncBackingStore(expose);
        return;
    }

#ifndef QT_NO_QWS_MANAGER
    // Invalidate exposed decoration area.
    if (win && win->isWindow()) {
        QTLWExtra *topextra = win->d_func()->extra->topextra;
        if (QWSManager *manager = topextra->qwsManager) {
            QRegion decorationExpose(manager->region());
            decorationExpose.translate(-win->geometry().topLeft());
            decorationExpose &= expose;
            if (!decorationExpose.isEmpty()) {
                expose -= decorationExpose;
                manager->d_func()->dirtyRegion(QDecoration::All, QDecoration::Normal, decorationExpose);
            }
        }
    }
#endif

    // Invalidate exposed widget area.
    win->d_func()->invalidateBuffer(expose);
}

/*!
    Returns the surface flags describing the contents of this surface.

    \sa isBuffered(), isOpaque(), isRegionReserved()
*/
QWSWindowSurface::SurfaceFlags QWSWindowSurface::surfaceFlags() const
{
    return d_ptr->flags;
}

/*!
    Sets the surface flags describing the contents of this surface, to
    be the given \a flags.

    \sa surfaceFlags()
*/
void QWSWindowSurface::setSurfaceFlags(SurfaceFlags flags)
{
    d_ptr->flags = flags;
}

void QWSWindowSurface::setGeometry(const QRect &rect)
{
    QRegion mask = rect;

    const QWidget *win = window();
    if (win) {
#ifndef QT_NO_QWS_MANAGER
        if (win->isWindow()) {
            QTLWExtra *topextra = win->d_func()->extra->topextra;
            QWSManager *manager = topextra->qwsManager;

            if (manager) {
                // The frame geometry is the bounding rect of manager->region,
                // which could be too much, so we need to clip.
                mask &= (manager->region() + win->geometry());
            }
        }
#endif

        const QRegion winMask = win->mask();
        if (!winMask.isEmpty())
            mask &= winMask.translated(win->geometry().topLeft());
    }

    setGeometry(rect, mask);
}

void QWSWindowSurface::setGeometry(const QRect &rect, const QRegion &mask)
{
    if (rect == geometry()) // XXX: && mask == prevMask
        return;

    const bool isResize = rect.size() != geometry().size();
    const bool needsRepaint = isResize || !isBuffered();

    QWindowSurface::setGeometry(rect);

    const QRegion region = mask & rect;
    QWidget *win = window();
    // Only request regions for widgets visible on the screen.
    // (Added the !win check for compatibility reasons, because
    // there was no "if (win)" check before).
    const bool requestQWSRegion = !win || !win->testAttribute(Qt::WA_DontShowOnScreen);
    if (requestQWSRegion)
        QWidget::qwsDisplay()->requestRegion(winId(), key(), permanentState(), region);

    if (needsRepaint)
        invalidateBuffer();

    if (!requestQWSRegion) {
        // We didn't request a region, hence we won't get a QWSRegionEvent::Allocation
        // event back from the server so we set the clip directly. We have to
        // do this after the invalidateBuffer() call above, as it might trigger a
        // backing store sync, resulting in too many update requests.
        setClipRegion(region);
    }
}

static inline void flushUpdate(QWidget *widget, const QRegion &region,
                               const QPoint &offset)
{
#ifdef QT_NO_PAINT_DEBUG
    Q_UNUSED(widget);
    Q_UNUSED(region);
    Q_UNUSED(offset);
#else
    static int delay = -1;

    if (delay == -1)
        delay = qgetenv("QT_FLUSH_UPDATE").toInt() * 10;

    if (delay == 0)
        return;

    static QWSYellowSurface surface(true);
    surface.setDelay(delay);
    surface.flush(widget, region, offset);
#endif // QT_NO_PAINT_DEBUG
}

void QWSWindowSurface::flush(QWidget *widget, const QRegion &region,
                             const QPoint &offset)
{
    const QWidget *win = window();
    if (!win)
        return;

#ifndef QT_NO_GRAPHICSVIEW
    QWExtra *extra = win->d_func()->extra;
    if (extra && extra->proxyWidget)
        return;
#endif //QT_NO_GRAPHICSVIEW

    Q_UNUSED(offset);

    const bool opaque = isOpaque();
#ifdef QT_QWS_DISABLE_FLUSHCLIPPING
    QRegion toFlush = region;
#else
    QRegion toFlush = region & d_ptr->clip;
#endif

    if (!toFlush.isEmpty()) {
        flushUpdate(widget, toFlush, QPoint(0, 0));
        QPoint globalZero = win->mapToGlobal(QPoint(0, 0));
        toFlush.translate(globalZero);

#ifdef QT_QWS_CLIENTBLIT
        bool needRepaint = true;
        if (opaque) {
            QScreen* widgetScreen = getScreen(widget);
            if (widgetScreen->supportsBlitInClients()) {

                QWSDisplay::grab();
                if(directRegion().intersected(toFlush) == toFlush) {
                    QPoint translate = -globalZero + painterOffset() + geometry().topLeft();
                    QRegion flushRegion = toFlush.translated(translate);
                    widgetScreen->blit(image(), geometry().topLeft(), flushRegion);
                    widgetScreen->setDirty(toFlush.boundingRect());
                    needRepaint = false;
                }
                QWSDisplay::ungrab();
            }
        }

        if(needRepaint)
#endif
            win->qwsDisplay()->repaintRegion(winId(), win->windowFlags(), opaque, toFlush);
    }
}

/*!
    Move the surface with the given \a offset.

    A subclass may reimplement this function to enable accelerated window move.
    It must return true if the move was successful and no repaint is necessary,
    false otherwise.

    The default implementation updates the QWindowSurface geometry and
    returns true if the surface is buffered; false otherwise.

    This function is called by the window system on the client instance.

    \sa isBuffered()
*/
bool QWSWindowSurface::move(const QPoint &offset)
{
    QWindowSurface::setGeometry(geometry().translated(offset));
    return isBuffered();
}

/*!
    Move the surface with the given \a offset.

    The new visible region after the window move is given by \a newClip
    in screen coordinates.

    A subclass may reimplement this function to enable accelerated window move.
    The returned region indicates the area that still needs to be composed
    on the screen.

    The default implementation updates the QWindowSurface geometry and
    returns the union of the old and new geometry.

    This function is called by the window system on the server instance.
*/
QRegion QWSWindowSurface::move(const QPoint &offset, const QRegion &newClip)
{
    const QRegion oldGeometry = geometry();
    QWindowSurface::setGeometry(geometry().translated(offset));
    return oldGeometry + newClip;
}

void QWSWindowSurface::releaseSurface()
{
}

bool QWSMemorySurface::lock(int timeout)
{
    Q_UNUSED(timeout);
#ifndef QT_NO_QWS_MULTIPROCESS
    if (memlock && !memlock->lock(QWSLock::BackingStore))
        return false;
#endif
#ifndef QT_NO_THREAD
    threadLock.lock();
#endif
    return true;
}

void QWSMemorySurface::unlock()
{
#ifndef QT_NO_THREAD
    threadLock.unlock();
#endif
#ifndef QT_NO_QWS_MULTIPROCESS
    if (memlock)
        memlock->unlock(QWSLock::BackingStore);
#endif
}

QWSMemorySurface::QWSMemorySurface()
    : QWSWindowSurface()
#ifndef QT_NO_QWS_MULTIPROCESS
    , memlock(0)
#endif
{
    setSurfaceFlags(Buffered);
}

QWSMemorySurface::QWSMemorySurface(QWidget *w)
    : QWSWindowSurface(w)
{
    SurfaceFlags flags = Buffered;
    if (isWidgetOpaque(w))
        flags |= Opaque;
    setSurfaceFlags(flags);

#ifndef QT_NO_QWS_MULTIPROCESS
    memlock = QWSDisplay::Data::getClientLock();
#endif
}

QWSMemorySurface::~QWSMemorySurface()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (memlock != QWSDisplay::Data::getClientLock())
        delete memlock;
#endif
}


QImage::Format
QWSMemorySurface::preferredImageFormat(const QWidget *widget) const
{
    QScreen *screen = getScreen(widget);
    const int depth = screen->depth();
    const bool opaque = isWidgetOpaque(widget);

    if (!opaque) {
        if (depth <= 12)
            return QImage::Format_ARGB4444_Premultiplied;
        else if (depth <= 15)
            return QImage::Format_ARGB8555_Premultiplied;
        else if (depth <= 16)
            return QImage::Format_ARGB8565_Premultiplied;
        else if (depth <= 18)
            return QImage::Format_ARGB6666_Premultiplied;
        else
            return QImage::Format_ARGB32_Premultiplied;
    }

    QImage::Format format = screen->pixelFormat();
    if (format > QImage::Format_Indexed8) // ### assumes all new image formats supports a QPainter
        return format;

    if (depth <= 12)
        return QImage::Format_RGB444;
    else if (depth <= 15)
        return QImage::Format_RGB555;
    else if (depth <= 16)
        return QImage::Format_RGB16;
    else if (depth <= 18)
        return QImage::Format_RGB666;
    else if (depth <= 24)
        return QImage::Format_RGB888;
    else
        return QImage::Format_ARGB32_Premultiplied;
}

#ifndef QT_NO_QWS_MULTIPROCESS
void QWSMemorySurface::setLock(int lockId)
{
    if (memlock && memlock->id() == lockId)
        return;
    if (memlock != QWSDisplay::Data::getClientLock())
        delete memlock;
    memlock = (lockId == -1 ? 0 : new QWSLock(lockId));
}
#endif // QT_NO_QWS_MULTIPROCESS

bool QWSMemorySurface::isValid() const
{
    if (img.isNull())
        return true;

    const QWidget *win = window();
    if (preferredImageFormat(win) != img.format())
        return false;

    if (isOpaque() != isWidgetOpaque(win)) // XXX: use QWidgetPrivate::isOpaque()
        return false;

    return true;
}

// ### copied from qwindowsurface_raster.cpp -- should be cross-platform
void QWSMemorySurface::beginPaint(const QRegion &rgn)
{
    if (!isWidgetOpaque(window())) {
        QPainter p(&img);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QVector<QRect> rects = rgn.rects();
        const QColor blank = Qt::transparent;
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it) {
            QRect r = *it;
#ifdef Q_BACKINGSTORE_SUBSURFACES
            r.translate(painterOffset());
#endif
            p.fillRect(r, blank);
        }
    }
    QWSWindowSurface::beginPaint(rgn);
}

// from qwindowsurface.cpp
extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QWSMemorySurface::scroll(const QRegion &area, int dx, int dy)
{
    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(img, rects.at(i), QPoint(dx, dy));

    return true;
}

QPoint QWSMemorySurface::painterOffset() const
{
    const QWidget *w = window();
    if (!w)
        return QPoint();

    if (w->mask().isEmpty())
        return QWSWindowSurface::painterOffset();

    const QRegion region = w->mask()
                           & w->frameGeometry().translated(-w->geometry().topLeft());
    return -region.boundingRect().topLeft();
}

QWSLocalMemSurface::QWSLocalMemSurface()
    : QWSMemorySurface(), mem(0), memsize(0)
{
}

QWSLocalMemSurface::QWSLocalMemSurface(QWidget *w)
    : QWSMemorySurface(w), mem(0), memsize(0)
{
}

QWSLocalMemSurface::~QWSLocalMemSurface()
{
    if (memsize)
        delete[] mem;
}

void QWSLocalMemSurface::setGeometry(const QRect &rect)
{
    QSize size = rect.size();

    QWidget *win = window();
    if (win && !win->mask().isEmpty()) {
        const QRegion region = win->mask()
                               & rect.translated(-win->geometry().topLeft());
        size = region.boundingRect().size();
    }

    uchar *deleteLater = 0;

    if (img.size() != size) {
        if (size.isEmpty()) {
            if (memsize) {
                // In case of a Hide event we need to delete the memory after sending the
                // event to the server in order to let the server animate the event.
                deleteLater = mem;
                memsize = 0;
            }
            mem = 0;
            img = QImage();
        } else {
            const QImage::Format format = preferredImageFormat(win);
            const int bpl = nextMulOf4(bytesPerPixel(format) * size.width());
            const int imagesize = bpl * size.height();
            if (memsize < imagesize) {
                delete[] mem;
                memsize = imagesize;
                mem = new uchar[memsize];
            }
            img = QImage(mem, size.width(), size.height(), bpl, format);
            setImageMetrics(img, win);
        }
    }

    QWSWindowSurface::setGeometry(rect);

    delete[] deleteLater;
}

QByteArray QWSLocalMemSurface::permanentState() const
{
    QByteArray array(sizeof(uchar*) + 3 * sizeof(int) + sizeof(SurfaceFlags), Qt::Uninitialized);

    char *ptr = array.data();

    *reinterpret_cast<uchar**>(ptr) = mem;
    ptr += sizeof(uchar*);

    reinterpret_cast<int*>(ptr)[0] = img.width();
    reinterpret_cast<int*>(ptr)[1] = img.height();
    ptr += 2 * sizeof(int);

    *reinterpret_cast<int *>(ptr) = img.format();
    ptr += sizeof(int);

    *reinterpret_cast<SurfaceFlags*>(ptr) = surfaceFlags();

    return array;
}

void QWSLocalMemSurface::setPermanentState(const QByteArray &data)
{
    if (memsize) {
        delete[] mem;
        memsize = 0;
    }

    int width;
    int height;
    QImage::Format format;
    SurfaceFlags flags;

    const char *ptr = data.constData();

    mem = *reinterpret_cast<uchar* const*>(ptr);
    ptr += sizeof(uchar*);

    width = reinterpret_cast<const int*>(ptr)[0];
    height = reinterpret_cast<const int*>(ptr)[1];
    ptr += 2 * sizeof(int);

    format = QImage::Format(*reinterpret_cast<const int*>(ptr));
    ptr += sizeof(int);

    flags = *reinterpret_cast<const SurfaceFlags*>(ptr);

    const int bpl = nextMulOf4(bytesPerPixel(format) * width);
    QWSMemorySurface::img = QImage(mem, width, height, bpl, format);
    setSurfaceFlags(flags);
}

void QWSLocalMemSurface::releaseSurface()
{
    if (memsize) {
        delete[] mem;
        memsize = 0;
    }
    mem = 0;
    img = QImage();
}

#ifndef QT_NO_QWS_MULTIPROCESS

QWSSharedMemSurface::QWSSharedMemSurface()
    : QWSMemorySurface()
{
}

QWSSharedMemSurface::QWSSharedMemSurface(QWidget *widget)
    : QWSMemorySurface(widget)
{
}

QWSSharedMemSurface::~QWSSharedMemSurface()
{
    // mem.detach() is done automatically by ~QSharedMemory
}

bool QWSSharedMemSurface::setMemory(int memId)
{
    if (mem.id() == memId)
        return true;

    mem.detach();

    if (memId != -1 && !mem.attach(memId)) {
#ifndef QT_NO_DEBUG
        perror("QWSSharedMemSurface: attaching to shared memory");
        qCritical("QWSSharedMemSurface: Error attaching to shared memory 0x%x", memId);
#endif
        return false;
    }

    return true;
}

#ifdef QT_QWS_CLIENTBLIT
void QWSSharedMemSurface::setDirectRegion(const QRegion &r, int id)
{
    QWSMemorySurface::setDirectRegion(r, id);
    if (mem.address())
        *(uint *)mem.address() = id;
}

const QRegion QWSSharedMemSurface::directRegion() const
{
    if (mem.address() && *(uint *)mem.address() == uint(directRegionId()))
        return QWSMemorySurface::directRegion();
    return QRegion();
}
#endif

void QWSSharedMemSurface::setPermanentState(const QByteArray &data)
{
    int memId;
    int width;
    int height;
    int lockId;
    QImage::Format format;
    SurfaceFlags flags;

    const int *ptr = reinterpret_cast<const int*>(data.constData());

    memId = ptr[0];
    width = ptr[1];
    height = ptr[2];
    lockId = ptr[3];
    format = QImage::Format(ptr[4]);
    flags = SurfaceFlags(ptr[5]);

    setSurfaceFlags(flags);
    setMemory(memId);
    setLock(lockId);

#ifdef QT_QWS_CLIENTBLIT
    uchar *base = static_cast<uchar*>(mem.address()) + sizeof(uint);
#else
    uchar *base = static_cast<uchar*>(mem.address());
#endif
    const int bpl = nextMulOf4(bytesPerPixel(format) * width);
    QWSMemorySurface::img = QImage(base, width, height, bpl, format);
}

void QWSSharedMemSurface::setGeometry(const QRect &rect)
{
    const QSize size = rect.size();
    if (img.size() != size) {
        if (size.isEmpty()) {
            mem.detach();
            img = QImage();
        } else {
            QWidget *win = window();
            const QImage::Format format = preferredImageFormat(win);
            const int bpl = nextMulOf4(bytesPerPixel(format) * size.width());
#ifdef QT_QWS_CLIENTBLIT
            const int imagesize = bpl * size.height() + sizeof(uint);
#else
            const int imagesize = bpl * size.height();
#endif
            if (mem.size() < imagesize) {
                mem.detach();
                if (!mem.create(imagesize)) {
                    perror("QWSSharedMemSurface::setGeometry allocating shared memory");
                    qFatal("Error creating shared memory of size %d", imagesize);
                }
            }
#ifdef QT_QWS_CLIENTBLIT
            *((uint *)mem.address()) = 0;
            uchar *base = static_cast<uchar*>(mem.address()) + sizeof(uint);
#else
            uchar *base = static_cast<uchar*>(mem.address());
#endif
            img = QImage(base, size.width(), size.height(), bpl, format);
            setImageMetrics(img, win);
        }
    }

    QWSWindowSurface::setGeometry(rect);
}

QByteArray QWSSharedMemSurface::permanentState() const
{
    QByteArray array(6 * sizeof(int), Qt::Uninitialized);

    int *ptr = reinterpret_cast<int*>(array.data());

    ptr[0] = mem.id();
    ptr[1] = img.width();
    ptr[2] = img.height();
    ptr[3] = (memlock ? memlock->id() : -1);
    ptr[4] = int(img.format());
    ptr[5] = int(surfaceFlags());

    return array;
}

void QWSSharedMemSurface::releaseSurface()
{
    mem.detach();
    img = QImage();
}

#endif // QT_NO_QWS_MULTIPROCESS

#ifndef QT_NO_PAINTONSCREEN

QWSOnScreenSurface::QWSOnScreenSurface(QWidget *w)
    : QWSMemorySurface(w)
{
    attachToScreen(getScreen(w));
    setSurfaceFlags(Opaque);
}

QWSOnScreenSurface::QWSOnScreenSurface()
    : QWSMemorySurface()
{
    setSurfaceFlags(Opaque);
}

void QWSOnScreenSurface::attachToScreen(const QScreen *s)
{
    screen = s;
    uchar *base = screen->base();
    QImage::Format format  = screen->pixelFormat();

    if (format == QImage::Format_Invalid || format == QImage::Format_Indexed8) {
        //### currently we have no paint engine for indexed image formats
        qFatal("QWSOnScreenSurface::attachToScreen(): screen depth %d "
               "not implemented", screen->depth());
        return;
    }
    QWSMemorySurface::img = QImage(base, screen->width(), screen->height(),
                                   screen->linestep(), format );
}

QWSOnScreenSurface::~QWSOnScreenSurface()
{
}

QPoint QWSOnScreenSurface::painterOffset() const
{
    return geometry().topLeft() + QWSWindowSurface::painterOffset();
}

bool QWSOnScreenSurface::isValid() const
{
    const QWidget *win = window();
    if (screen != getScreen(win))
        return false;
    if (img.isNull())
        return false;
    return QScreen::isWidgetPaintOnScreen(win);
}

QByteArray QWSOnScreenSurface::permanentState() const
{
    QByteArray array(sizeof(int), Qt::Uninitialized);

    int *ptr = reinterpret_cast<int*>(array.data());
    ptr[0] = QApplication::desktop()->screenNumber(window());
    return array;
}

void QWSOnScreenSurface::setPermanentState(const QByteArray &data)
{
    const int *ptr = reinterpret_cast<const int*>(data.constData());
    const int screenNo = ptr[0];

    QScreen *screen = qt_screen;
    if (screenNo > 0)
        screen = qt_screen->subScreens().at(screenNo);
    attachToScreen(screen);
}

#endif // QT_NO_PAINTONSCREEN

#ifndef QT_NO_PAINT_DEBUG

QWSYellowSurface::QWSYellowSurface(bool isClient)
    : QWSWindowSurface(), delay(10)
{
    if (isClient) {
        setWinId(QWidget::qwsDisplay()->takeId());
        QWidget::qwsDisplay()->nameRegion(winId(),
                                          QLatin1String("Debug flush paint"),
                                          QLatin1String("Silly yellow thing"));
        QWidget::qwsDisplay()->setAltitude(winId(), 1, true);
    }
    setSurfaceFlags(Buffered);
}

QWSYellowSurface::~QWSYellowSurface()
{
}

QByteArray QWSYellowSurface::permanentState() const
{
    QByteArray array(2 * sizeof(int), Qt::Uninitialized);

    int *ptr = reinterpret_cast<int*>(array.data());
    ptr[0] = surfaceSize.width();
    ptr[1] = surfaceSize.height();

    return array;
}

void QWSYellowSurface::setPermanentState(const QByteArray &data)
{
    const int *ptr = reinterpret_cast<const int*>(data.constData());

    const int width = ptr[0];
    const int height = ptr[1];

    img = QImage(width, height, QImage::Format_ARGB32);
    img.fill(qRgba(255,255,31,127));
}

void QWSYellowSurface::flush(QWidget *widget, const QRegion &region,
                             const QPoint &offset)
{
    Q_UNUSED(offset);

    QWSDisplay *display = QWidget::qwsDisplay();
    QRegion rgn = region;

    if (widget)
        rgn.translate(widget->mapToGlobal(QPoint(0, 0)));

    surfaceSize = region.boundingRect().size();

    const int id = winId();
    display->requestRegion(id, key(), permanentState(), rgn);
    display->setAltitude(id, 1, true);
    display->repaintRegion(id, 0, false, rgn);

    ::usleep(500 * delay);
    display->requestRegion(id, key(), permanentState(), QRegion());
    ::usleep(500 * delay);
}

#endif // QT_NO_PAINT_DEBUG

#ifndef QT_NO_DIRECTPAINTER

static inline QScreen *getPrimaryScreen()
{
    QScreen *screen = QScreen::instance();
    if (!screen->base()) {
        QList<QScreen*> subScreens = screen->subScreens();
        if (subScreens.size() < 1)
            return 0;
        screen = subScreens.at(0);
    }
    return screen;
}

QWSDirectPainterSurface::QWSDirectPainterSurface(bool isClient,
                                                 QDirectPainter::SurfaceFlag flags)
    : QWSWindowSurface(), flushingRegionEvents(false), doLocking(false)
{
    setSurfaceFlags(Opaque);
    synchronous = (flags == QDirectPainter::ReservedSynchronous);

    if (isClient) {
        setWinId(QWidget::qwsDisplay()->takeId());
        QWidget::qwsDisplay()->nameRegion(winId(),
                                          QLatin1String("QDirectPainter reserved space"),
                                          QLatin1String("reserved"));
    } else {
        setWinId(0);
    }
    _screen = QScreen::instance();
    if (!_screen->base()) {
        QList<QScreen*> subScreens = _screen->subScreens();
        if (subScreens.size() < 1)
            _screen = 0;
        else
            _screen = subScreens.at(0);
    }
}

QWSDirectPainterSurface::~QWSDirectPainterSurface()
{
    if (winId() && QWSDisplay::instance()) // make sure not in QApplication destructor
        QWidget::qwsDisplay()->destroyRegion(winId());
}

void QWSDirectPainterSurface::setRegion(const QRegion &region)
{
    const int id = winId();
    QWidget::qwsDisplay()->requestRegion(id, key(), permanentState(), region);
#ifndef QT_NO_QWS_MULTIPROCESS
    if (synchronous)
        QWSDisplay::instance()->d->waitForRegionAck(id);
#endif
}

void QWSDirectPainterSurface::flush(QWidget *, const QRegion &r, const QPoint &)
{
    QWSDisplay::instance()->repaintRegion(winId(), 0, true, r);
}

QByteArray QWSDirectPainterSurface::permanentState() const
{
    QByteArray res;
    if (isRegionReserved())
        res.append( 'r');
    return res;
}

void QWSDirectPainterSurface::setPermanentState(const QByteArray &ba)
{
    if (ba.size() > 0 && ba.at(0) == 'r')
        setReserved();
    setSurfaceFlags(surfaceFlags() | Opaque);
}

void QWSDirectPainterSurface::beginPaint(const QRegion &region)
{
    QWSWindowSurface::beginPaint(region);
#ifndef QT_NO_QWS_MULTIPROCESS
    if (!synchronous) {
        flushingRegionEvents = true;
        QWSDisplay::instance()->d->waitForRegionEvents(winId(), doLocking);
        flushingRegionEvents = false;
    }
#endif
}

bool QWSDirectPainterSurface::hasPendingRegionEvents() const
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (synchronous)
        return false;

    return QWSDisplay::instance()->d->hasPendingRegionEvents();
#else
    return false;
#endif
}

bool QWSDirectPainterSurface::lock(int timeout)
{
#ifndef QT_NO_THREAD
    threadLock.lock();
#endif
    Q_UNUSED(timeout);
    if (doLocking)
        QWSDisplay::grab(true);
    return true;
}

void QWSDirectPainterSurface::unlock()
{
    if (doLocking)
        QWSDisplay::ungrab();
#ifndef QT_NO_THREAD
    threadLock.unlock();
#endif
}

#endif // QT_NO_DIRECTPAINTER

QT_END_NAMESPACE

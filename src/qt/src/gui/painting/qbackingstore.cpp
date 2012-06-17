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


#include "qplatformdefs.h"

#include "qbackingstore_p.h"

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qevent.h>
#include <QtGui/qapplication.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/qgraphicsproxywidget.h>

#include <private/qwidget_p.h>
#include <private/qwindowsurface_raster_p.h>
#include <private/qapplication_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qgraphicseffect_p.h>

#include "qgraphicssystem_p.h"

#ifdef Q_WS_QWS
#include <QtGui/qwsmanager_qws.h>
#include <private/qwsmanager_p.h>
#endif

QT_BEGIN_NAMESPACE

extern QRegion qt_dirtyRegion(QWidget *);

/*
   A version of QRect::intersects() that does not normalize the rects.
*/
static inline bool qRectIntersects(const QRect &r1, const QRect &r2)
{
    return (qMax(r1.left(), r2.left()) <= qMin(r1.right(), r2.right())
            && qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));
}

/**
 * Flushes the contents of the \a windowSurface into the screen area of \a widget.
 * \a tlwOffset is the position of the top level widget relative to the window surface.
 * \a region is the region to be updated in \a widget coordinates.
 */
static inline void qt_flush(QWidget *widget, const QRegion &region, QWindowSurface *windowSurface,
                            QWidget *tlw, const QPoint &tlwOffset)
{
    Q_ASSERT(widget);
    Q_ASSERT(!region.isEmpty());
    Q_ASSERT(windowSurface);
    Q_ASSERT(tlw);

#if !defined(QT_NO_PAINT_DEBUG) && !defined(Q_WS_QWS)
    // QWS does flush update in QWindowSurface::flush (because it needs to lock the surface etc).
    static int flushUpdate = qgetenv("QT_FLUSH_UPDATE").toInt();
    if (flushUpdate > 0)
        QWidgetBackingStore::showYellowThing(widget, region, flushUpdate * 10, false);
#endif

    //The performance hit by doing this should be negligible. However, be aware that
    //using this FPS when you have > 1 windowsurface can give you inaccurate FPS
    static bool fpsDebug = qgetenv("QT_DEBUG_FPS").toInt();
    if (fpsDebug) {
        static QTime time = QTime::currentTime();
        static int frames = 0;

        frames++;

        if(time.elapsed() > 5000) {
            double fps = double(frames * 1000) /time.restart();
            fprintf(stderr,"FPS: %.1f\n",fps);
            frames = 0;
        }
    }
    if (widget != tlw)
        windowSurface->flush(widget, region, tlwOffset + widget->mapTo(tlw, QPoint()));
    else
        windowSurface->flush(widget, region, tlwOffset);
}

#ifndef QT_NO_PAINT_DEBUG
#ifdef Q_WS_WIN
static void showYellowThing_win(QWidget *widget, const QRegion &region, int msec)
{
    HBRUSH brush;
    static int i = 0;
    switch (i) {
    case 0:
        brush = CreateSolidBrush(RGB(255, 255, 0));
        break;
    case 1:
        brush = CreateSolidBrush(RGB(255, 200, 55));
        break;
    case 2:
        brush = CreateSolidBrush(RGB(200, 255, 55));
        break;
    case 3:
        brush = CreateSolidBrush(RGB(200, 200, 0));
        break;
    }
    i = (i + 1) & 3;

    HDC hdc = widget->getDC();

    const QVector<QRect> &rects = region.rects();
    foreach (QRect rect, rects) {
        RECT winRect;
        SetRect(&winRect, rect.left(), rect.top(), rect.right(), rect.bottom());
        FillRect(hdc, &winRect, brush);
    }

    widget->releaseDC(hdc);
    ::Sleep(msec);
}
#endif

void QWidgetBackingStore::showYellowThing(QWidget *widget, const QRegion &toBePainted, int msec, bool unclipped)
{
#ifdef Q_WS_QWS
    Q_UNUSED(widget);
    Q_UNUSED(unclipped);
    static QWSYellowSurface surface(true);
    surface.setDelay(msec);
    surface.flush(widget, toBePainted, QPoint());
#else
    QRegion paintRegion = toBePainted;
    QRect widgetRect = widget->rect();

    if (!widget->internalWinId()) {
        QWidget *nativeParent = widget->nativeParentWidget();
        const QPoint offset = widget->mapTo(nativeParent, QPoint(0, 0));
        paintRegion.translate(offset);
        widgetRect.translate(offset);
        widget = nativeParent;
    }

#ifdef Q_WS_WIN
    Q_UNUSED(unclipped);
    showYellowThing_win(widget, paintRegion, msec);
#else
    //flags to fool painter
    bool paintUnclipped = widget->testAttribute(Qt::WA_PaintUnclipped);
    if (unclipped && !widget->d_func()->paintOnScreen())
        widget->setAttribute(Qt::WA_PaintUnclipped);

    const bool setFlag = !widget->testAttribute(Qt::WA_WState_InPaintEvent);
    if (setFlag)
        widget->setAttribute(Qt::WA_WState_InPaintEvent);

    //setup the engine
    QPaintEngine *pe = widget->paintEngine();
    if (pe) {
        pe->setSystemClip(paintRegion);
        {
            QPainter p(widget);
            p.setClipRegion(paintRegion);
            static int i = 0;
            switch (i) {
            case 0:
                p.fillRect(widgetRect, QColor(255,255,0));
                break;
            case 1:
                p.fillRect(widgetRect, QColor(255,200,55));
                break;
            case 2:
                p.fillRect(widgetRect, QColor(200,255,55));
                break;
            case 3:
                p.fillRect(widgetRect, QColor(200,200,0));
                break;
            }
            i = (i+1) & 3;
            p.end();
        }
    }

    if (setFlag)
        widget->setAttribute(Qt::WA_WState_InPaintEvent, false);

    //restore
    widget->setAttribute(Qt::WA_PaintUnclipped, paintUnclipped);

    if (pe)
        pe->setSystemClip(QRegion());

    QApplication::syncX();

#if defined(Q_OS_UNIX)
    ::usleep(1000 * msec);
#endif
#endif // Q_WS_WIN
#endif // Q_WS_QWS
}

bool QWidgetBackingStore::flushPaint(QWidget *widget, const QRegion &rgn)
{
    if (!widget)
        return false;

    int delay = 0;
    if (widget->testAttribute(Qt::WA_WState_InPaintEvent)) {
        static int flushPaintEvent = qgetenv("QT_FLUSH_PAINT_EVENT").toInt();
        if (!flushPaintEvent)
            return false;
        delay = flushPaintEvent;
    } else {
        static int flushPaint = qgetenv("QT_FLUSH_PAINT").toInt();
        if (!flushPaint)
            return false;
        delay = flushPaint;
    }

    QWidgetBackingStore::showYellowThing(widget, rgn, delay * 10, true);
    return true;
}

void QWidgetBackingStore::unflushPaint(QWidget *widget, const QRegion &rgn)
{
    if (widget->d_func()->paintOnScreen() || rgn.isEmpty())
        return;

    QWidget *tlw = widget->window();
    QTLWExtra *tlwExtra = tlw->d_func()->maybeTopData();
    if (!tlwExtra)
        return;

    const QPoint offset = widget->mapTo(tlw, QPoint());
    qt_flush(widget, rgn, tlwExtra->backingStore->windowSurface, tlw, offset);
}
#endif // QT_NO_PAINT_DEBUG

/*
    Moves the whole rect by (dx, dy) in widget's coordinate system.
    Doesn't generate any updates.
*/
bool QWidgetBackingStore::bltRect(const QRect &rect, int dx, int dy, QWidget *widget)
{
    const QPoint pos(tlwOffset + widget->mapTo(tlw, rect.topLeft()));
    const QRect tlwRect(QRect(pos, rect.size()));
    if (fullUpdatePending || dirty.intersects(tlwRect))
        return false; // We don't want to scroll junk.
    return windowSurface->scroll(tlwRect, dx, dy);
}

void QWidgetBackingStore::releaseBuffer()
{
    if (windowSurface)
#if defined(Q_WS_QPA)
        windowSurface->resize(QSize());
#else
        windowSurface->setGeometry(QRect());
#endif
#ifdef Q_BACKINGSTORE_SUBSURFACES
    for (int i = 0; i < subSurfaces.size(); ++i)
        subSurfaces.at(i)->setGeometry(QRect());
#endif
}

/*!
    Prepares the window surface to paint a\ toClean region of the \a widget and
    updates the BeginPaintInfo struct accordingly.

    The \a toClean region might be clipped by the window surface.
*/
void QWidgetBackingStore::beginPaint(QRegion &toClean, QWidget *widget, QWindowSurface *windowSurface,
                                     BeginPaintInfo *returnInfo, bool toCleanIsInTopLevelCoordinates)
{
#ifdef Q_WS_QWS
    QWSWindowSurface *surface = static_cast<QWSWindowSurface *>(windowSurface);
    QWidget *surfaceWidget = surface->window();

    if (!surface->isValid()) {
        // this looks strange but it really just releases the surface
        surface->releaseSurface();
        // the old window surface is deleted in setWindowSurface, which is
        // called from QWindowSurface constructor.
        windowSurface = tlw->d_func()->createDefaultWindowSurface();
        surface = static_cast<QWSWindowSurface *>(windowSurface);
        // createDefaultWindowSurface() will set topdata->windowSurface on the
        // widget to zero. However, if this is a sub-surface, it should point
        // to the widget's sub windowSurface, so we set that here:
        if (!surfaceWidget->isWindow())
            surfaceWidget->d_func()->topData()->windowSurface = windowSurface;
        surface->setGeometry(topLevelRect());
        returnInfo->windowSurfaceRecreated = true;
    }

    const QRegion toCleanUnclipped(toClean);

    if (surfaceWidget->isWindow())
        tlwOffset = surface->painterOffset();
#ifdef Q_BACKINGSTORE_SUBSURFACES
    else if (toCleanIsInTopLevelCoordinates)
        toClean &= surface->clipRegion().translated(surfaceWidget->mapTo(tlw, QPoint()));
    if (!toCleanIsInTopLevelCoordinates && windowSurface == this->windowSurface)
        toClean &= surface->clipRegion().translated(-widget->mapTo(surfaceWidget, QPoint()));
#else
    toClean &= surface->clipRegion();
#endif

    if (toClean.isEmpty()) {
        if (surfaceWidget->isWindow()) {
            dirtyFromPreviousSync += toCleanUnclipped;
            hasDirtyFromPreviousSync = true;
        }

        returnInfo->nothingToPaint = true;
        // Nothing to repaint. However, we might have newly exposed areas on the
        // screen, so we have to make sure those are flushed.
        flush();
        return;
    }

    if (surfaceWidget->isWindow()) {
        if (toCleanUnclipped != toClean) {
            dirtyFromPreviousSync += (toCleanUnclipped - surface->clipRegion());
            hasDirtyFromPreviousSync = true;
        }
        if (hasDirtyFromPreviousSync) {
            dirtyFromPreviousSync -= toClean;
            hasDirtyFromPreviousSync = !dirtyFromPreviousSync.isEmpty();
        }
    }

#endif // Q_WS_QWS

    Q_UNUSED(widget);
    Q_UNUSED(toCleanIsInTopLevelCoordinates);

    // Always flush repainted areas.
    dirtyOnScreen += toClean;

#if defined(Q_WS_QWS) && !defined(Q_BACKINGSTORE_SUBSURFACES)
    toClean.translate(tlwOffset);
#endif

#ifdef QT_NO_PAINT_DEBUG
    windowSurface->beginPaint(toClean);
#else
    returnInfo->wasFlushed = QWidgetBackingStore::flushPaint(tlw, toClean);
    // Avoid deadlock with QT_FLUSH_PAINT: the server will wait for
    // the BackingStore lock, so if we hold that, the server will
    // never release the Communication lock that we are waiting for in
    // sendSynchronousCommand
    if (!returnInfo->wasFlushed)
        windowSurface->beginPaint(toClean);
#endif

    Q_UNUSED(returnInfo);
}

void QWidgetBackingStore::endPaint(const QRegion &cleaned, QWindowSurface *windowSurface,
        BeginPaintInfo *beginPaintInfo)
{
#ifndef QT_NO_PAINT_DEBUG
    if (!beginPaintInfo->wasFlushed)
        windowSurface->endPaint(cleaned);
    else
        QWidgetBackingStore::unflushPaint(tlw, cleaned);
#else
    Q_UNUSED(beginPaintInfo);
    windowSurface->endPaint(cleaned);
#endif

#ifdef Q_BACKINGSTORE_SUBSURFACES
    flush(static_cast<QWSWindowSurface *>(windowSurface)->window(), windowSurface);
#else
    flush();
#endif
}

/*!
    Returns the region (in top-level coordinates) that needs repaint and/or flush.

    If the widget is non-zero, only the dirty region for the widget is returned
    and the region will be in widget coordinates.
*/
QRegion QWidgetBackingStore::dirtyRegion(QWidget *widget) const
{
    const bool widgetDirty = widget && widget != tlw;
    const QRect tlwRect(topLevelRect());
#if defined(Q_WS_QPA)
    const QRect surfaceGeometry(tlwRect.topLeft(), windowSurface->size());
#else
    const QRect surfaceGeometry(windowSurface->geometry());
#endif
    if (fullUpdatePending || (surfaceGeometry != tlwRect && surfaceGeometry.size() != tlwRect.size())) {
        if (widgetDirty) {
            const QRect dirtyTlwRect = QRect(QPoint(), tlwRect.size());
            const QPoint offset(widget->mapTo(tlw, QPoint()));
            const QRect dirtyWidgetRect(dirtyTlwRect & widget->rect().translated(offset));
            return dirtyWidgetRect.translated(-offset);
        }
        return QRect(QPoint(), tlwRect.size());
    }

    // Calculate the region that needs repaint.
    QRegion r(dirty);
    for (int i = 0; i < dirtyWidgets.size(); ++i) {
        QWidget *w = dirtyWidgets.at(i);
        if (widgetDirty && w != widget && !widget->isAncestorOf(w))
            continue;
        r += w->d_func()->dirty.translated(w->mapTo(tlw, QPoint()));
    }

    // Append the region that needs flush.
    r += dirtyOnScreen;

    if (dirtyOnScreenWidgets) { // Only in use with native child widgets.
        for (int i = 0; i < dirtyOnScreenWidgets->size(); ++i) {
            QWidget *w = dirtyOnScreenWidgets->at(i);
            if (widgetDirty && w != widget && !widget->isAncestorOf(w))
                continue;
            QWidgetPrivate *wd = w->d_func();
            Q_ASSERT(wd->needsFlush);
            r += wd->needsFlush->translated(w->mapTo(tlw, QPoint()));
        }
    }

    if (widgetDirty) {
        // Intersect with the widget geometry and translate to its coordinates.
        const QPoint offset(widget->mapTo(tlw, QPoint()));
        r &= widget->rect().translated(offset);
        r.translate(-offset);
    }
    return r;
}

/*!
    Returns the static content inside the \a parent if non-zero; otherwise the static content
    for the entire backing store is returned. The content will be clipped to \a withinClipRect
    if non-empty.
*/
QRegion QWidgetBackingStore::staticContents(QWidget *parent, const QRect &withinClipRect) const
{
    if (!parent && tlw->testAttribute(Qt::WA_StaticContents)) {
#if defined(Q_WS_QPA)
        const QSize surfaceGeometry(windowSurface->size());
#else
        const QRect surfaceGeometry(windowSurface->geometry());
#endif
        QRect surfaceRect(0, 0, surfaceGeometry.width(), surfaceGeometry.height());
        if (!withinClipRect.isEmpty())
            surfaceRect &= withinClipRect;
        return QRegion(surfaceRect);
    }

    QRegion region;
    if (parent && parent->d_func()->children.isEmpty())
        return region;

    const bool clipToRect = !withinClipRect.isEmpty();
    const int count = staticWidgets.count();
    for (int i = 0; i < count; ++i) {
        QWidget *w = staticWidgets.at(i);
        QWidgetPrivate *wd = w->d_func();
        if (!wd->isOpaque || !wd->extra || wd->extra->staticContentsSize.isEmpty()
            || !w->isVisible() || (parent && !parent->isAncestorOf(w))) {
            continue;
        }

        QRect rect(0, 0, wd->extra->staticContentsSize.width(), wd->extra->staticContentsSize.height());
        const QPoint offset = w->mapTo(parent ? parent : tlw, QPoint());
        if (clipToRect)
            rect &= withinClipRect.translated(-offset);
        if (rect.isEmpty())
            continue;

        rect &= wd->clipRect();
        if (rect.isEmpty())
            continue;

        QRegion visible(rect);
        wd->clipToEffectiveMask(visible);
        if (visible.isEmpty())
            continue;
        wd->subtractOpaqueSiblings(visible, 0, /*alsoNonOpaque=*/true);

        visible.translate(offset);
        region += visible;
    }

    return region;
}

static inline void sendUpdateRequest(QWidget *widget, bool updateImmediately)
{
    if (!widget)
        return;

    if (updateImmediately) {
        QEvent event(QEvent::UpdateRequest);
        QApplication::sendEvent(widget, &event);
    } else {
        QApplication::postEvent(widget, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
    }
}

/*!
    Marks the region of the widget as dirty (if not already marked as dirty) and
    posts an UpdateRequest event to the top-level widget (if not already posted).

    If updateImmediately is true, the event is sent immediately instead of posted.

    If invalidateBuffer is true, all widgets intersecting with the region will be dirty.

    If the widget paints directly on screen, the event is sent to the widget
    instead of the top-level widget, and invalidateBuffer is completely ignored.

    ### Qt 4.6: Merge into a template function (after MSVC isn't supported anymore).
*/
void QWidgetBackingStore::markDirty(const QRegion &rgn, QWidget *widget, bool updateImmediately,
                                    bool invalidateBuffer)
{
    Q_ASSERT(tlw->d_func()->extra);
    Q_ASSERT(tlw->d_func()->extra->topextra);
    Q_ASSERT(!tlw->d_func()->extra->topextra->inTopLevelResize);
    Q_ASSERT(widget->isVisible() && widget->updatesEnabled());
    Q_ASSERT(widget->window() == tlw);
    Q_ASSERT(!rgn.isEmpty());

#ifndef QT_NO_GRAPHICSEFFECT
    widget->d_func()->invalidateGraphicsEffectsRecursively();
#endif //QT_NO_GRAPHICSEFFECT

    if (widget->d_func()->paintOnScreen()) {
        if (widget->d_func()->dirty.isEmpty()) {
            widget->d_func()->dirty = rgn;
            sendUpdateRequest(widget, updateImmediately);
            return;
        } else if (qt_region_strictContains(widget->d_func()->dirty, widget->rect())) {
            if (updateImmediately)
                sendUpdateRequest(widget, updateImmediately);
            return; // Already dirty.
        }

        const bool eventAlreadyPosted = !widget->d_func()->dirty.isEmpty();
        widget->d_func()->dirty += rgn;
        if (!eventAlreadyPosted || updateImmediately)
            sendUpdateRequest(widget, updateImmediately);
        return;
    }

    if (fullUpdatePending) {
        if (updateImmediately)
            sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    if (!windowSurface->hasFeature(QWindowSurface::PartialUpdates)) {
        fullUpdatePending = true;
        sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    const QPoint offset = widget->mapTo(tlw, QPoint());
    const QRect widgetRect = widget->d_func()->effectiveRectFor(widget->rect());
    if (qt_region_strictContains(dirty, widgetRect.translated(offset))) {
        if (updateImmediately)
            sendUpdateRequest(tlw, updateImmediately);
        return; // Already dirty.
    }

    if (invalidateBuffer) {
        const bool eventAlreadyPosted = !dirty.isEmpty();
#ifndef QT_NO_GRAPHICSEFFECT
        if (widget->d_func()->graphicsEffect)
            dirty += widget->d_func()->effectiveRectFor(rgn.boundingRect()).translated(offset);
        else
#endif //QT_NO_GRAPHICSEFFECT
            dirty += rgn.translated(offset);
        if (!eventAlreadyPosted || updateImmediately)
            sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    if (dirtyWidgets.isEmpty()) {
        addDirtyWidget(widget, rgn);
        sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    if (widget->d_func()->inDirtyList) {
        if (!qt_region_strictContains(widget->d_func()->dirty, widgetRect)) {
#ifndef QT_NO_GRAPHICSEFFECT
            if (widget->d_func()->graphicsEffect)
                widget->d_func()->dirty += widget->d_func()->effectiveRectFor(rgn.boundingRect());
            else
#endif //QT_NO_GRAPHICSEFFECT
                widget->d_func()->dirty += rgn;
        }
    } else {
        addDirtyWidget(widget, rgn);
    }

    if (updateImmediately)
        sendUpdateRequest(tlw, updateImmediately);
}

/*!
    This function is equivalent to calling markDirty(QRegion(rect), ...), but
    is more efficient as it eliminates QRegion operations/allocations and can
    use the rect more precisely for additional cut-offs.

    ### Qt 4.6: Merge into a template function (after MSVC isn't supported anymore).
*/
void QWidgetBackingStore::markDirty(const QRect &rect, QWidget *widget, bool updateImmediately,
                                    bool invalidateBuffer)
{
    Q_ASSERT(tlw->d_func()->extra);
    Q_ASSERT(tlw->d_func()->extra->topextra);
    Q_ASSERT(!tlw->d_func()->extra->topextra->inTopLevelResize);
    Q_ASSERT(widget->isVisible() && widget->updatesEnabled());
    Q_ASSERT(widget->window() == tlw);
    Q_ASSERT(!rect.isEmpty());

#ifndef QT_NO_GRAPHICSEFFECT
    widget->d_func()->invalidateGraphicsEffectsRecursively();
#endif //QT_NO_GRAPHICSEFFECT

    if (widget->d_func()->paintOnScreen()) {
        if (widget->d_func()->dirty.isEmpty()) {
            widget->d_func()->dirty = QRegion(rect);
            sendUpdateRequest(widget, updateImmediately);
            return;
        } else if (qt_region_strictContains(widget->d_func()->dirty, rect)) {
            if (updateImmediately)
                sendUpdateRequest(widget, updateImmediately);
            return; // Already dirty.
        }

        const bool eventAlreadyPosted = !widget->d_func()->dirty.isEmpty();
        widget->d_func()->dirty += rect;
        if (!eventAlreadyPosted || updateImmediately)
            sendUpdateRequest(widget, updateImmediately);
        return;
    }

    if (fullUpdatePending) {
        if (updateImmediately)
            sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    if (!windowSurface->hasFeature(QWindowSurface::PartialUpdates)) {
        fullUpdatePending = true;
        sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    const QRect widgetRect = widget->d_func()->effectiveRectFor(rect);
    const QRect translatedRect(widgetRect.translated(widget->mapTo(tlw, QPoint())));
    if (qt_region_strictContains(dirty, translatedRect)) {
        if (updateImmediately)
            sendUpdateRequest(tlw, updateImmediately);
        return; // Already dirty
    }

    if (invalidateBuffer) {
        const bool eventAlreadyPosted = !dirty.isEmpty();
        dirty += translatedRect;
        if (!eventAlreadyPosted || updateImmediately)
            sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    if (dirtyWidgets.isEmpty()) {
        addDirtyWidget(widget, rect);
        sendUpdateRequest(tlw, updateImmediately);
        return;
    }

    if (widget->d_func()->inDirtyList) {
        if (!qt_region_strictContains(widget->d_func()->dirty, widgetRect))
            widget->d_func()->dirty += widgetRect;
    } else {
        addDirtyWidget(widget, rect);
    }

    if (updateImmediately)
        sendUpdateRequest(tlw, updateImmediately);
}

/*!
    Marks the \a region of the \a widget as dirty on screen. The \a region will be copied from
    the backing store to the \a widget's native parent next time flush() is called.

    Paint on screen widgets are ignored.
*/
void QWidgetBackingStore::markDirtyOnScreen(const QRegion &region, QWidget *widget, const QPoint &topLevelOffset)
{
    if (!widget || widget->d_func()->paintOnScreen() || region.isEmpty())
        return;

#if defined(Q_WS_QWS) || defined(Q_WS_MAC)
    if (!widget->testAttribute(Qt::WA_WState_InPaintEvent))
        dirtyOnScreen += region.translated(topLevelOffset);
    return;
#endif

    // Top-level.
    if (widget == tlw) {
        if (!widget->testAttribute(Qt::WA_WState_InPaintEvent))
            dirtyOnScreen += region;
        return;
    }

    // Alien widgets.
    if (!widget->internalWinId() && !widget->isWindow()) {
        QWidget *nativeParent = widget->nativeParentWidget();        // Alien widgets with the top-level as the native parent (common case).
        if (nativeParent == tlw) {
            if (!widget->testAttribute(Qt::WA_WState_InPaintEvent))
                dirtyOnScreen += region.translated(topLevelOffset);
            return;
        }

        // Alien widgets with native parent != tlw.
        QWidgetPrivate *nativeParentPrivate = nativeParent->d_func();
        if (!nativeParentPrivate->needsFlush)
            nativeParentPrivate->needsFlush = new QRegion;
        const QPoint nativeParentOffset = widget->mapTo(nativeParent, QPoint());
        *nativeParentPrivate->needsFlush += region.translated(nativeParentOffset);
        appendDirtyOnScreenWidget(nativeParent);
        return;
    }

    // Native child widgets.
    QWidgetPrivate *widgetPrivate = widget->d_func();
    if (!widgetPrivate->needsFlush)
        widgetPrivate->needsFlush = new QRegion;
    *widgetPrivate->needsFlush += region;
    appendDirtyOnScreenWidget(widget);
}

void QWidgetBackingStore::removeDirtyWidget(QWidget *w)
{
    if (!w)
        return;

    dirtyWidgetsRemoveAll(w);
    dirtyOnScreenWidgetsRemoveAll(w);
    resetWidget(w);

    QWidgetPrivate *wd = w->d_func();
    const int n = wd->children.count();
    for (int i = 0; i < n; ++i) {
        if (QWidget *child = qobject_cast<QWidget*>(wd->children.at(i)))
            removeDirtyWidget(child);
    }
}

#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_MANAGER)
bool QWidgetBackingStore::hasDirtyWindowDecoration() const
{
    QTLWExtra *tlwExtra = tlw->d_func()->maybeTopData();
    if (tlwExtra && tlwExtra->qwsManager)
        return !tlwExtra->qwsManager->d_func()->dirtyRegions.isEmpty();
    return false;
}

void QWidgetBackingStore::paintWindowDecoration()
{
    if (!hasDirtyWindowDecoration())
        return;

    QDecoration &decoration = QApplication::qwsDecoration();
    const QRect decorationRect = tlw->rect();
    QRegion decorationRegion = decoration.region(tlw, decorationRect);

    QWSManagerPrivate *managerPrivate = tlw->d_func()->topData()->qwsManager->d_func();
    const bool doClipping = !managerPrivate->entireDecorationNeedsRepaint
                            && !managerPrivate->dirtyClip.isEmpty();

    if (doClipping) {
        decorationRegion &= static_cast<QWSWindowSurface *>(windowSurface)->clipRegion();
        decorationRegion &= managerPrivate->dirtyClip;
    }

    if (decorationRegion.isEmpty())
        return;

    //### The QWS decorations do not always paint the pixels they promise to paint.
    // This causes painting problems with QWSMemorySurface. Since none of the other
    // window surfaces actually use the region, passing an empty region is a safe
    // workaround.

    windowSurface->beginPaint(QRegion());

    QPaintEngine *engine = windowSurface->paintDevice()->paintEngine();
    Q_ASSERT(engine);
    const QRegion oldSystemClip(engine->systemClip());
    engine->setSystemClip(decorationRegion.translated(tlwOffset));

    QPainter painter(windowSurface->paintDevice());
    painter.setFont(QApplication::font());
    painter.translate(tlwOffset);

    const int numDirty = managerPrivate->dirtyRegions.size();
    for (int i = 0; i < numDirty; ++i) {
        const int area = managerPrivate->dirtyRegions.at(i);

        QRegion clipRegion = decoration.region(tlw, decorationRect, area);
        if (!clipRegion.isEmpty()) {
            // Decoration styles changes the clip and assumes the old clip is non-empty,
            // so we have to set it, but in theory it shouldn't be required.
            painter.setClipRegion(clipRegion);
            decoration.paint(&painter, tlw, area, managerPrivate->dirtyStates.at(i));
        }
    }
    markDirtyOnScreen(decorationRegion, tlw, QPoint());

    painter.end();
    windowSurface->endPaint(decorationRegion);
    managerPrivate->clearDirtyRegions();
    engine->setSystemClip(oldSystemClip);
}
#endif

void QWidgetBackingStore::updateLists(QWidget *cur)
{
    if (!cur)
        return;

    QList<QObject*> children = cur->children();
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget*>(children.at(i));
        if (!child)
            continue;

        updateLists(child);
    }

    if (cur->testAttribute(Qt::WA_StaticContents))
        addStaticWidget(cur);

#ifdef Q_BACKINGSTORE_SUBSURFACES
    QTLWExtra *extra = cur->d_func()->maybeTopData();
    if (extra && extra->windowSurface && cur != tlw)
        subSurfaces.append(extra->windowSurface);
#endif
}

QWidgetBackingStore::QWidgetBackingStore(QWidget *topLevel)
    : tlw(topLevel), dirtyOnScreenWidgets(0), hasDirtyFromPreviousSync(false)
    , fullUpdatePending(0)
{
    windowSurface = tlw->windowSurface();
    if (!windowSurface)
        windowSurface = topLevel->d_func()->createDefaultWindowSurface();

    // The QWindowSurface constructor will call QWidget::setWindowSurface(),
    // but automatically created surfaces should not be added to the topdata.
#ifdef Q_BACKINGSTORE_SUBSURFACES
    Q_ASSERT(topLevel->d_func()->topData()->windowSurface == windowSurface);
#endif
    topLevel->d_func()->topData()->windowSurface = 0;

    // Ensure all existing subsurfaces and static widgets are added to their respective lists.
    updateLists(topLevel);
}

QWidgetBackingStore::~QWidgetBackingStore()
{
    for (int c = 0; c < dirtyWidgets.size(); ++c) {
        resetWidget(dirtyWidgets.at(c));
    }

    delete windowSurface;
    windowSurface = 0;
    delete dirtyOnScreenWidgets;
    dirtyOnScreenWidgets = 0;
}

//parent's coordinates; move whole rect; update parent and widget
//assume the screen blt has already been done, so we don't need to refresh that part
void QWidgetPrivate::moveRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    if (!q->isVisible() || (dx == 0 && dy == 0))
        return;

    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();
    if (x->inTopLevelResize)
        return;

    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_MOVE").toInt() == 0;
    }

    QWidget *pw = q->parentWidget();
    QPoint toplevelOffset = pw->mapTo(tlw, QPoint());
    QWidgetPrivate *pd = pw->d_func();
    QRect clipR(pd->clipRect());
#ifdef Q_WS_QWS
    QWidgetBackingStore *wbs = x->backingStore.data();
    QWSWindowSurface *surface = static_cast<QWSWindowSurface*>(wbs->windowSurface);
    clipR = clipR.intersected(surface->clipRegion().translated(-toplevelOffset).boundingRect());
#endif
    const QRect newRect(rect.translated(dx, dy));
    QRect destRect = rect.intersected(clipR);
    if (destRect.isValid())
        destRect = destRect.translated(dx, dy).intersected(clipR);
    const QRect sourceRect(destRect.translated(-dx, -dy));
    const QRect parentRect(rect & clipR);

    bool accelerateMove = accelEnv && isOpaque
#ifndef QT_NO_GRAPHICSVIEW
                          // No accelerate move for proxy widgets.
                          && !tlw->d_func()->extra->proxyWidget
#endif
                          && !isOverlapped(sourceRect) && !isOverlapped(destRect);

    if (!accelerateMove) {
        QRegion parentR(effectiveRectFor(parentRect));
        if (!extra || !extra->hasMask) {
            parentR -= newRect;
        } else {
            // invalidateBuffer() excludes anything outside the mask
            parentR += newRect & clipR;
        }
        pd->invalidateBuffer(parentR);
        invalidateBuffer((newRect & clipR).translated(-data.crect.topLeft()));
    } else {

        QWidgetBackingStore *wbs = x->backingStore.data();
        QRegion childExpose(newRect & clipR);

        if (sourceRect.isValid() && wbs->bltRect(sourceRect, dx, dy, pw))
            childExpose -= destRect;

        if (!pw->updatesEnabled())
            return;

        const bool childUpdatesEnabled = q->updatesEnabled();
        if (childUpdatesEnabled && !childExpose.isEmpty()) {
            childExpose.translate(-data.crect.topLeft());
            wbs->markDirty(childExpose, q);
            isMoved = true;
        }

        QRegion parentExpose(parentRect);
        parentExpose -= newRect;
        if (extra && extra->hasMask)
            parentExpose += QRegion(newRect) - extra->mask.translated(data.crect.topLeft());

        if (!parentExpose.isEmpty()) {
            wbs->markDirty(parentExpose, pw);
            pd->isMoved = true;
        }

        if (childUpdatesEnabled) {
            QRegion needsFlush(sourceRect);
            needsFlush += destRect;
            wbs->markDirtyOnScreen(needsFlush, pw, toplevelOffset);
        }
    }
}

//widget's coordinates; scroll within rect;  only update widget
void QWidgetPrivate::scrollRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();
    if (x->inTopLevelResize)
        return;

    QWidgetBackingStore *wbs = x->backingStore.data();
    if (!wbs)
        return;

    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_SCROLL").toInt() == 0;
    }

    QRect scrollRect = rect & clipRect();
    bool overlapped = false;
    bool accelerateScroll = accelEnv && isOpaque
                            && !(overlapped = isOverlapped(scrollRect.translated(data.crect.topLeft())));

#if defined(Q_WS_QWS)
    QWSWindowSurface *surface;
    surface = static_cast<QWSWindowSurface*>(wbs->windowSurface);

    if (accelerateScroll && !surface->isBuffered()) {
        const QRegion surfaceClip = surface->clipRegion();
        const QRegion outsideClip = QRegion(rect) - surfaceClip;
        if (!outsideClip.isEmpty()) {
            const QVector<QRect> clipped = (surfaceClip & rect).rects();
            if (clipped.size() < 8) {
                for (int i = 0; i < clipped.size(); ++i)
                    this->scrollRect(clipped.at(i), dx, dy);
                return;
            } else {
                accelerateScroll = false;
            }
        }
    }
#endif // Q_WS_QWS

    if (!accelerateScroll) {
        if (overlapped) {
            QRegion region(scrollRect);
            subtractOpaqueSiblings(region);
            invalidateBuffer(region);
        }else {
            invalidateBuffer(scrollRect);
        }
    } else {
        const QPoint toplevelOffset = q->mapTo(tlw, QPoint());
#ifdef Q_WS_QWS
        QWSWindowSurface *surface = static_cast<QWSWindowSurface*>(wbs->windowSurface);
        const QRegion clip = surface->clipRegion().translated(-toplevelOffset) & scrollRect;
        const QRect clipBoundingRect = clip.boundingRect();
        scrollRect &= clipBoundingRect;
#endif
        const QRect destRect = scrollRect.translated(dx, dy) & scrollRect;
        const QRect sourceRect = destRect.translated(-dx, -dy);

        QRegion childExpose(scrollRect);
        if (sourceRect.isValid()) {
            if (wbs->bltRect(sourceRect, dx, dy, q))
                childExpose -= destRect;
        }

        if (inDirtyList) {
            if (rect == q->rect()) {
                dirty.translate(dx, dy);
            } else {
                QRegion dirtyScrollRegion = dirty.intersected(scrollRect);
                if (!dirtyScrollRegion.isEmpty()) {
                    dirty -= dirtyScrollRegion;
                    dirtyScrollRegion.translate(dx, dy);
                    dirty += dirtyScrollRegion;
                }
            }
        }

        if (!q->updatesEnabled())
            return;

        if (!childExpose.isEmpty()) {
            wbs->markDirty(childExpose, q);
            isScrolled = true;
        }

        // Instead of using native scroll-on-screen, we copy from
        // backingstore, giving only one screen update for each
        // scroll, and a solid appearance
        wbs->markDirtyOnScreen(destRect, q, toplevelOffset);
    }
}

static inline bool discardSyncRequest(QWidget *tlw, QTLWExtra *tlwExtra)
{
    if (!tlw || !tlwExtra)
        return true;

#ifdef Q_WS_X11
    // Delay the sync until we get an Expose event from X11 (initial show).
    // Qt::WA_Mapped is set to true, but the actual mapping has not yet occurred.
    // However, we must repaint immediately regardless of the state if someone calls repaint().
    if (tlwExtra->waitingForMapNotify && !tlwExtra->inRepaint)
        return true;
#endif

    if (!tlw->testAttribute(Qt::WA_Mapped))
        return true;

    if (!tlw->isVisible()
#ifndef Q_WS_X11
        // If we're minimized on X11, WA_Mapped will be false and we
        // will return in the case above. Some window managers on X11
        // sends us the PropertyNotify to change the minimized state
        // *AFTER* we've received the expose event, which is baaad.
        || tlw->isMinimized()
#endif
        )
        return true;

    return false;
}

/*!
    Synchronizes the \a exposedRegion of the \a exposedWidget with the backing store.

    If there's nothing to repaint, the area is flushed and painting does not occur;
    otherwise the area is marked as dirty on screen and will be flushed right after
    we are done with all painting.
*/
void QWidgetBackingStore::sync(QWidget *exposedWidget, const QRegion &exposedRegion)
{
    QTLWExtra *tlwExtra = tlw->d_func()->maybeTopData();
    if (discardSyncRequest(tlw, tlwExtra) || tlwExtra->inTopLevelResize)
        return;

    if (!exposedWidget || !exposedWidget->internalWinId() || !exposedWidget->isVisible()
        || !exposedWidget->updatesEnabled() || exposedRegion.isEmpty()) {
        return;
    }

    // If there's no preserved contents support we always need
    // to do a full repaint before flushing
    if (!windowSurface->hasFeature(QWindowSurface::PreservedContents))
        fullUpdatePending = true;

    // Nothing to repaint.
    if (!isDirty()) {
        qt_flush(exposedWidget, exposedRegion, windowSurface, tlw, tlwOffset);
        return;
    }

    if (exposedWidget != tlw)
        markDirtyOnScreen(exposedRegion, exposedWidget, exposedWidget->mapTo(tlw, QPoint()));
    else
        markDirtyOnScreen(exposedRegion, exposedWidget, QPoint());
    sync();
}

/*!
    Synchronizes the backing store, i.e. dirty areas are repainted and flushed.
*/
void QWidgetBackingStore::sync()
{
    QTLWExtra *tlwExtra = tlw->d_func()->maybeTopData();
    if (discardSyncRequest(tlw, tlwExtra)) {
        // If the top-level is minimized, it's not visible on the screen so we can delay the
        // update until it's shown again. In order to do that we must keep the dirty states.
        // These will be cleared when we receive the first expose after showNormal().
        // However, if the widget is not visible (isVisible() returns false), everything will
        // be invalidated once the widget is shown again, so clear all dirty states.
        if (!tlw->isVisible()) {
            dirty = QRegion();
            for (int i = 0; i < dirtyWidgets.size(); ++i)
                resetWidget(dirtyWidgets.at(i));
            dirtyWidgets.clear();
            fullUpdatePending = false;
        }
        return;
    }

    const bool updatesDisabled = !tlw->updatesEnabled();
    bool repaintAllWidgets = false;

    const bool inTopLevelResize = tlwExtra->inTopLevelResize;
    const QRect tlwRect(topLevelRect());
#ifdef  Q_WS_QPA
    const QRect surfaceGeometry(tlwRect.topLeft(), windowSurface->size());
#else
    const QRect surfaceGeometry(windowSurface->geometry());
#endif
    if ((fullUpdatePending || inTopLevelResize || surfaceGeometry.size() != tlwRect.size()) && !updatesDisabled) {
        if (hasStaticContents()) {
            // Repaint existing dirty area and newly visible area.
            const QRect clipRect(0, 0, surfaceGeometry.width(), surfaceGeometry.height());
            const QRegion staticRegion(staticContents(0, clipRect));
            QRegion newVisible(0, 0, tlwRect.width(), tlwRect.height());
            newVisible -= staticRegion;
            dirty += newVisible;
            windowSurface->setStaticContents(staticRegion);
        } else {
            // Repaint everything.
            dirty = QRegion(0, 0, tlwRect.width(), tlwRect.height());
            for (int i = 0; i < dirtyWidgets.size(); ++i)
                resetWidget(dirtyWidgets.at(i));
            dirtyWidgets.clear();
            repaintAllWidgets = true;
        }
    }

#ifdef Q_WS_QPA
    if (inTopLevelResize || surfaceGeometry.size() != tlwRect.size())
        windowSurface->resize(tlwRect.size());
#else
    if (inTopLevelResize || surfaceGeometry != tlwRect)
        windowSurface->setGeometry(tlwRect);
#endif

    if (updatesDisabled)
        return;

    if (hasDirtyFromPreviousSync)
        dirty += dirtyFromPreviousSync;

    // Contains everything that needs repaint.
    QRegion toClean(dirty);

    // Loop through all update() widgets and remove them from the list before they are
    // painted (in case someone calls update() in paintEvent). If the widget is opaque
    // and does not have transparent overlapping siblings, append it to the
    // opaqueNonOverlappedWidgets list and paint it directly without composition.
    QVarLengthArray<QWidget *, 32> opaqueNonOverlappedWidgets;
    for (int i = 0; i < dirtyWidgets.size(); ++i) {
        QWidget *w = dirtyWidgets.at(i);
        QWidgetPrivate *wd = w->d_func();
        if (wd->data.in_destructor)
            continue;

        // Clip with mask() and clipRect().
        wd->dirty &= wd->clipRect();
        wd->clipToEffectiveMask(wd->dirty);

        // Subtract opaque siblings and children.
        bool hasDirtySiblingsAbove = false;
        // We know for sure that the widget isn't overlapped if 'isMoved' is true.
        if (!wd->isMoved)
            wd->subtractOpaqueSiblings(wd->dirty, &hasDirtySiblingsAbove);
        // Scrolled and moved widgets must draw all children.
        if (!wd->isScrolled && !wd->isMoved)
            wd->subtractOpaqueChildren(wd->dirty, w->rect());

        if (wd->dirty.isEmpty()) {
            resetWidget(w);
            continue;
        }

        const QRegion widgetDirty(w != tlw ? wd->dirty.translated(w->mapTo(tlw, QPoint()))
                                           : wd->dirty);
        toClean += widgetDirty;

#ifndef QT_NO_GRAPHICSVIEW
        if (tlw->d_func()->extra->proxyWidget) {
            resetWidget(w);
            continue;
        }
#endif

        if (!hasDirtySiblingsAbove && wd->isOpaque && !dirty.intersects(widgetDirty.boundingRect())) {
            opaqueNonOverlappedWidgets.append(w);
        } else {
            resetWidget(w);
            dirty += widgetDirty;
        }
    }
    dirtyWidgets.clear();

    fullUpdatePending = false;

    if (toClean.isEmpty()) {
        // Nothing to repaint. However, we might have newly exposed areas on the
        // screen if this function was called from sync(QWidget *, QRegion)), so
        // we have to make sure those are flushed.
        flush();
        return;
    }

#ifndef QT_NO_GRAPHICSVIEW
    if (tlw->d_func()->extra->proxyWidget) {
        updateStaticContentsSize();
        dirty = QRegion();
        const QVector<QRect> rects(toClean.rects());
        for (int i = 0; i < rects.size(); ++i)
            tlw->d_func()->extra->proxyWidget->update(rects.at(i));
        return;
    }
#endif

#ifndef Q_BACKINGSTORE_SUBSURFACES
    BeginPaintInfo beginPaintInfo;
    beginPaint(toClean, tlw, windowSurface, &beginPaintInfo);
    if (beginPaintInfo.nothingToPaint) {
        for (int i = 0; i < opaqueNonOverlappedWidgets.size(); ++i)
            resetWidget(opaqueNonOverlappedWidgets[i]);
        dirty = QRegion();
        return;
    }
#endif

    // Must do this before sending any paint events because
    // the size may change in the paint event.
    updateStaticContentsSize();
    const QRegion dirtyCopy(dirty);
    dirty = QRegion();

    // Paint opaque non overlapped widgets.
    for (int i = 0; i < opaqueNonOverlappedWidgets.size(); ++i) {
        QWidget *w = opaqueNonOverlappedWidgets[i];
        QWidgetPrivate *wd = w->d_func();

        int flags = QWidgetPrivate::DrawRecursive;
        // Scrolled and moved widgets must draw all children.
        if (!wd->isScrolled && !wd->isMoved)
            flags |= QWidgetPrivate::DontDrawOpaqueChildren;
        if (w == tlw)
            flags |= QWidgetPrivate::DrawAsRoot;

        QRegion toBePainted(wd->dirty);
        resetWidget(w);

#ifdef Q_BACKINGSTORE_SUBSURFACES
        QWindowSurface *subSurface = w->windowSurface();
        BeginPaintInfo beginPaintInfo;

        QPoint off = w->mapTo(tlw, QPoint());
        toBePainted.translate(off);
        beginPaint(toBePainted, w, subSurface, &beginPaintInfo, true);
        toBePainted.translate(-off);

        if (beginPaintInfo.nothingToPaint)
            continue;

        if (beginPaintInfo.windowSurfaceRecreated) {
            // Eep the window surface has changed. The old one may have been
            // deleted, in which case we will segfault on the call to
            // painterOffset() below. Use the new window surface instead.
            subSurface = w->windowSurface();
        }

        QPoint offset(tlwOffset);
        if (subSurface == windowSurface)
            offset += w->mapTo(tlw, QPoint());
        else
            offset = static_cast<QWSWindowSurface*>(subSurface)->painterOffset();
        wd->drawWidget(subSurface->paintDevice(), toBePainted, offset, flags, 0, this);

        endPaint(toBePainted, subSurface, &beginPaintInfo);
#else
        QPoint offset(tlwOffset);
        if (w != tlw)
            offset += w->mapTo(tlw, QPoint());
        wd->drawWidget(windowSurface->paintDevice(), toBePainted, offset, flags, 0, this);
#endif
    }

    // Paint the rest with composition.
#ifndef Q_BACKINGSTORE_SUBSURFACES
    if (repaintAllWidgets || !dirtyCopy.isEmpty()) {
        const int flags = QWidgetPrivate::DrawAsRoot | QWidgetPrivate::DrawRecursive;
        tlw->d_func()->drawWidget(windowSurface->paintDevice(), dirtyCopy, tlwOffset, flags, 0, this);
    }

    endPaint(toClean, windowSurface, &beginPaintInfo);
#else
    if (!repaintAllWidgets && dirtyCopy.isEmpty())
        return; // Nothing more to paint.

    QList<QWindowSurface *> surfaceList(subSurfaces);
    surfaceList.prepend(windowSurface);
    const QRect dirtyBoundingRect(dirtyCopy.boundingRect());

    // Loop through all window surfaces (incl. the top-level surface) and
    // repaint those intersecting with the bounding rect of the dirty region.
    for (int i = 0; i < surfaceList.size(); ++i) {
        QWindowSurface *subSurface = surfaceList.at(i);
        QWidget *w = subSurface->window();
        QWidgetPrivate *wd = w->d_func();

        const QRect clipRect = wd->clipRect().translated(w->mapTo(tlw, QPoint()));
        if (!qRectIntersects(dirtyBoundingRect, clipRect))
            continue;

        toClean = dirtyCopy;
        BeginPaintInfo beginPaintInfo;
        beginPaint(toClean, w, subSurface, &beginPaintInfo);
        if (beginPaintInfo.nothingToPaint)
            continue;

        if (beginPaintInfo.windowSurfaceRecreated) {
            // Eep the window surface has changed. The old one may have been
            // deleted, in which case we will segfault on the call to
            // painterOffset() below. Use the new window surface instead.
            subSurface = w->windowSurface();
        }

        int flags = QWidgetPrivate::DrawRecursive;
        if (w == tlw)
            flags |= QWidgetPrivate::DrawAsRoot;
        const QPoint painterOffset = static_cast<QWSWindowSurface*>(subSurface)->painterOffset();
        wd->drawWidget(subSurface->paintDevice(), toClean, painterOffset, flags, 0, this);

        endPaint(toClean, subSurface, &beginPaintInfo);
    }
#endif
}

/*!
    Flushes the contents of the backing store into the top-level widget.
    If the \a widget is non-zero, the content is flushed to the \a widget.
    If the \a surface is non-zero, the content of the \a surface is flushed.
*/
void QWidgetBackingStore::flush(QWidget *widget, QWindowSurface *surface)
{
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_MANAGER)
    paintWindowDecoration();
#endif

    if (!dirtyOnScreen.isEmpty()) {
        QWidget *target = widget ? widget : tlw;
        QWindowSurface *source = surface ? surface : windowSurface;
        qt_flush(target, dirtyOnScreen, source, tlw, tlwOffset);
        dirtyOnScreen = QRegion();
    }

    if (!dirtyOnScreenWidgets || dirtyOnScreenWidgets->isEmpty())
        return;

    for (int i = 0; i < dirtyOnScreenWidgets->size(); ++i) {
        QWidget *w = dirtyOnScreenWidgets->at(i);
        QWidgetPrivate *wd = w->d_func();
        Q_ASSERT(wd->needsFlush);
        qt_flush(w, *wd->needsFlush, windowSurface, tlw, tlwOffset);
        *wd->needsFlush = QRegion();
    }
    dirtyOnScreenWidgets->clear();
}

static inline bool discardInvalidateBufferRequest(QWidget *widget, QTLWExtra *tlwExtra)
{
    Q_ASSERT(widget);
    if (QApplication::closingDown())
        return true;

    if (!tlwExtra || tlwExtra->inTopLevelResize || !tlwExtra->backingStore)
        return true;

    if (!widget->isVisible() || !widget->updatesEnabled())
        return true;

    return false;
}

/*!
    Invalidates the buffer when the widget is resized.
    Static areas are never invalidated unless absolutely needed.
*/
void QWidgetPrivate::invalidateBuffer_resizeHelper(const QPoint &oldPos, const QSize &oldSize)
{
    Q_Q(QWidget);
    Q_ASSERT(!q->isWindow());
    Q_ASSERT(q->parentWidget());

    const bool staticContents = q->testAttribute(Qt::WA_StaticContents);
    const bool sizeDecreased = (data.crect.width() < oldSize.width())
                               || (data.crect.height() < oldSize.height());

    const QPoint offset(data.crect.x() - oldPos.x(), data.crect.y() - oldPos.y());
    const bool parentAreaExposed = !offset.isNull() || sizeDecreased;
    const QRect newWidgetRect(q->rect());
    const QRect oldWidgetRect(0, 0, oldSize.width(), oldSize.height());

    if (!staticContents || graphicsEffect) {
        QRegion staticChildren;
        QWidgetBackingStore *bs = 0;
        if (offset.isNull() && (bs = maybeBackingStore()))
            staticChildren = bs->staticContents(q, oldWidgetRect);
        const bool hasStaticChildren = !staticChildren.isEmpty();

        if (hasStaticChildren) {
            QRegion dirty(newWidgetRect);
            dirty -= staticChildren;
            invalidateBuffer(dirty);
        } else {
            // Entire widget needs repaint.
            invalidateBuffer(newWidgetRect);
        }

        if (!parentAreaExposed)
            return;

        // Invalidate newly exposed area of the parent.
        if (!graphicsEffect && extra && extra->hasMask) {
            QRegion parentExpose(extra->mask.translated(oldPos));
            parentExpose &= QRect(oldPos, oldSize);
            if (hasStaticChildren)
                parentExpose -= data.crect; // Offset is unchanged, safe to do this.
            q->parentWidget()->d_func()->invalidateBuffer(parentExpose);
        } else {
            if (hasStaticChildren && !graphicsEffect) {
                QRegion parentExpose(QRect(oldPos, oldSize));
                parentExpose -= data.crect; // Offset is unchanged, safe to do this.
                q->parentWidget()->d_func()->invalidateBuffer(parentExpose);
            } else {
                q->parentWidget()->d_func()->invalidateBuffer(effectiveRectFor(QRect(oldPos, oldSize)));
            }
        }
        return;
    }

    // Move static content to its new position.
    if (!offset.isNull()) {
        if (sizeDecreased) {
            const QSize minSize(qMin(oldSize.width(), data.crect.width()),
                                qMin(oldSize.height(), data.crect.height()));
            moveRect(QRect(oldPos, minSize), offset.x(), offset.y());
        } else {
            moveRect(QRect(oldPos, oldSize), offset.x(), offset.y());
        }
    }

    // Invalidate newly visible area of the widget.
    if (!sizeDecreased || !oldWidgetRect.contains(newWidgetRect)) {
        QRegion newVisible(newWidgetRect);
        newVisible -= oldWidgetRect;
        invalidateBuffer(newVisible);
    }

    if (!parentAreaExposed)
        return;

    // Invalidate newly exposed area of the parent.
    const QRect oldRect(oldPos, oldSize);
    if (extra && extra->hasMask) {
        QRegion parentExpose(oldRect);
        parentExpose &= extra->mask.translated(oldPos);
        parentExpose -= (extra->mask.translated(data.crect.topLeft()) & data.crect);
        q->parentWidget()->d_func()->invalidateBuffer(parentExpose);
    } else {
        QRegion parentExpose(oldRect);
        parentExpose -= data.crect;
        q->parentWidget()->d_func()->invalidateBuffer(parentExpose);
    }
}

/*!
    Invalidates the \a rgn (in widget's coordinates) of the backing store, i.e.
    all widgets intersecting with the region will be repainted when the backing store
    is synced.

    ### Qt 4.6: Merge into a template function (after MSVC isn't supported anymore).
*/
void QWidgetPrivate::invalidateBuffer(const QRegion &rgn)
{
    Q_Q(QWidget);

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (discardInvalidateBufferRequest(q, tlwExtra) || rgn.isEmpty())
        return;

    QRegion wrgn(rgn);
    wrgn &= clipRect();
    if (!graphicsEffect && extra && extra->hasMask)
        wrgn &= extra->mask;
    if (wrgn.isEmpty())
        return;

    tlwExtra->backingStore->markDirty(wrgn, q, false, true);
}

/*!
    This function is equivalent to calling invalidateBuffer(QRegion(rect), ...), but
    is more efficient as it eliminates QRegion operations/allocations and can
    use the rect more precisely for additional cut-offs.

    ### Qt 4.6: Merge into a template function (after MSVC isn't supported anymore).
*/
void QWidgetPrivate::invalidateBuffer(const QRect &rect)
{
    Q_Q(QWidget);

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (discardInvalidateBufferRequest(q, tlwExtra) || rect.isEmpty())
        return;

    QRect wRect(rect);
    wRect &= clipRect();
    if (wRect.isEmpty())
        return;

    if (graphicsEffect || !extra || !extra->hasMask) {
        tlwExtra->backingStore->markDirty(wRect, q, false, true);
        return;
    }

    QRegion wRgn(extra->mask);
    wRgn &= wRect;
    if (wRgn.isEmpty())
        return;

    tlwExtra->backingStore->markDirty(wRgn, q, false, true);
}

void QWidgetPrivate::repaint_sys(const QRegion &rgn)
{
    if (data.in_destructor)
        return;

    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_StaticContents)) {
        if (!extra)
            createExtra();
        extra->staticContentsSize = data.crect.size();
    }

#ifdef Q_WS_QPA //Don't even call q->p
    QPaintEngine *engine = 0;
#else
    QPaintEngine *engine = q->paintEngine();
#endif
    // QGLWidget does not support partial updates if:
    // 1) The context is double buffered
    // 2) The context is single buffered and auto-fill background is enabled.
    const bool noPartialUpdateSupport = (engine && (engine->type() == QPaintEngine::OpenGL
                                                || engine->type() == QPaintEngine::OpenGL2))
                                        && (usesDoubleBufferedGLContext || q->autoFillBackground());
    QRegion toBePainted(noPartialUpdateSupport ? q->rect() : rgn);

#ifdef Q_WS_MAC
    // No difference between update() and repaint() on the Mac.
    update_sys(toBePainted);
    return;
#endif

    toBePainted &= clipRect();
    clipToEffectiveMask(toBePainted);
    if (toBePainted.isEmpty())
        return; // Nothing to repaint.

#ifndef QT_NO_PAINT_DEBUG
    bool flushed = QWidgetBackingStore::flushPaint(q, toBePainted);
#endif

    drawWidget(q, toBePainted, QPoint(), QWidgetPrivate::DrawAsRoot | QWidgetPrivate::DrawPaintOnScreen, 0);

#ifndef QT_NO_PAINT_DEBUG
    if (flushed)
        QWidgetBackingStore::unflushPaint(q, toBePainted);
#endif

    if (!q->testAttribute(Qt::WA_PaintOutsidePaintEvent) && q->paintingActive())
        qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");
}


QT_END_NAMESPACE

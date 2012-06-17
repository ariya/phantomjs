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

#ifndef QGRAPHICSVIEW_P_H
#define QGRAPHICSVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgraphicsview.h"

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

#include <QtGui/qevent.h>
#include <QtCore/qcoreapplication.h>
#include "qgraphicssceneevent.h"
#include <QtGui/qstyleoption.h>
#include <private/qabstractscrollarea_p.h>
#include <private/qapplication_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QGraphicsViewPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsView)
public:
    QGraphicsViewPrivate();

    void recalculateContentSize();
    void centerView(QGraphicsView::ViewportAnchor anchor);

    QPainter::RenderHints renderHints;

    QGraphicsView::DragMode dragMode;

    quint32 sceneInteractionAllowed : 1;
    quint32 hasSceneRect : 1;
    quint32 connectedToScene : 1;
    quint32 useLastMouseEvent : 1;
    quint32 identityMatrix : 1;
    quint32 dirtyScroll : 1;
    quint32 accelerateScrolling : 1;
    quint32 keepLastCenterPoint : 1;
    quint32 transforming : 1;
    quint32 handScrolling : 1;
    quint32 mustAllocateStyleOptions : 1;
    quint32 mustResizeBackgroundPixmap : 1;
    quint32 fullUpdatePending : 1;
    quint32 hasUpdateClip : 1;
    quint32 padding : 18;

    QRectF sceneRect;
    void updateLastCenterPoint();

    qint64 horizontalScroll() const;
    qint64 verticalScroll() const;

    QRectF mapRectToScene(const QRect &rect) const;
    QRectF mapRectFromScene(const QRectF &rect) const;

    QRect updateClip;
    QPointF mousePressItemPoint;
    QPointF mousePressScenePoint;
    QPoint mousePressViewPoint;
    QPoint mousePressScreenPoint;
    QPointF lastMouseMoveScenePoint;
    QPoint lastMouseMoveScreenPoint;
    QPoint dirtyScrollOffset;
    Qt::MouseButton mousePressButton;
    QTransform matrix;
    qint64 scrollX, scrollY;
    void updateScroll();

    qreal leftIndent;
    qreal topIndent;

    // Replaying mouse events
    QMouseEvent lastMouseEvent;
    void replayLastMouseEvent();
    void storeMouseEvent(QMouseEvent *event);
    void mouseMoveEventHandler(QMouseEvent *event);

    QPointF lastCenterPoint;
    Qt::Alignment alignment;

    QGraphicsView::ViewportAnchor transformationAnchor;
    QGraphicsView::ViewportAnchor resizeAnchor;
    QGraphicsView::ViewportUpdateMode viewportUpdateMode;
    QGraphicsView::OptimizationFlags optimizationFlags;

    QPointer<QGraphicsScene> scene;
#ifndef QT_NO_RUBBERBAND
    QRect rubberBandRect;
    QRegion rubberBandRegion(const QWidget *widget, const QRect &rect) const;
    bool rubberBanding;
    Qt::ItemSelectionMode rubberBandSelectionMode;
#endif
    int handScrollMotions;

    QGraphicsView::CacheMode cacheMode;

    QVector<QStyleOptionGraphicsItem> styleOptions;
    QStyleOptionGraphicsItem *allocStyleOptionsArray(int numItems);
    void freeStyleOptionsArray(QStyleOptionGraphicsItem *array);

    QBrush backgroundBrush;
    QBrush foregroundBrush;
    QPixmap backgroundPixmap;
    QRegion backgroundPixmapExposed;

#ifndef QT_NO_CURSOR
    QCursor originalCursor;
    bool hasStoredOriginalCursor;
    void _q_setViewportCursor(const QCursor &cursor);
    void _q_unsetViewportCursor();
#endif

    QGraphicsSceneDragDropEvent *lastDragDropEvent;
    void storeDragDropEvent(const QGraphicsSceneDragDropEvent *event);
    void populateSceneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                                    QDropEvent *source);

    QRect mapToViewRect(const QGraphicsItem *item, const QRectF &rect) const;
    QRegion mapToViewRegion(const QGraphicsItem *item, const QRectF &rect) const;
    QRegion dirtyRegion;
    QRect dirtyBoundingRect;
    void processPendingUpdates();
    inline void updateAll()
    {
        viewport->update();
        fullUpdatePending = true;
        dirtyBoundingRect = QRect();
        dirtyRegion = QRegion();
    }

    inline void dispatchPendingUpdateRequests()
    {
#ifdef Q_WS_MAC
        // QWidget::update() works slightly different on the Mac without the raster engine;
        // it's not part of our backing store so it needs special threatment.
        if (QApplicationPrivate::graphics_system_name != QLatin1String("raster")) {
            // At this point either HIViewSetNeedsDisplay (Carbon) or setNeedsDisplay: YES (Cocoa)
            // is called, which means there's a pending update request. We want to dispatch it
            // now because otherwise graphics view updates would require two
            // round-trips in the event loop before the item is painted.
            extern void qt_mac_dispatchPendingUpdateRequests(QWidget *);
            qt_mac_dispatchPendingUpdateRequests(viewport->window());
        } else
#endif // !Q_WS_MAC
        {
            if (qt_widget_private(viewport)->paintOnScreen())
                QCoreApplication::sendPostedEvents(viewport, QEvent::UpdateRequest);
            else
                QCoreApplication::sendPostedEvents(viewport->window(), QEvent::UpdateRequest);
        }
    }

    void setUpdateClip(QGraphicsItem *);

    inline bool updateRectF(const QRectF &rect)
    {
        if (rect.isEmpty())
            return false;
        if (optimizationFlags & QGraphicsView::DontAdjustForAntialiasing)
            return updateRect(rect.toAlignedRect().adjusted(-1, -1, 1, 1));
        return updateRect(rect.toAlignedRect().adjusted(-2, -2, 2, 2));
    }

    bool updateRect(const QRect &rect);
    bool updateRegion(const QRectF &rect, const QTransform &xform);
    bool updateSceneSlotReimplementedChecked;
    QRegion exposedRegion;

    QList<QGraphicsItem *> findItems(const QRegion &exposedRegion, bool *allItems,
                                     const QTransform &viewTransform) const;

    QPointF mapToScene(const QPointF &point) const;
    QRectF mapToScene(const QRectF &rect) const;
    static void translateTouchEvent(QGraphicsViewPrivate *d, QTouchEvent *touchEvent);
    void updateInputMethodSensitivity();
};

QT_END_NAMESPACE

#endif // QT_NO_GRAPHICSVIEW

#endif
